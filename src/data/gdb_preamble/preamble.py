# SPDX-License-Identifier: GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
# SPDX-FileCopyrightText: 2021-2022 Harald Sitter <sitter@kde.org>

import os
import sys

sys.path.append(f'{os.path.dirname(os.path.dirname(os.path.abspath(__file__)))}/')

# Initialize sentry reports for exceptions in this script
# NOTE: this happens before other imports so we get reports when we have systems with missing deps
try:
    import sentry_sdk
    sentry_sdk.init(
        dsn="https://d6d53bb0121041dd97f59e29051a1781@crash-reports.kde.org/13",
        traces_sample_rate=1.0,
        release="drkonqi@" + os.getenv('DRKONQI_VERSION'),
        dist=os.getenv('DRKONQI_DISTRIBUTION'),
        ignore_errors=[KeyboardInterrupt],
        # Shutdown performance doesn't matter all that much, give us ample time to send a possible report instead.
        shutdown_timeout=30,
    )
except ImportError:
    print("python sentry-sdk not installed :(")

os.environ['LC_ALL'] = 'C.UTF-8'

import gdb
from gdb.FrameDecorator import FrameDecorator

from datetime import datetime, timezone
import uuid
import json
import subprocess
import signal
import re
import binascii
import platform
import multiprocessing
from pathlib import Path
import psutil
import traceback

crashed_thread = None
core_images = []

class UnexpectedMappingException(Exception):
    pass

class NoBuildIdException(Exception):
    pass

def mangle_path(path):
    if not path:
        return path
    return re.sub(str(Path.home()), "$HOME", path, count=1)

class SentryQMLThread:
    def __init__(self):
        self.payload = None

        # TODO this is largely a code dupe of print_qml_trace

        if gdb.selected_inferior().connection.type == 'core':
            # Only live processes can be traced unfortunately since we need to
            # call a function on the process. That does not work on cores.
            return

        # should we iterate the inferiors? Probably makes no diff for 99% of apps.
        for thread in gdb.selected_inferior().threads():
            if not thread.is_valid() :
                continue
            thread.switch()
            if gdb.selected_thread() != thread:
                continue # failed to switch :shrug:

            try:
                frame = gdb.newest_frame()
            except gdb.error:
                pass
            while frame:
                ret = qml_trace_frame(frame)
                if ret:
                    self.payload = ret
                    break
                try:
                    frame = frame.older()
                except gdb.error:
                    pass

    def to_sentry_frame(self, frame):
        print(frame)
        blob = {
            'platform': 'other', # always different from the cpp/native frames. alas, technically this frame isn't a javascript frame
            'in_app': True # qml is always in the app I should think
        }
        if 'file' in frame: blob['filename'] = mangle_path(frame['file'])
        if 'func' in frame: blob['function'] = frame['func']
        if 'line' in frame: blob['lineno'] = int(frame['line'])
        return blob

    def to_sentry_frames(self, frames):
        lst = []
        for frame in frames:
            data = self.to_sentry_frame(frame)
            if not data:
                continue
            lst.append(data)
        return lst

    def to_dict(self):
        if not self.payload:
            return None

        payload = self.payload

        from pygdbmi import gdbmiparser
        result = gdbmiparser.parse_response("*stopped," + payload)
        frames = result['payload']['frame']
        print(frames)
        if type(frames) is dict: # single frames traces aren't arrays to make it more fun -.-
            frames = [frames]
        lst = self.to_sentry_frames(frames)
        print(lst)
        if lst:
            return {
                'id': 'QML', # docs say this is typically a number to there is indeed no enforcement it seems
                'name': 'QML',
                'crashed': True,
                'stacktrace': {
                    'frames': self.to_sentry_frames(frames)
                }
            }
        return None

    def to_list(self):
        data = self.to_dict()
        if data:
            return [data]
        return []

class SentryVariablesStatistics:
    # All seen frames
    frames_count: int = 1 # we divide by this, it had better be >0
    # Amount of frames that had local variables
    frames_with_vars: int = 0

    # Only ever consider frames in the crashing thread!
    # The other threads are 99% of the time not interesting.

    def increment_frames():
        if gdb.selected_thread() and gdb.selected_thread() == crashed_thread:
            SentryVariablesStatistics.frames_count = SentryVariablesStatistics.frames_count + 1

    def increment_vars():
        if gdb.selected_thread() and gdb.selected_thread() == crashed_thread:
            SentryVariablesStatistics.frames_with_vars = SentryVariablesStatistics.frames_with_vars + 1

    def vars_rate():
        return round(SentryVariablesStatistics.frames_with_vars / SentryVariablesStatistics.frames_count, 2)

# Only grabing the most local block, technically we could also gather up encompassing scopes but it may be a bit much.
class SentryVariables:
    def __init__(self, frame):
        self.frame = frame
        SentryVariablesStatistics.increment_frames()

    def block(self):
        try:
            return self.frame.block()
        except:
            return None

    def to_dict(self):
        ret = self._to_dict_internal()
        if len(ret) > 0:
            SentryVariablesStatistics.increment_vars()
        return ret

    def _to_dict_internal(self):
        ret = {}
        block = self.block()
        if not block:
            return ret

        for symbol in block:
            try:
                ret[str(symbol)] = str(symbol.value(self.frame))
            except:
                pass # either not a variable or not stringable

        return ret

class SentryFrame:
    def __init__(self, gdb_frame):
        self.frame = gdb_frame
        self.sal = gdb_frame.find_sal()

    def type(self):
        return self.frame.type()

    def filename(self):
        return self.sal.symtab.fullname() if (self.sal and self.sal.symtab) else None

    def lineNumber(self):
        if self.sal.line < 0:
            return None
        # NOTE "The line number of the call, starting at 1." - I'm almost sure gdb starts at 0, so add 1
        return self.sal.line + 1

    def function(self):
        return self.frame.name() or self.frame.function() or None

    def package(self):
        name = gdb.solib_name(self.frame.pc())
        if not name:
            return name
        # NOTE: realpath because neon's gdb is confused over UsrMerge symlinking of /lib to /usr/lib messing up
        # path consistency (mapping data and by extension SentryImage instances use the real path already though
        return os.path.realpath(name)

    def address(self):
        return ('0x%x' % self.frame.pc())

    def to_dict(self, with_vars):
        data = {
            'filename': mangle_path(self.filename()),
            'function': self.function(),
            'package': mangle_path(self.package()),
            'instruction_addr': self.address(),
            'lineno': self.lineNumber(),
        }
        if with_vars:
            data['vars'] = SentryVariables(self.frame).to_dict()
        return data

class SentryRegisters:
    def __init__(self, gdb_frame):
        self.frame = gdb_frame

    def to_dict(self):
        js = {}
        try: # registers() is only available in somewhat new gdbs. (e.g. not ubuntu 20.04)
            for register in self.frame.architecture().registers():
                if register.startswith('ymm'): # ymm actually contains stuff sentry cannot handle. alas :(
                    continue
                value = self.frame.read_register(register).format_string(format='x')
                if value: # may be empty if the value cannot be expressed as hex (happens for extra gdb register magic - 'ymm0' etc)
                    js[register.name] = value
                else:
                    js[register.name] = "0x0"
        except AttributeError:
            return None
        return js

class LockReason:
    def __init__(self, frame: SentryFrame, type, class_name):
        self.type = type
        self.class_name = class_name
        self.thread_id = gdb.selected_thread().ptid[1]

    def to_dict(self):
        return {
            'type': self.type,
            'class_name': self.class_name,
            'thread_id': self.thread_id,
            'package_name': 'java.lang',
        }

    def make(frame):
        # export enum LockType {
        #   LOCKED = 1,
        #   WAITING = 2,
        #   SLEEPING = 4,
        #   BLOCKED = 8,
        # }
        func = frame.function()
        match func:
            case 'QtLinuxFutex::_q_futex':
                return LockReason(frame, 8, 'QtLinuxFutex')
            case ('___pthread_cond_wait', 'pthread_cond_wait'):
                return LockReason(frame, 2, 'pthread_cond_wait')
            case 'QWaitCondition::wait':
                return LockReason(frame, 2, 'QWaitCondition')
        return None

class SentryTrace:
    loaded_solibs = []

    def __init__(self, thread, is_crashed):
        thread.switch()
        self.is_crashed = is_crashed
        self.lock_reasons = {}
        self.was_main_thread = False
        self.crashed = self.is_crashed # different from is_crashed (=input) this indicates if we stumbled over the kcrash handler

    def load_solib(cramped):
        # Lazy load solibs. This is super complicated because the gdb CLI and API don't actually give us all the control.
        # Also loading new symbols resets the trace so we need to select-frames fairly aggressively.

        i = -1

        while True:
            # Check if the next frame even exists
            if i >= 0:
                gdb.execute(f'select-frame {i}')
                if not gdb.selected_frame().older():
                    break

            # Increment in the beginning so we can continue early without having to worry that the index will be off
            i = i + 1

            # Select the actual frame for loading
            gdb.execute(f'select-frame {i}')
            frame = gdb.selected_frame()

            # Determine the solib path
            solib = gdb.current_progspace().solib_name(frame.pc())
            if solib in SentryTrace.loaded_solibs:
                continue
            for core_image in core_images:
                if int(core_image.address, 16) <= frame.pc() < (int(core_image.address, 16) + core_image.length):
                    image = core_image
                    break
            solib = solib or image.file

            if solib in SentryTrace.loaded_solibs:
                continue

            # Actually execute the load
            gdb.execute(f'sharedlibrary {solib}')
            # Make sure we loaded the correct solib (guards against live system updates having removed the correct file)
            objfile = gdb.lookup_objfile(image.build_id, by_build_id=True)
            if objfile.build_id != image.build_id:
                raise UnexpectedMappingException(f"Unexpected mapping for {image.file} ({image.build_id})")
            if cramped:
                # Explicit off in cramped mode even when the user enabled it.
                gdb.execute('set debuginfod enabled off')
            else:
                # (Re)load the symbols
                gdb.execute(f'add-symbol-file {solib}')

            SentryTrace.loaded_solibs.append(solib)

        gdb.execute('select-frame 0')

    def to_dict(self):
        cramped_memory = os.getenv('DRKONQI_MEMORY') == 'cramped' and self.is_crashed
        little_memory = os.getenv('DRKONQI_MEMORY') == 'little' and self.is_crashed
        some_memory = os.getenv('DRKONQI_MEMORY') == 'some'
        spacious_memory = os.getenv('DRKONQI_MEMORY') == 'spacious'
        # In spacious mode we load all solibs by default and don't need to do anything extra. All other modes load on-demand.
        if cramped_memory or little_memory or some_memory:
            SentryTrace.load_solib(cramped_memory)

        frames = [ SentryFrame(frame) for frame in gdb.FrameIterator.FrameIterator(gdb.newest_frame()) ]

        self.lock_reasons = {}
        self.was_main_thread = False

        kcrash_index = -1
        trap_index = -1
        for index, frame in enumerate(frames):
            if frame.function():
                lock_reason = LockReason.make(frame)
                if lock_reason:
                    r = lock_reason.to_dict()
                    address = f'0x{str(len(self.lock_reasons.keys()))}'
                    r['address'] = address
                    self.lock_reasons[address] = r
                if frame.function().startswith('KCrash::defaultCrashHandler'):
                    kcrash_index = index
                    self.crashed = True
                if frame.function().startswith('QCoreApplication::exec'):
                    self.was_main_thread = True
            if frame.type() == gdb.SIGTRAMP_FRAME:
                trap_index = index
        clip_index = max(kcrash_index, trap_index)

        # Throw away kcrash or sigtrap frame, and above. They are useless noise - but only when on the crashing thread.
        if self.is_crashed and clip_index > -1:
            frames = frames[(clip_index + 1):]

        # Sentry format wants oldest frame first.
        frames.reverse()
        data = { 'frames': [ frame.to_dict(with_vars=(some_memory or spacious_memory)) for frame in frames ] }
        if some_memory or spacious_memory:
            data['registers'] = SentryRegisters(gdb.newest_frame()).to_dict()
        return data

class SentryThread:
    def __init__(self, gdb_thread, is_crashed):
        self.thread = gdb_thread
        self.is_crashed = is_crashed

    def to_dict(self):
        # https://develop.sentry.dev/sdk/event-payloads/threads/
        # As per Sentry policy, the thread that crashed with an exception should not have a stack trace,
        #  but instead, the thread_id attribute should be set on the exception and Sentry will connect the two.
        trace = SentryTrace(self.thread, self.is_crashed)
        # NB: trace.to_dict creates members as side effect, run it asap
        payload = {
            'stacktrace': trace.to_dict(),
            'id': self.thread.ptid[1],
            'name': self.thread.name,
            'current': self.is_crashed,
            'crashed': trace.crashed, # side effect
            'main': trace.was_main_thread, # side effect
            'held_locks': trace.lock_reasons, # side effect
        }
        # States appear not documented. They are
        #   RUNNABLE = 'Runnable',
        #   TIMED_WAITING = 'Timed waiting',
        #   BLOCKED = 'Blocked',
        #   WAITING = 'Waiting',
        #   NEW = 'New',
        #   TERMINATED = 'Terminated',
        state = None
        if self.thread.is_exited():
            state = 'Terminated'
        if not state:
            for addr, reason in trace.lock_reasons.items():
                match reason['type']:
                    case 1:
                        state = None # locked doesn't exist as thread state
                    case 2:
                        state = 'Waiting'
                    case 4:
                        state = 'Runnable'
                    case 8:
                        state = 'Blocked'
                break
        payload['state'] = state
        return payload

class SentryImage:
    # This can throw if objfiles fail to resolve!
    def __init__(self, image):
        self.image = image

    def debug_id(self):
        # Identifier of the dynamic library or executable.
        # It is the value of the build_id custom section and must be formatted
        # as UUID truncated to the leading 16 bytes.
        build_id = self.build_id()
        if not build_id:
            raise NoBuildIdException(f'Unexpectedly stumbled over an objfile ({self.file}) without build_id. Not creating payload.')
        truncate_bytes = 16
        build_id = build_id + ("00" * truncate_bytes)
        return str(uuid.UUID(bytes_le=binascii.unhexlify(build_id)[:truncate_bytes]))

    def build_id(self):
        return self.image.build_id

    def to_dict(self):
        # https://develop.sentry.dev/sdk/event-payloads/debugmeta

        return {
            'type': 'elf',
            'image_addr': self.image.address,
            'image_size': self.image.length,
            'debug_id': self.debug_id(),
            # 'debug_file': None, # technically could get this from objfiles somehow but probably not useful cause it can't be used for anything
            'code_id': self.build_id(),
            'code_file': self.image.file,
            # 'image_vmaddr': None, # not available we'd have to read the ELF I think
            'arch': platform.machine(),
        }

def get_stdout(proc, env=None):
    proc = subprocess.run(proc, stdout=subprocess.PIPE, env=env)
    if proc.returncode != 0:
        return ''
    return proc.stdout.decode("utf-8").strip()

class SentryImages:
    def to_list(self):
        ret = []
        for core_image in core_images:
            image = SentryImage(image=core_image)
            ret.append(image.to_dict())
        return ret

class SentryEvent:
    def cpu_model(self):
        with open("/proc/cpuinfo") as f:
            for line in f.readlines():
                key, value = line.split(':', 1)
                if key.strip() == 'model name':
                    return value.strip()
        return None

    def make(self, program, crash_thread):
        crash_signal = int(os.getenv('DRKONQI_SIGNAL'))
        vm = psutil.virtual_memory()
        boot_time = datetime.fromtimestamp(psutil.boot_time()).astimezone(timezone.utc).strftime('%Y-%m-%dT%H:%M:%S')

        # crutch to get the build id. if we did this outside gdb I expect it'd be neater
        progfile = gdb.current_progspace().filename
        build_id = gdb.lookup_objfile(progfile).build_id

        # NOTE: this is run before the other threads because as a side effect it may load symbols that help other threads produce useful output.
        stacktrace = SentryTrace(crash_thread, True).to_dict()

        base_data = json.loads(get_stdout(['drkonqi-sentry-data']))
        sentry_event = { # https://develop.sentry.dev/sdk/event-payloads/
            "debug_meta": { # https://develop.sentry.dev/sdk/event-payloads/debugmeta/
                "images": SentryImages().to_list()
            },
            'threads':  [ # https://develop.sentry.dev/sdk/event-payloads/threads/
                 SentryThread(thread, is_crashed=(thread == crash_thread)).to_dict() for thread in gdb.selected_inferior().threads()
            ] # + SentryQMLThread().to_list(), TODO make qml more efficient it iterates everything again after the sentry threads were collected. a right waste of time!
            ,
            'event_id': uuid.uuid4().hex,
            # Gets overwritten by ReportInterface with a more accurate value
            'timestamp': datetime.now(timezone.utc).isoformat(),
            'message': 'Signal {} in {}'.format(crash_signal, program),
            'platform': 'native',
            'sdk': {
                'name': 'kde.drkonqi.gdb',
                'version': os.getenv('DRKONQI_VERSION'),
             },
            'level': 'fatal',
            # FIXME this is kind of wrong, program ought to be mapped to the project name via our DSNs mapping table (see reportinterface.cpp)
            'release': "{}@unknown".format(program),
            'dist': build_id,
            # TODO environment entry (could be staging for beta releases?)
            'contexts': { # https://develop.sentry.dev/sdk/event-payloads/contexts/
                'device': {
                    'name': base_data['Hostname'],
                    'model': self.cpu_model(),
                    'family': base_data['Chassis'],
                    'simulator': base_data['Virtualization'],
                    'arch': platform.machine(),
                    'memory_size': vm.total,
                    'free_memory': vm.available,
                    'boot_time': boot_time,
                    'timezone': base_data['Timezone'],
                    'processor_count': multiprocessing.cpu_count()
                },
                # 'os' gets injected on the cpp side so it is always available
            },
            'exception': { # https://develop.sentry.dev/sdk/event-payloads/exception/
                'values': [
                    {
                        'value': signal.strsignal(crash_signal),
                        'thread_id': crash_thread.ptid[1],
                        'mechanism': {
                            'type': 'drkonqi',
                            'handled': False,
                            "synthetic": True, # Docs: This flag should be set for all "segfaults"
                            'meta': {
                                'signal': {
                                    'number': crash_signal,
                                    'name': signal.strsignal(crash_signal)
                                },
                            },
                        },
                        'stacktrace': stacktrace,
                    }
                ]
            },
        }

        # WARNING: must be after the trace is constructed so SentryVariables are counted
        sentry_event['tags'] = {
            'binary': program, # for fallthrough we still need a convenient way to identify things
            'stack_vars': 'yes' if (SentryVariablesStatistics.vars_rate() > 0.25) else 'no',
            'stack_vars_rate': str(SentryVariablesStatistics.vars_rate()), # must be str for sentry to consume it properly
        }

        if os.getenv('DRKONQI_APP_VERSION'):
            sentry_event['release'] = '{}@{}'.format(program, os.getenv('DRKONQI_APP_VERSION'))

        return sentry_event

def qml_trace_frame(frame):
    # NB: Super inspired by QtCreator's gdbbridge.py (GPL3).
    # I've made the code less of an eye sore though.

    # This is a very exhaustive attempt at finding a frame that has a symbol to the
    # QV4::ExecutionEngine as we need its address to get the QML trace via qt_v4StackTraceForEngine.
    # Unfortunately there's no shorter way of accomplishing this since the engine isn't necessarily
    # appearing as a frame (consequently we can't easily get to a this pointer).

    try:
        block = frame.block()
    except:
        block = None

    if not block:
        return None

    for symbol in block:
        if not symbol.is_variable and not symbol.is_argument:
            continue

        value = symbol.value(frame)
        if value.is_optimized_out: # can't read values that have been optimized out
            continue

        typeobj = value.type
        if typeobj.code != gdb.TYPE_CODE_PTR:
            continue

        dereferenced_type = typeobj.target().unqualified()
        if dereferenced_type.name != 'QV4::ExecutionEngine':
            continue

        addr = int(value)
        methods = [
            'qt_v4StackTraceForEngine((void*)0x{0:x})',
            'qt_v4StackTrace(((QV4::ExecutionEngine *)0x{0:x})->currentContext())',
            'qt_v4StackTrace(((QV4::ExecutionEngine *)0x{0:x})->currentContext)',
        ]
        for method in methods:
            try: # throws when the function is invalid
                result = str(gdb.parse_and_eval(method.format(addr))).strip()
            except gdb.error:
                continue
            if result:
                # We need to massage the result a bit. It's of the form
                #   "$addr    stack=[...."
                # but we want to drop the addr as it's not useful data and can't get parsed.
                # Also drop the stack nesting. Serves no purpose for us. Also unescape the quotes.
                pos = result.find('"stack=[')
                if pos != -1:
                    result = result[pos + 8:-2]
                    result = result.replace('\\\"', '\"')
                    return result

    return None

def print_qml_frame(frame):
    data = {
        'level': '?',
        'func': '?',
        'file': '?',
        'line': '?'
    }
    data.update(frame)
    print("level={level} func={func} at={file}:{line}".format(**data) )

def print_qml_frames(payload):
    from pygdbmi import gdbmiparser
    response = gdbmiparser.parse_response("*stopped," + payload)
    frames = response['payload']['frame']
    if type(frames) is dict: # single frames traces aren't arrays to make it more fun -.-
        print_qml_frame(frames)
    else: # presumably an iterable
        for frame in frames:
            print_qml_frame(frame)


def print_qml_trace():
    if gdb.selected_inferior().connection.type == 'core':
        # Only live processes can be traced unfortunately since we need to
        # call a function on the process. That does not work on cores.
        print('Cannot QML trace cores :(')
        return

    try:
        from pygdbmi import gdbmiparser
    except ImportError:
        print('Cannot QML trace cores because pygdbmi is missing :(')
        return

    # should we iterate the inferiors? Probably makes no diff for 99% of apps.
    for thread in gdb.selected_inferior().threads():
        if not thread.is_valid():
            continue
        thread.switch()
        if gdb.selected_thread() != thread:
            continue # failed to switch :shrug:

        try:
            frame = gdb.newest_frame()
        except gdb.error:
            pass
        while frame:
            ret = qml_trace_frame(frame)
            if ret:
                header = "____drkonqi_qmltrace_thread:{}____".format(str(thread.num))
                print(frame)
                print(header)
                print_qml_frames(ret)
                print('-' * len(header))
                print("(beware that frames may have been optimized out)")
                print() # separator newline
                break # next thread (there should only be one engine per thread I think?)
            try:
                frame = frame.older()
            except gdb.error:
                pass

def print_kcrash_error_message():
    symbol = gdb.lookup_static_symbol("s_kcrashErrorMessage")
    if not symbol or not symbol.is_valid():
        return

    try:
        value = symbol.value()
    except: # docs say value can throw!
        return
    print("KCRASH_INFO_MESSAGE: Content of s_kcrashErrorMessage: " + value.format_string())
    print() # separator newline

def print_sentry_payload(thread):
    program = os.path.basename(gdb.current_progspace().filename)
    payload = SentryEvent().make(program, thread)

    tmpdir = os.getenv('DRKONQI_TMP_DIR')
    if tmpdir:
        with open(tmpdir + '/sentry_payload.json', mode='w') as tmpfile:
            tmpfile.write(json.dumps(payload))
            tmpfile.flush()

class CoreImage:
    def __init__(self, eu_unstrip_line):
        self.valid = False

        address_pack, build_id_pack, file, debug, self.name = eu_unstrip_line.split(' ', 4)
        self.have_elf = file != '-'
        self.have_dwarf = debug != '-'
        self.build_id, self.build_id_address = build_id_pack.split('@', 1)
        self.address, self.length = address_pack.split('+', 1)
        self.length = int(self.length, 16)
        if self.have_elf:
            # For builtin images the file will be '.'. This notably happens for the executable itself, but also for linux-vdso.
            # For the former we expect the name to be a path and for the latter we don't care since we can't resolve it anyway.
            if file == '.':
                if self.name.startswith('/'):
                    self.file = self.name
                else:
                    return # invalid image (e.g. linux-vdso)
            else:
                self.file = file

        self.valid = True

def resolve_modules():
    corefile = os.getenv("DRKONQI_COREFILE")
    if not corefile:
        raise RuntimeError("No corefile found. Cannot resolve modules.")

    global core_images

    env = os.environ.copy()
    env.pop('DEBUGINFOD_URLS', None) # we don't want debug info downloads, we'll do them from gdb if necessary
    # Beware that build ids from eu-unstrip are a bit unreliable in that we get back the current build id in case the
    # core doesn't contain one. That makes the ids a bit unreliable but still better than nothing I suppose.
    # Ultimately we'll want to use gdb here.
    # https://sourceware.org/bugzilla/show_bug.cgi?id=32844
    output = get_stdout(['eu-unstrip', "--list-only", f"--core={corefile}"], env=env)
    for line in output.splitlines():
        image = CoreImage(line)
        if image.valid:
            core_images.append(image)

def print_preamble_internal():
    resolve_modules()

    thread = gdb.selected_thread()
    if thread == None:
        # Can happen when e.g. the core is missing or not readable etc. We basically aren't debugging anything
        return
    if 'sentry_sdk' in globals():
        sentry_sdk.add_breadcrumb(
            category='debug',
            level='debug',
            message=f'Selected thread {thread}',
        )
    global crashed_thread
    crashed_thread = thread
    # run this first as it expects the current frame to be the crashing one and qml tracing changes the frames around
    print_kcrash_error_message()
    # changes current frame and thread!
    print_qml_trace()
    # prints sentry report
    try:
        print_sentry_payload(thread)
    except NoBuildIdException as e:
        # TODO should this get re-raised
        traceback.print_exc()
        print(e)
        pass

def print_preamble():
    try:
        print_preamble_internal()
    except Exception as e:
        if 'sentry_sdk' in globals():
            sentry_sdk.capture_exception(e)
        traceback.print_exc()
        print(e)
        gdb.execute('quit 1')

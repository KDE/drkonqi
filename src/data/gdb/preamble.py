# SPDX-License-Identifier: GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
# SPDX-FileCopyrightText: 2021 Harald Sitter <sitter@kde.org>

import gdb
from gdb.FrameDecorator import FrameDecorator

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
                result = str(gdb.parse_and_eval(method.format(addr)))
            except:
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
    print("level={level} func={func} at={file}:{line}".format(**frame) )

def print_qml_frames(payload):
    try: # try pretty printing via pygdbmi. If it is not available print verbatim.
        from pygdbmi import gdbmiparser
        response = gdbmiparser.parse_response("*stopped," + payload)
        frames = response['payload']['frame']
        if type(frames) is dict: # single frames traces aren't arrays to make it more fun -.-
            print_qml_frame(frames)
        else: # presumably an iterable
            for frame in frames:
                print_qml_frame(frame)
    except Exception as e:
        print("Failed to do pygdbmi parsing: {}".format(str(e)))
        print(payload)


def print_qml_trace():
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
                header = "____drkonqi_qmltrace_thread:{}____".format(str(thread.num))
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

def print_preamble():
    print("__preamble__")
    # run this first as it expects the current frame to be the crashing one and qml tracing changes the frames around
    print_kcrash_error_message()
    # changes current frame and thread!
    print_qml_trace()

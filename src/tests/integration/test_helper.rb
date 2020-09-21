# SPDX-FileCopyrightText: 2017 Harald Sitter <sitter@kde.org>
#
# SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL

STDOUT.sync = true # force immediate flushing without internal caching

DRKONQI_PATH = ENV['DRKONQI_PATH']
AT_SPI_BUS_LAUNCHER_PATH = ENV['AT_SPI_BUS_LAUNCHER_PATH']
AT_SPI_REGISTRY_PATH = ENV['AT_SPI_REGISTRY_PATH']
warn "Testing against #{DRKONQI_PATH} with #{AT_SPI_BUS_LAUNCHER_PATH} " \
     " and #{AT_SPI_REGISTRY_PATH}"

# Only set inside the test to prevent dbus activation of supporting services.
# We'll force a11y here as depending on the distribution a11y may not be enabled
# by default.
ENV['QT_ACCESSIBILITY'] = '1'
ENV['QT_LINUX_ACCESSIBILITY_ALWAYS_ON'] = '1'

# We kill these after our test run. When isolated they would die with our
# bus, on CI systems we employ no isolation and instead need to manage them
# manually.
# NB: do not give additional options to the launcher. Ubuntu broke theirs
#   causing crashes...
launcher_pid = spawn(AT_SPI_BUS_LAUNCHER_PATH, '--launch-immediately')
registry_pid = spawn(AT_SPI_REGISTRY_PATH)

require 'atspi'
require 'minitest/autorun'

# Adds convenience methods for ATSPI on top of minitest.
class ATSPITest < Minitest::Test
  def find_in(parent, name: nil, recursion: false)
    raise 'no accessible' if parent.nil?
    accessibles = parent.children.collect do |child|
      ret = []
      if child.children.size != 0 # recurse
        ret += find_in(child, name: name, recursion: true)
      end
      if name && child.states.include?(:showing)
        if (name.is_a?(Regexp) && child.name.match(name)) ||
           (name.is_a?(String) && child.name == name)
          ret << child
        end
      end
      ret
    end.compact.uniq.flatten
    return accessibles if recursion
    raise "not exactly one accessible for #{name} => #{accessibles.collect {|x| x.name}.join(', ')}" if accessibles.size > 1
    raise "cannot find accessible(#{name})" if accessibles.size < 1
    yield accessibles[0] if block_given?
    accessibles[0]
  end

  def press(accessible)
    raise 'no accessible' if accessible.nil?
    action = accessible.actions.find { |x| x.name == 'Press' }
    refute_nil action, 'expected accessible to be pressable'
    action.do_it!
    sleep 0.25
  end

  def focus(accessible)
    raise 'no accessible' if accessible.nil?
    action = accessible.actions.find { |x| x.name == 'SetFocus' }
    refute_nil action, 'expected accessible to be focusable'
    action.do_it!
    sleep 0.1
  end

  def toggle(accessible)
    raise 'no accessible' if accessible.nil?
    action = accessible.actions.find { |x| x.name == 'Toggle' }
    refute_nil action, 'expected accessible to be toggle'
    action.do_it!
    sleep 0.1
  end

  def toggle_on(accessible)
    raise 'no accessible' if accessible.nil?
    return if accessible.states.any? { |x| %i[checked selected].include?(x) }
    toggle(accessible)
  end
end

Minitest.after_run do
  Process.kill('KILL', launcher_pid)
  Process.kill('KILL', registry_pid)
end

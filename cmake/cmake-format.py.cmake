# DO NOT CHANGE
# -- CMAKE_FORMAT_MODULE_MARKER --
# DO NOT CHANGE

# SPDX-License-Identifier: CC0-1.0
# SPDX-FileCopyrightText: none

# ----------------------------------
# Options affecting listfile parsing
# ----------------------------------
with section("parse"):

  additional_commands = @_CMAKE_FORMAT_ADDITIONAL_COMMANDS@

  # Specify variable tags.
  vartags = [
    (".*_SRCS", ["file-list"]),
  ]

# -----------------------------
# Options affecting formatting.
# -----------------------------
with section("format"):

  # Disable formatting entirely, making cmake-format a no-op
  disable = False

  # How wide to allow formatted cmake files
  # TODO: I'm going with 120 here because cmake lines are often harder on the eyes than cpp
  line_width = 120

  # How many spaces to tab for indent
  tab_size = 4

  # If true, lines are indented using tab characters (utf-8 0x09) instead of
  # <tab_size> space characters (utf-8 0x20). In cases where the layout would
  # require a fractional tab character, the behavior of the  fractional
  # indentation is governed by <fractional_tab_policy>
  use_tabchars = False

  # If a statement is wrapped to more than one line, than dangle the closing
  # parenthesis on its own line.
  # TODO dangle is sometimes nice but on the balance it is probably garbage more often than not?
  dangle_parens = False

  # If the statement spelling length (including space and parenthesis) is
  # smaller than this amount, then force reject nested layouts.
  #
  # 8 is a reasonable sweet spot with install() still being fairly OK.
  min_prefix_chars = 8

  # If the statement spelling length (including space and parenthesis) is larger
  # than the tab width by more than this amount, then force reject un-nested
  # layouts.
  #
  # always force nesting it "feels" better, e.g. long find_package calls
  max_prefix_chars = 0

  # If a candidate layout is wrapped horizontally but it exceeds this many
  # lines, then reject the layout.
  # TODO greater might be better here
  max_lines_hwrap = 2

# SPDX-License-Identifier: BSD-2-Clause
# SPDX-FileCopyrightText: 2025 Harald Sitter <sitter@kde.org>

# Core library ideally only depending on Qt::Core and KCoreAddons.
# Place to put helper code that is used throughout the stack. Both our CoreApps and GUIApps.
add_library(drkonqi-core STATIC metadata.cpp drkonqipaths.cpp)
target_link_libraries(drkonqi-core PUBLIC Qt::Core)
target_compile_options(
    drkonqi-core
    PRIVATE
        -DKDE_INSTALL_FULL_LIBEXECDIR=\"${KDE_INSTALL_FULL_LIBEXECDIR}\"
)

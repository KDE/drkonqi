# SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
# SPDX-FileCopyrightText: 2023 Harald Sitter <sitter@kde.org>

[Unit]
Description=Consume pending crashes using DrKonqi
PartOf=graphical-session.target
Requires=drkonqi-coredump-launcher.socket
After=plasma-core.target
After=drkonqi-coredump-launcher.socket

[Service]
# wait until system has hopefully settled down from login
ExecStartPre=/usr/bin/sleep 60
ExecStart=@KDE_INSTALL_FULL_LIBEXECDIR@/drkonqi-coredump-processor --pickup --uid %U
RuntimeMaxSec=31 minutes

[Install]
WantedBy=plasma-core.target

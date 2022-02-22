<!--
    SPDX-License-Identifier: CC0-1.0
    SPDX-FileCopyrightText: 2021-2022 Harald Sitter <sitter@kde.org>
    SPDX-FileCopyrightText: 2021 Aleix Pol i Gonzalez <aleixpol@kde.org>
-->

# DrKonqi
## Activating the debug button for DrKonqi
Add into `~/.config/drkonqirc`:
```
[DrKonqi]
ShowDebugButton=true
```

## Integrating with coredumpd
We need to make sure that the `drkonqi-coredump-processor@.service` is enabled so that coredumpd knows to tell us when a crash happens. You can inspect it with:

* Check it's enabled: `systemctl is-enabled drkonqi-coredump-processor@.service`
* You can enable it with `systemctl enable drkonqi-coredump-processor@.service`

To make sure the coredumpd setup is working correctly:
* `cat /proc/sys/kernel/core_pattern` should show systemd-coredump as handler
* `KCRASH_DUMP_ONLY=1 KDE_COREDUMP_NOTIFY=1 kwrite`
* killall -SEGV kwrite
* journalctl sould show the following:
  * systemd-coredump@.service run an instance (e.g. systemd-coredump@12-66048-0.service)
  * drkonqi-coredump-processor@.service should run an instance with same suffix as systemd-coredump@
  * drkonqi-coredump-launcher@.service should run an instance with yet the same suffix
  * at this point a notification should show up that notifies you about the crash

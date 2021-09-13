<!--
    SPDX-License-Identifier: CC0-1.0
    SPDX-FileCopyrightText: 2021 Harald Sitter <sitter@kde.org>
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

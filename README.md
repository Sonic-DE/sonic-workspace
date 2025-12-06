# Plasma Workspace

Plasma Workspace is used as the base for Plasma Desktop, Mobile, and Bigscreen.
It contains shared KCMs, applets as well as multiple libraries.

## Contacting the SonicDE development team

<img src="https://github.com/Sonic-DE/.github/tree/main/profile/icons/bluesky.svg">&nbsp;[Bluesky](https://bsky.app/profile/sonicdesktop.bsky.social)&nbsp; <img src="https://github.com/Sonic-DE/.github/tree/main/profile/icons/discord.svg">&nbsp;[Discord](https://discord.gg/cNZMQ62u5S) &nbsp; <img src="https://github.com/Sonic-DE/.github/tree/main/profile/icons/mastodon.svg">&nbsp;[Mastodon](https://mastodon.social/@sonicdesktop) &nbsp; <img src="https://github.com/Sonic-DE/.github/tree/main/profile/icons/oftc.svg">&nbsp;[OFTC IRC](https://webchat.oftc.net/?channels=sonicde%2Csonicde-devel%2Csonicde-dist&uio=MT11bmRlZmluZWQb1) &nbsp; <img src="https://github.com/Sonic-DE/.github/tree/main/profile/icons/matrix.svg">&nbsp;[Matrix](https://matrix.to/#/#sonicdesktop:matrix.org) &nbsp; <img src="https://github.com/Sonic-DE/.github/tree/main/profile/icons/telegram.svg">&nbsp;[Telegram](https://t.me/sonic_de) &nbsp; <img src="https://github.com/Sonic-DE/.github/tree/main/profile/icons/x.svg">&nbsp;[X (Twitter)](https://x.com/SonicDesktop)

### TaskManager Library

The Task Manager provides various QAbstractListModel-based model for listing
Windows (`TaskManager::AbstractWindowTasksModel`), Startup tasks (`TaskManager::StartupTasksModel`) and Launcher
Job (`TaskManager::LauncherTasksModel`).

### Workspace Library

libkworkspace provides functions to allow you to interact with the
**KDE** session manager (`SessionManagement`).

### Notification Manager Library

libnotificationmanager is responsible for listing notifications, closing them
and interacting with them in Plasma. This class provides a **Qt** model for jobs:
`NotificationManager::JobsModel`. As well as a **Qt** model for notifications and
jobs: `NotificationManager::Notifications`.

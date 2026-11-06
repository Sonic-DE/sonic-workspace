# Sonic Workspace Release Notes

Sonic Workspace is the heart of the SonicDE desktop: it provides the desktop shell, panels, widgets, notifications, clipboard, login and lock screen look, and many of the system settings pages.

## Unreleased

- The desktop shell is now called Sonic Shell, and the desktop's apps and services carry SonicDE names throughout. Your existing panels, widgets, settings and shortcuts move over automatically.
- When the on-screen keyboard is open on the login or lock screen, your picture stays visible and the picture and password field sit centered above the keyboard.
- The blurred background on the login and lock screens now reaches all the way to the screen edges, with no gap around the borders.
- The darkened background behind the login and lock screens now picks up the colors of your chosen theme.
- Font previews show up properly again in the file manager, and fixed a crash while those previews were being created.
- A panel's keyboard shortcut now opens and closes an auto-hiding panel right away, and a panel keeps the keyboard while one of its menus or dialogs is open. Escape now goes to the menu or widget using it before it closes the panel.
- Behind the scenes: new automated tests for the on-screen keyboard, and the desktop session uses QXLibre for X11 when it's installed, going back to Qt's own X11 support when it isn't. Panels can also leave their background out behind part of a widget.

## 6.8.0.4 (2026-10-24)

- The darkened background behind the login and lock screens now picks up the colors of your chosen theme.

## 6.8.0.3 (2026-10-23)

- Font previews show up properly again in the file manager, and fixed a crash while those previews were being created.

## 6.8.0.2 (2026-10-22)

- The blurred background on the login and lock screens now reaches all the way to the screen edges, with no gap around the borders.
- Behind the scenes: new automated tests for the on-screen keyboard.

## 6.8.0.1 (2026-10-21)

- When the on-screen keyboard is open on the login or lock screen, your picture stays visible and the picture and password field sit centered above the keyboard.

## 6.8.0 (2026-10-20)

- Do Not Disturb can now switch on by itself during a time range you choose, so notifications stay quiet while you sleep.
- The login and lock screens get a new built-in on-screen keyboard, so you can sign in without a physical keyboard.
- Clipboard settings are simpler: choose whether to save text you only select, and whether to save copied images, in your clipboard history. Your previous choices carry over.
- The selected item in the clipboard history is readable again; its text no longer disappears.
- You can now turn the splash screen off by choosing "None".
- New users get the full SonicDE look from their very first login.
- Fixed the task manager's handling of window order with the latest Qt release.
- Behind the scenes: release preparation and updated dependency versions.

## 6.7.5.4 (2026-10-07)

- New users get the full SonicDE look from their very first login.

## 6.7.5.3 (2026-09-28)

- The selected item in the clipboard history is readable again; its text no longer disappears.

## 6.7.5.2 (2026-09-23)

- You can now turn the splash screen off by choosing "None".

## 6.7.5.1 (2026-09-14)

- Fixed the task manager's handling of window order with the latest Qt release.

## 6.7.5 (2026-09-12)

- The media player widget, notification previews, clipboard and wallpaper blur effects no longer need older compatibility libraries, so SonicDE installs lighter.
- Behind the scenes: release preparation.

## 6.7.4.7 (2026-08-31)

- The welcome text now reads "Welcome to SonicDE".

## 6.7.4.6 (2026-08-21)

- The "Get New" download window in the Global Theme settings now stays properly attached to the settings window.
- Behind the scenes: a small comment correction.

## 6.7.4.5 (2026-08-20)

- Fixed a possible panel crash when a screen is switched off or disconnected.

## 6.7.4.4 (2026-08-12)

- Your desktop session now starts Sonic Win, SonicDE's own window manager.

## 6.7.4.3 (2026-08-12)

- Fixed a desktop crash that could happen when the computer enters power saving.

## 6.7.4.2 (2026-08-11)

- Fixed a crash in the font tools when they run without a display.

## 6.7.4.1 (2026-08-11)

- Your login screen now lists the session as "SonicDE (Sonic Desktop Environment)", and outdated Wayland session entries are gone.

## 6.7.4 (2026-08-04)

No SonicDE-specific changes in this release; it brings the package up to date with upstream.

## 6.7.3.3 (2026-08-04)

- More of the desktop now uses the Silver look, picking up pieces of the old Breeze theme that were missed before.

## 6.7.3.2 (2026-08-02)

- The SonicDE logo now appears wherever the KDE logo used to.

## 6.7.3.1 (2026-08-02)

- Silver is now the default look of the desktop, replacing Breeze.
- Hidden themes no longer clutter the desktop theme picker in settings.
- Behind the scenes: shared code formatting settings.

## 6.7.3 (2026-07-14)

- Behind the scenes: code formatting rules.

## 6.7.2 (2026-06-29)

No SonicDE-specific changes in this release; it brings the package up to date with upstream.

## 6.7.1 (2026-06-23)

No SonicDE-specific changes in this release; it brings the package up to date with upstream.

## 6.7.0 (2026-06-21)

- The logout screen and the Region & Language settings no longer try to install software themselves; system updates and language packs are left to your distribution's own tools.
- The last Wayland-only leftovers are gone, including an on-screen keyboard option that never worked on SonicDE's X11 desktop.
- Behind the scenes: build warning fixes, removal of outdated tests and comments, and general code tidying.

## 6.6.5.1 (2026-06-16)

No SonicDE-specific changes in this release; it brings the package up to date with upstream.

## 6.6.5 (2026-05-14)

No SonicDE-specific changes in this release; it brings the package up to date with upstream.

## 6.6.4.1 (2026-05-07)

- The last Wayland-only leftovers are gone, including an on-screen keyboard option that never worked on SonicDE's X11 desktop.
- Behind the scenes: removal of outdated tests and comments.

## 6.6.4 (2026-04-07)

- The logout screen and the Region & Language settings no longer try to install software themselves; system updates and language packs are left to your distribution's own tools.
- Application menu entries set to show only the app name now display correctly.
- Behind the scenes: consistent formatting across the code and files, build warning fixes, optional test tools, and general code tidying.

## 6.6.3 (2026-03-17)

No SonicDE-specific changes in this release; it brings the package up to date with upstream.

## 6.6.2.1 (2026-03-15)

- SonicDE now focuses fully on the X11 desktop: Wayland-only parts have been removed from the panels, system tray, notifications, clipboard, task manager, app menu, search, splash screen, settings and more, making the desktop leaner.
- The login screen no longer offers a Wayland session that SonicDE doesn't support.
- Behind the scenes: removal of Wayland-only tests and build requirements, and README updates.

## 6.6.2 (2026-02-22)

- Sonic Workspace now works with Sonic Win, SonicDE's window manager.
- Panels and desktops stay on the right screens more reliably when monitors are connected or disconnected.
- Behind the scenes: README and contact updates, developer tool support, and a lighter test setup.

## 6.6.1 (2026-02-22)

- Sonic Workspace now works with Sonic Win, SonicDE's window manager.
- Panels and desktops stay on the right screens more reliably when monitors are connected or disconnected.
- Behind the scenes: README and contact updates, developer tool support, and a lighter test setup.

## 6.6.0.2 (2026-02-22)

- Sonic Workspace now works with Sonic Win, SonicDE's window manager.

## 6.6.0.1 (2026-02-18)

- Panels and desktops stay on the right screens more reliably when monitors are connected or disconnected.
- Behind the scenes: README and contact updates, developer tool support, and a lighter test setup.

## 6.5.5.1 (2026-01-14)

- Panels and desktops stay on the right screens more reliably when monitors are connected or disconnected.
- Behind the scenes: README and contact updates, a lighter test setup, groundwork for on-screen keyboard support, and build tidying for the keyboard layout feature.

## 6.5.5 (2025-12-06)

- Panels and desktops stay on the right screens more reliably when monitors are connected or disconnected.
- Behind the scenes: README and contact updates, and a lighter test setup.

## 6.5.4 (2025-12-09)

No SonicDE-specific changes in this release; it brings the package up to date with upstream.

## 6.5.3.6 (2025-12-07)

No SonicDE-specific changes in this release; it brings the package up to date with upstream.

## 6.5.3.5 (2025-12-07)

- Behind the scenes: README, contact and icon link updates.

## 6.5.3.4 (2025-11-30)

- Behind the scenes: a lighter test setup and build housekeeping.

## 6.5.3.3 (2025-11-28)

No SonicDE-specific changes in this release; it brings the package up to date with upstream.

## 6.5.3.2 (2025-11-27)

No SonicDE-specific changes in this release; it brings the package up to date with upstream.

## 6.5.3 (2025-11-21)

- Panels and desktops stay on the right screens more reliably when monitors are connected or disconnected.
- Behind the scenes: README formatting.

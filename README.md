# Nothing X (desktop)

An unofficial, free desktop companion for Nothing earbuds: the controls of
the Nothing X phone app, as a tray popup on **Omarchy** (Hyprland) and
**Windows**. Both are built from the same source and QML, so they look and
behave the same, and they update to the same release.

> **Unofficial, independent and not for profit.** This project is not made,
> endorsed or supported by Nothing Technology Limited. "Nothing", "Nothing X"
> and the product names are Nothing's trademarks, used only to say which
> earbuds this works with. See the [legal notice](#legal-notice).

Made to work the way the official Nothing X app does, so it can talk to
earbuds you own:
- **Commands and settings.** The control commands and setting rules follow
  the official Android app (v3.10.3), studied for interoperability rather
  than guessed. That includes the frame format, reply status codes, LDAC and
  low-latency values, and the dual-connection commands.
- **Look.** It uses Nothing's own fonts (NType 82, Ndot, Geist), icons and
  product photos. These are imported at runtime from your copy of the official
  app, so none of Nothing's files are in this repository.
- **Features per model.** They come from the official app's per-model
  configuration. Importing a newer official app brings its devices, options
  and artwork along.

Made for the **Nothing Ear (3)**.

## What it does

| Official Nothing X | Here |
|---|---|
| Battery: left, right, case, charging | Home, the tray icon (a dot-matrix number, rounded to 5%) and its tooltip |
| Noise control: Noise Cancellation (High, Mid, Low, Adaptive), Transparency, Off | Home, and the tray's right-click menu |
| Equaliser: Balanced, More bass, More treble, Voice, Custom (bass, mid, treble) | Equaliser › Simple |
| Advanced equaliser: up to 8 points with frequency, gain and Q, profiles | Equaliser › Advanced |
| Spatial audio: Off, Fixed, Head-tracking | Home |
| Bass Enhancement, 5 levels | Home |
| Personal Sound Profile on/off | Home |
| Controls: every pinch gesture per earbud, the noise-control cycle, the case's TALK button | Controls |
| Super Mic, walkie-talkie mode, auto-transparency during calls | Super Mic |
| In-ear detection, low-latency mode | Device Settings |
| High-quality audio (AAC / LDAC) | Device Settings |
| Dual Connection, and connecting or disconnecting the other device | Device Settings |
| Ear tip fit test | Home |
| Find my earbuds (ring left or right) | Home |
| Firmware versions (earbuds and case), serial number, colour | Device details |
| News Reporter gesture (Nothing phones only) | **Desktop assistant**: set any gesture to it and it runs your assistant command (App settings › Assistant), no phone involved |

The app also applies the official app's rules between settings. For example,
turning on spatial audio turns off LDAC, Adaptive ANC and (on the Ear (3))
Bass Enhancement and the advanced equaliser. It asks first, using the official
app's wording.

These still need the phone app:
- **Firmware updates** need Nothing's servers.
- **Creating a Personal Sound Profile** runs a hearing test in a closed SDK.
  The desktop app turns an existing profile on and off.
- **The online EQ library.**
- **Essential Space** works with Nothing phones only.

The earbuds' own "Voice assistant" gesture asks for the phone's voice dialling,
which a computer doesn't pass on to apps. Use **Desktop assistant** instead.
With App settings › Assistant left empty it runs
Wumpy (`wumpy --voice`) if it is installed
where its installer puts it, on Omarchy or Windows. Anything typed there runs
instead, as it would at a prompt: through `sh` on Omarchy and `cmd` on
Windows, where a quoted path with spaces works as typed.

## Install on Omarchy

1. Pair the earbuds once: open the case, hold its button for 2 seconds, and pair
   them from the bar's Bluetooth menu.
2. Put the official Nothing X app (`.xapk` or `.apk`) in this folder or in
   Downloads.
3. Build, install and import:

   ```sh
   bin/install
   ```

Nothing X then starts with the computer and lives in the tray. Open it from
the tray or the launcher (Super + Space).

## Install on Windows

Pair the earbuds in Settings › Bluetooth & devices, then run
`NothingX-Setup-<version>.exe` from the
[releases](https://github.com/psyduck-z/Nothing-X-Desktop/releases). The first
time, choose the official Nothing X app under App settings, or drop it on the
popup.

## Updates

- **This app:** every tagged release builds the Windows installer and the
  Arch package from the same commit, so both platforms run the same version.
  The app checks twice a day and installs with one click: quietly on Windows,
  and through the system password prompt on Omarchy.
- **The official app:** put a newer Nothing X `.xapk` or `.apk` in Downloads
  (or the Desktop, Documents, or home folder). It is imported on its own,
  bringing new devices, options and artwork. You can also choose it under App
  settings.

## Command line

| Command | Does |
|---|---|
| `nothingx` | Start in the tray and open the popup (or toggle it if running) |
| `nothingx --hidden` | Start in the tray only (used at login) |
| `nothingx --toggle` / `--show` / `--hide` / `--quit` | Control the running instance |
| `nothingx --import-official FILE` | Import the official app's look and data |
| `nothingx --demo` | Simulated Ear (3), no Bluetooth needed |

Esc goes back or closes the popup; Ctrl+Q quits.

## Troubleshooting

- **"Couldn't reach the earbuds"**: take them out of the case. Only one app at
  a time can hold the earbuds' control channel, so close Nothing X on the phone
  for a moment.
- **A setting doesn't stick**: the earbuds answer every change with a status.
  If they refuse one, the app says so and shows the real value again.

## How it works

The app speaks Nothing's control protocol over Bluetooth RFCOMM (service UUID
`aeac4a03-dff5-498f-843a-34487cf133eb`):

```
55 | control 0x0160 | command | length | sequence | payload | CRC-16/MODBUS
```

All fields are little-endian. Replies carry the request's command with
bit 15 cleared and a status code in the control word.

```
src/protocol.*       framing, CRC, every command's payload (pure, tested byte for byte)
src/device.*         the earbuds' state, the command queue, the official rules between settings
src/modelconfig.*    what a model can do, from the official per-model configuration
src/officialpack.*   imports Nothing's fonts, icons and data from your copy of the official app
src/updater.*        updates from this project's GitHub releases
src/bttransport.*    RFCOMM via Qt Bluetooth (BlueZ on Linux, WinRT on Windows)
src/demotransport.*  a simulated Ear (3) for --demo and the tests
src/popupplacer.*    the tray flyout: layer-shell on Wayland, docked to the taskbar on Windows
src/qml/             the pages
```

**Unconfirmed from the official code.** A few Ear (3) commands exist only in
the official app's compiled Flutter code, which can't be read. These are
Super Mic, walkie-talkie, auto-transparency, the fit test and Personal Sound.
They use codes captured from the official app by the community. Their command
numbers do appear in the official app's compiled command tables.

## Development

```sh
bin/build                                # build/nothingx
bin/test                                 # protocol, device and import tests, offscreen
build/nothingx --demo                    # try the UI without earbuds
build/nothingx --screenshots /tmp/shots  # render every page to PNG
```

Versions follow the official app: `3.10.3.1` is the first desktop build of
Nothing X 3.10.3. Release by bumping `VERSION` in `nothingx.pro` and pushing
a tag `v<version>`; CI publishes both platforms.

## Credits

The community's reverse-engineering work came first, and was checked here
against the official app:
[ear-web](https://github.com/radiance-project/ear-web),
[nothingctl](https://github.com/FormalSnake/nothingctl),
[PairFree](https://github.com/Omega-Destroyer/PairFree),
[BudsLink](https://github.com/maniacx/BudsLink),
[Gadgetbridge](https://codeberg.org/Freeyourgadget/Gadgetbridge),
[EarA-linux](https://github.com/oneydef/EarA-linux). This app copies no code
from them.

## Legal notice

- **Unofficial.** This is an independent hobby project. It is not made,
  sponsored, endorsed, approved or supported by Nothing Technology Limited or
  its affiliates. For help with your earbuds, use Nothing's official app and
  support.
- **Not for profit.** It is free and open source. It is not sold, carries no
  advertising, and has no commercial purpose.
- **Trademarks.** "Nothing", "Nothing X", "Ear (3)", "Essential Space" and the
  related names and logos are trademarks of Nothing Technology Limited. They
  are used here only to describe which products this works with. That use
  does not imply any connection with Nothing or endorsement by it.
- **Credit to Nothing.** The design this app follows is Nothing's: the
  dot-matrix look, how the controls are laid out and explained, and some of
  the on-screen wording, which is kept the same as the official app's so the
  two read alike. The app icon was drawn for this project in that style. Full
  credit for that design goes to Nothing Technology Limited; any mistakes are
  this project's own.
- **No Nothing files in this repository.** It contains no fonts, icons,
  images, firmware or app files from Nothing, and none of its code. A few
  short labels and messages are worded the same as the official app's (see
  the credit above). The app reads its look and device data from a copy of
  the official app that you supply on your own computer, and nothing from
  that copy is uploaded or shared. You are responsible for obtaining the
  official app lawfully and for following its terms.
- **Interoperability only.** The Bluetooth control protocol was studied only
  so this app can work with earbuds the user owns, alongside the open-source
  community work credited above. It does not use Nothing's servers, does not
  modify or distribute firmware, and does not bypass any copy protection.
- **No warranty.** It is provided "as is", without warranty of any kind,
  under the Apache License 2.0. Use it at your own risk. The authors are not
  liable for any damage to devices, data or anything else.
- **Requests from Nothing.** If Nothing Technology Limited objects to anything
  here, open an issue and it will be changed or taken down promptly.

## License

Apache-2.0, see [LICENSE](LICENSE). Nothing's fonts, icons, images and app are
Nothing's and are not part of this repository.

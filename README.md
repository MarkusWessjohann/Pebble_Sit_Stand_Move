# sit_stand_move

A Pebble watchapp/watchface written in C using the Pebble SDK.

## Building & running

```sh
uv run --locked pebble build                          # build using the project's locked CLI
uv run --locked pebble install --emulator emery       # install on the emery emulator
uv run --locked pebble install --phone <ip>           # install to a paired phone
```

The lockfile fixes the project's Python CLI dependency set to Pebble Tool 5.0.40 and its transitive Python dependencies; `.python-version` selects Python 3.13. Use `uv run --locked pebble ...` so builds and installs use that project environment instead of a separately installed global CLI (the failing global CLI traceback was running from Python 3.15). The Pebble C SDK is installed separately by the host Pebble SDK setup; see the [platform verification report](_bmad-output/implementation-artifacts/platform-verification.md) for recording the SDK selected by the build. The owner reports SDK 4.33.1, which still needs confirmation.

## Target platforms

`targetPlatforms` in `package.json` controls which watches you build for. The
modern Pebble hardware is **emery** (Pebble Time 2), **gabbro** (Pebble Round
2), and **flint** (Pebble 2 Duo); the original Pebble platforms (aplite,
basalt, chalk, diorite) are included by default for backwards compatibility.

## Project layout

```
src/c/           C source for the watchapp
src/pkjs/        PebbleKit JS (phone-side) source, if any
worker_src/c/    Background worker source, if any
resources/       Images, fonts, and other bundled resources
package.json     Project metadata (UUID, platforms, resources, message keys)
wscript          Build rules — usually no need to edit
```

By default this project is configured as a watchapp. To make it a watchface,
set `pebble.watchapp.watchface` to `true` in `package.json`.

## Documentation

Full SDK docs, tutorials, and API reference: <https://developer.repebble.com>

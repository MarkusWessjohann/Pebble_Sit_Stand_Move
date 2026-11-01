# sit_stand_move

A Pebble watchapp/watchface written in C using the Pebble SDK.

## Building & running

```sh
uv run --locked python scripts/pebble.py build                          # build using the pinned CLI and active SDK
uv run --locked python scripts/pebble.py install --emulator emery       # install on the emery emulator
uv run --locked python scripts/pebble.py install --phone <ip>           # install to a paired phone
```

The lockfile fixes the project's Python CLI dependency set to Pebble Tool 5.0.40 and its transitive Python dependencies; `.python-version` pins Python 3.13.5. `.pebble-sdk-version` pins the active host C SDK to 4.33.1. The wrapper checks `pebble sdk list` and refuses to build/install unless the project-pinned SDK is active, then runs the command through the locked CLI. The C SDK is installed separately by the host Pebble SDK manager; activate the pinned version there before using these commands. In `package.json`, `sdkVersion: "3"` remains the manifest API level, not the C SDK package version.

## Tests

```sh
sh scripts/test.sh    # host-side C coordinator tests (Pebble API stubs) and Python wrapper tests
```

These do not exercise the Pebble SDK build, the menu UI, or watch firmware.

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

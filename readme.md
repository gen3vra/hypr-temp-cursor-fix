# cursor-fix

Small Hyprland plugin to work around cursor/focus edge-cases where the compositor
doesn't re-evaluate the surface under the pointer. The plugin forces a cursor
reset (or simulates a small mouse movement) periodically so the cursor shape
and focus behave correctly after events like workspace switches or window
closures. The plugin is hacky but it calms my nerves until upstream fixes

## Symptoms addressed
- Cursor shape doesn't change when a window is closed and the desktop is left under the cursor.
- Scroll/input no-ops after switching workspaces in some `follow_mouse` setups.
- Cursor shape not updating on workspace switch when pointing at empty desktop space.

## Requirements
- Hyprland with plugin support and `hyprctl` available in your PATH
- Build dependency (Debian/Ubuntu):

```sh
sudo apt install libpixman-1-dev
```

## Build

```sh
make
```

## Manual load / unload

Load the plugin from the repository root:

```sh
hyprctl plugin load $(pwd)/cursor-fix.so
```

Unload:

```sh
hyprctl plugin unload /full/path/to/cursor-fix.so
```

Quick rebuild + reload:

```sh
make && hyprctl plugin unload $(pwd)/cursor-fix.so; hyprctl plugin load $(pwd)/cursor-fix.so
```

## Autoload on Hyprland start
To load the plugin automatically when Hyprland starts, add an `exec-once`
command to your Hyprland config (for example `~/.config/hypr/hyprland.conf`):

```text
exec-once = hyprctl plugin load /full/path/to/cursor-fix.so
```
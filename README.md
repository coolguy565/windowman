# WindowMan

Runtime window icon replacement launcher for Windows. Apply custom `.ico` files to any window's taskbar and title bar at runtime without modifying the target EXE.

## Usage

```
WindowMan.exe --icon <file.ico> [--title <match>] [--keepalive] -- <program> [args...]
```

## Options

| Flag | Description |
|------|-------------|
| `--icon <file.ico>` | Path to the `.ico` file to apply (required) |
| `--title <match>` | Substring to match against window titles |
| `--keepalive` | Keep searching for windows until the target process exits |
| `--` | Separator: everything after this is the program + args |
| `--help` | Show help |

## Examples

```bash
WindowMan.exe --icon roblox-old.ico -- "C:\Path\RobloxPlayerBeta.exe"
WindowMan.exe --icon old.ico --title "Roblox" -- "program.exe" --foo
WindowMan.exe --icon app.ico --keepalive -- "app.exe"
```

## How It Works

1. Snapshots all existing windows before launch
2. Creates a temp `.lnk` shortcut with your icon and launches via `ShellExecuteEx` — this makes Windows use the shortcut's icon for the taskbar
3. Polls for new windows that weren't in the snapshot
4. Applies `WM_SETICON` + `SetClassLongPtrW` for the window title bar icon
5. Keeps searching for up to 30s even after bootstrapper processes exit

## Building

Requires [MinGW-w64](https://www.mingw-w64.org/) cross-compiler (available in MSYS2 or WSL):

```bash
./build.sh
```

Or on Windows with MSVC:

```
build.bat
```

## Use Case: Roblox Old Icon

1. Place `WindowMan.exe` somewhere permanent
2. Edit your desktop shortcut to target WindowMan instead of Roblox directly:

```
Target: C:\Path\To\WindowMan.exe
Arguments: --icon "C:\Path\To\old-icon.ico" -- "C:\Path\To\RobloxPlayerBeta.exe"
```

3. Set the shortcut's icon to your custom `.ico` as well

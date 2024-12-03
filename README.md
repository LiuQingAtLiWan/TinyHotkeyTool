# TinyHotkeyTool

Forked from [yanglx2022/TinyHotkeyTool](https://github.com/yanglx2022/TinyHotkeyTool).

The original "readme" can be found [here](./README_original.md).

This program enables global hotkey functionality on Windows, registering hotkeys based on a configuration file. It runs in the background without a user interface, responding to global hotkeys by executing corresponding commands. You can check if it’s running via Task Manager.

------

## Features

### Configuration File (setting.ini) Instructions

- Each line starting with `#` represents one configuration. The configuration items are separated by `|`, and the second `#` indicates the end of that configuration. Text following it serves as a comment. A line with `#####` marks the end of all configurations.
- **Configuration details**:
  - The first item: `C` represents Ctrl, `A` represents Alt.
  - The second item: an English letter representing the hotkey (up to two keys).
  - The third item: the program to launch along with its command-line arguments.
  - The fourth item: special parameters:
    - `[clipboard]`: uses clipboard text as a command-line argument.
    - `[exit]`: closes the program.
    - `[uac]`: requests administrator privileges.
- Changes to the configuration file require a program restart to take effect.

#### Configuration File Example

```ini
#CA|K||[exit]#                         Ctrl+Alt+K closes the program  
#CA|U||[uac]#                          Ctrl+Alt+U requests administrator privileges  
#CA|IP|C:\Windows\System32\calc.exe#   Ctrl+I+P opens Calculator  
#CA|D|explorer "C:\Users\Yang\Documents\"# Ctrl+Alt+D opens the Documents folder  
#CA|W|explorer|[clipboard]#          Ctrl+Alt+W opens the folder path copied to the clipboard  
```

------

## Usage Instructions

1. Place the compiled `ShortCut.exe` file and the configured `setting.ini` file in the same directory.
2. Run `ShortCut.exe`. A dialog box will pop up asking if you want to enable it at startup. Choose "Yes" or "No". The program will then run in the background, listening for hotkeys.
3. When a hotkey launches a program requiring administrator privileges, Windows will prompt for confirmation each time. You can use the Ctrl+Alt+U hotkey (or a customized hotkey) to grant administrator privileges. Afterward, the confirmation window won’t appear for programs requiring elevated privileges.
4. To close this program, press the Ctrl+Alt+K hotkey (or a customized hotkey), which will trigger a confirmation dialog. Choose "Yes" to exit the program.

------

## Compilation Instructions

Open Visual Studio, create a new empty VC++ project, and add `shortcut.cpp` to the project. Then compile the project.

(Tested on Windows 10 with Visual Studio 2008, 2013, and 2017.)

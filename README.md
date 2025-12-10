
# NAME
**parte** simple notification displayer

# SYNOPSIS
**parte** [options] [*text* ...]

# DESCRIPTION
**parte** is a simple notification displayer that accepts input from
standard input or command-line arguments.  
It is highly configurable through its arguments and compile-time options.
Right-clicking the notification may trigger an action.

# EXAMPLE WITHOUT OPTIONS
```sh

    parte "Hello" "This is another line"
    echo "Hello\nThis is another line" | parte
    echo "Hello" | parte "This is another line"
    parte "right clic for echo" && echo "hello world!!"
```
**parte** return 1 in normal circunstances, 2 in errors and 0 when is closed with a right clic, so yo can make a script work only if you decide to (like above).

# EXIT STATUS
**parte** returns:

| Code | Meaning                |
| ---- | ---------------------- |
| `0`  | Closed via right-click |
| `1`  | Normal termination     |
| `2`  | Error occurred         |


# OPTIONS

| Flag                 | Argument    | Description                                                                                        |
| -------------------- | ----------- | -------------------------------------------------------------------------------------------------- |
| `-h`, `-?`, `--help` | —           | Show help and exit.                                                                                |
| `-u`                 | `mode`      | Set notification mode: `low`, `normal`, `urgent` (default: `normal`). Overrides prior style flags. |
| `-t`                 | `seconds`   | Display duration in seconds.                                                                       |
| `-border-size`      | `pixels`    | Border size in pixels.                                                                             |
| `-fg`                | `"#RRGGBB"` | Foreground (font) color.                                                                           |
| `-bg`                | `"#RRGGBB"` | Background color.                                                                                  |
| `-border-color`     | `"#RRGGBB"` | Border color.                                                                                      |
| `-font`              | `pattern`   | Fontconfig pattern for the font.                                                                   |
| `-focus-keyboard`   | —           | Show notification on monitor with keyboard focus.                                                  |
| `-focus-mouse`      | —           | Show notification on monitor under mouse cursor.                                                   |
| `-top-left`         | —           | Place notification in top-left corner.                                                             |
| `-top-right`        | —           | Place notification in top-right corner.                                                            |
| `-bottom-left`      | —           | Place notification in bottom-left corner.                                                          |
| `-bottom-right`     | —           | Place notification in bottom-right corner.                                                         |


# EXAMPLE WITH OPTIONS
```sh

    parte -fg "#FFFFFF" -bg "#000000" "Hello" "This is another line"
    echo "Hello\nThis is another line very urgent" | parte -u "urgent"
    echo "Hello" | ./parte -u "low" -t 3 "This is another line"
```

## INSTALATION

```sh
make config.h
make install clean
```
use sudo if your user needs it.
```sh
make uninstall
```
This will uninstall it.

# LICENCE
Copyright (c) Juan de la Puente Valbuena <softwaredelapuente@gmail.com>
Licensed under the EUPL-1.2 see more details in LICENSE

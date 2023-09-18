# escapeline

Escape (sequence) (status) Line.
Simple status line like uim-fep.

Works on both Windows and Linux.

```
update TTY stdin scroll region <== [SIGWINCH] ==> update pty size

+--------------------+^
|                    ||
| term rows - height |PTY size == SCROLL REGION(DECSTBM)
|                    |V
+--------------------+
| status line height |
+--------------------+
```

## build

```
> meson setup builddir
> meson compile -C builddir
```

## TODO

- Track the output of the pty to appropriately output additional drawing sequences.

## reference

- https://github.com/LionyxML/nocurses
- https://learn.microsoft.com/ja-jp/windows/console/creating-a-pseudoconsole-session
- https://learn.microsoft.com/en-us/windows/console/console-virtual-terminal-sequences
- https://github.com/uim/uim/blob/master/fep/README.ja


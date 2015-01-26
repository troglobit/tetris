Micro Tetris™
=============

Micro Tetris™ — one of the smallest Tetris implementations in the world!
Utilizing only ANSI escape sequences to draw the board, no external
library dependencies other than a standard C library, like [uClibc] or
[musl libc]. Hence, very suitable for embedded devices in need of an
easter egg ;-)

![ASCII Image of Micro Tetris](micro-tetris.png "Play Micro Tetris!")


Keyboard Control
----------------

    j         - Left
    k         - Rotate
    l         - Right
    <SPACE>   - Drop
    p         - Pause
    q         - Quit

The game is based on a 1989 *International Obfuscated C Code Contest*
(IOCCC) entry made by [John Tromp].

Issue tracker and GIT repository available at GitHub:

* [Repository]
* [Issue Tracker]
* [tetris-1.2.1.tar.bz2][latest release], [MD5][release hash]


Origin & References
--------------------

Micro Tetris™ is based on an original [IOCCC](http://www.ioccc.org)
entry by [John Tromp], initiated by [Freek Wiedijk].  See John's home
page for the source code http://tromp.github.io/tetris.html, or the
[1989 IOCCC entry page](http://www.ioccc.org/1989/tromp.hint)

This human-readable "clone" is maintained by [Joachim Nilsson].

[uClibc]: http://uclibc.org
[musl libc]: http://musl-libc.org
[Repository]: http://github.com/troglobit/tetris
[Issue Tracker]: http://github.com/troglobit/tetris/issues
[latest release]: ftp://troglobit.com/tetris/tetris-1.2.1.tar.bz2
[release hash]: ftp://troglobit.com/tetris/tetris-1.2.1.tar.bz2.md5
[John Tromp]: http://tromp.github.io/
[Freek Wiedijk]: http://www.cs.ru.nl/F.Wiedijk/
[Joachim Nilsson]: http://troglobit.com


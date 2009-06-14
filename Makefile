# Micro Tetris Makefile
# Remember to update the VERSION before a new release.
# -- " --  to set the DESTDIR env. variable when installing.
#
# Set CC and CFGLAGS in your local environment for a suitable
# compiler (tcc?) and CFLAGS (-Os -W -Wall -Werror).

VERSION = 1.0.0

all: tetris

tetris.o: Makefile tetris.c tetris.h

clean:
	-@$(RM) tetris tetris.o

distclean: clean

install: all
	install tetris $(DESTDIR)

dist:
	@git archive --format=tar --prefix=tetris-$(VERSION)/ $(VERSION) | bzip2 >../tetris-$(VERSION).tar.bz2


# Micro Tetris Makefile
# Remember to update the VERSION before a new release.
# -- " --  to set the DESTDIR env. variable when installing.
#
# Set CC and CFGLAGS in your local environment for a suitable
# compiler (tcc?) and CFLAGS (-Os -W -Wall -Werror).

VERSION        = 1.3.0-beta1
CFG_OPTS      ?= -DENABLE_SCORE -DENABLE_PREVIEW -DENABLE_HIGH_SCORE
CC            ?= @gcc
CPPFLAGS      += $(CFG_OPTS)

all: tetris

tetris.o: Makefile tetris.c tetris.h

clean:
	-@$(RM) tetris tetris.o *~

distclean: clean

install: all
	@install -D -m 0755 tetris $(DESTDIR)/bin/tetris
	@touch /tmp/tetris.scores
	@install -D -m 0664 /tmp/tetris.scores $(DESTDIR)/var/games/tetris.scores

dist:
	@git archive --format=tar --prefix=tetris-$(VERSION)/ $(VERSION) | bzip2 >../tetris-$(VERSION).tar.bz2


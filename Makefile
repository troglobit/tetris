# Micro Tetris Makefile
# Remember to update the VERSION before a new release.
# -- " --  to set the DESTDIR env. variable when installing.
#
# Set CC and CFGLAGS in your local environment for a suitable
# compiler (tcc?) and CFLAGS (-Os -W -Wall -Werror).

VERSION        = 1.3.0-beta8
CFG_OPTS      ?= -DENABLE_SCORE -DENABLE_PREVIEW -DENABLE_HIGH_SCORE
CC            ?= @gcc
CPPFLAGS      += $(CFG_OPTS)

all: tetris

tetris.o: Makefile tetris.c tetris.h

clean:
	-@$(RM) tetris tetris.o

distclean: clean
	-@$(RM) *.o *~

install: all
	@install -D -m 0755 tetris $(DESTDIR)/bin/tetris
	@touch /tmp/tetris.scores
	@install -D -m 0664 /tmp/tetris.scores $(DESTDIR)/var/games/tetris.scores

uninstall:
	-@$(RM) $(DESTDIR)/bin/tetris

dist:
	@git archive --format=tar --prefix=tetris-$(VERSION)/ $(VERSION) | bzip2 >../tetris-$(VERSION).tar.bz2
	@(cd .. && md5sum    tetris-$(VERSION).tar.bz2 > tetris-$(VERSION).tar.bz2.md5)
	@(cd .. && sha256sum tetris-$(VERSION).tar.bz2 > tetris-$(VERSION).tar.bz2.sha256)

release: dist
	@echo "Resulting release files in parent dir:"
	@echo "=================================================================================================="
	@for file in tetris-$(VERSION).tar.bz2; do					\
                printf "%-33s Distribution tarball\n" $$file;                           \
                printf "%-33s " $$file.md5;    cat ../$$file.md5    | cut -f1 -d' ';    \
                printf "%-33s " $$file.sha256; cat ../$$file.sha256 | cut -f1 -d' ';    \
	done

VERSION = 1.0.0

all: tetris

tetris.o: Makefile tetris.c tetris.h

romfs: all
	@$(ROMFSINST) /bin/tetris

clean:
	-@$(RM) tetris tetris.o

distclean: clean

dist:
	@git archive --format=tar --prefix=tetris-$(VERSION)/ $(VERSION) | bzip2 >../tetris-$(VERSION).tar.bz2


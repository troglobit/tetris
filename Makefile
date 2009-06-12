all: tetris

tetris.o: Makefile tetris.c tetris.h

romfs: all
	@$(ROMFSINST) /bin/tetris

clean:
	-@$(RM) tetris tetris.o

distclean: clean


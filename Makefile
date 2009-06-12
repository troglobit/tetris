CFLAGS         := $(filter-out -Werror, $(CFLAGS))
CFLAGS         := $(filter-out -W -Wall, $(CFLAGS))

all: tetris

romfs: all
	$(ROMFSINST) /bin/tetris

clean:
	rm tetris

distclean: clean

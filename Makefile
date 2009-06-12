CFLAGS         := $(filter-out -Werror, $(CFLAGS))
CFLAGS         := $(filter-out -W -Wall, $(CFLAGS))

all: tetris

romfs:
	$(ROMFSINST) /bin/tetris

clean:
	rm tetris

distclean: clean

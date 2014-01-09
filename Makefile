all: keep_wal_files

CFLAGS += -Os -Wall -Werror

keep_wal_files: keep_wal_files.c
	$(CC) $(CFLAGS) -o keep_wal_files keep_wal_files.c

clean:
	rm -f keep_wal_files

.PHONY: clean

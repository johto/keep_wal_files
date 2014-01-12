all: keep_wal_files record_standby_location

CFLAGS += -O2 -Wall -Werror

keep_wal_files: keep_wal_files.c
	$(CC) $(LDFLAGS) $(CFLAGS) -o keep_wal_files keep_wal_files.c

record_standby_location: record_standby_location.c
	$(CC) $(CFLAGS) -o record_standby_location record_standby_location.c $(LDFLAGS) -lpq

clean:
	rm -f keep_wal_files record_standby_location

.PHONY: clean

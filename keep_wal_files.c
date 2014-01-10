#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>

/*
 * N.B: the assumptions we make about the filenames and the comparison code is
 * copied from pg_archivecleanup.
 */

#define XLOG_DATA_FNAME_LEN  24

static const char *progname = "keep_wal_files";


static void
check_xlog_filename(const char *fname, const char *origin)
{
	if (strlen(fname) != XLOG_DATA_FNAME_LEN)
	{
		fprintf(stderr, "%s: %s \"%s\" has invalid length %d, expected %d\n",
					progname, origin, fname, (int) strlen(fname), XLOG_DATA_FNAME_LEN);
		exit(1);
	}

	if (strspn(fname, "0123456789ABCDEF") != XLOG_DATA_FNAME_LEN)
	{
		fprintf(stderr, "%s: %s \"%s\" is not a valid xlog filename\n",
					progname, origin, fname);
		exit(1);
	}
}

int
main(int argc, char *argv[])
{
	FILE *fh;
	char buf[65];
	size_t len;
	int err;

	if (argc != 3)
	{
		fprintf(stderr, "Usage: %s <progressfile> <%%f>\n", argv[0]);
		exit(1);
	}

	check_xlog_filename(argv[2], "%f");

	fh = fopen(argv[1], "r");
	if (!fh)
	{
		fprintf(stderr, "%s: could not open file %s for reading: %s\n",
					progname, argv[1], strerror(errno));
		exit(1);
	}

	len = fread(buf, 1, sizeof(buf) - 1, fh);
	if ((err = ferror(fh)) != 0)
	{
		fprintf(stderr, "%s: could not read from progress file: %s\n",
					progname, strerror(err));
		exit(1);
	}

	/* kill trailing newlines */
	while (len > 0 && (buf[len-1] == '\r' || buf[len-1] == '\n'))
		len--;
	buf[len] = '\0';

	fclose(fh);

	check_xlog_filename(buf, "progress file contents");

	return strcmp(buf + 8, argv[2] + 8) > 0 ?  0 : 2;
}

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>

#include <libpq-fe.h>

/* meh */
#define MAX_FPATH_LEN 1024

#define XLOG_DATA_FNAME_LEN  24

static const char *progname = "record_standby_location";


static int
get_standby_location(const char *standby, char *buf)
{
    PGconn *conn;
    PGresult *res;
    const char *values[] = { standby, NULL };
    int len;
    int ret;

    conn = PQconnectdb("");
    if (PQstatus(conn) != CONNECTION_OK)
    {
        fprintf(stderr, "%s: could not connect to the database: %s\n",
                    progname, PQerrorMessage(conn));
        PQfinish(conn);
        exit(1);
    }

    res = PQexecParams(conn, "SELECT get_standby_location($1)",
                           1, NULL, values, NULL, NULL, 1);
    if (PQresultStatus(res) != PGRES_TUPLES_OK)
    {
        fprintf(stderr, "%s: query failed: %s\n",
                    progname, PQerrorMessage(conn));
        PQclear(res);
        PQfinish(conn);
        exit(1);
    }

    if (PQntuples(res) != 1 || PQnfields(res) != 1)
    {
        fprintf(stderr, "%s: unexpected result structure (%d, %d)\n",
                    progname, PQntuples(res), PQnfields(res));
        PQclear(res);
        PQfinish(conn);
        exit(1);
    }

    if (PQgetisnull(res, 0, 0) == 0)
    {
        len = PQgetlength(res, 0, 0);
        memcpy(buf, PQgetvalue(res, 0, 0), len);
        buf[len] = '\0';
        ret = 1;
    }
    else
        ret = 0;

    PQclear(res);
    PQfinish(conn);
    return ret;
}

int
main(int argc, char *argv[])
{
    char tmpfilename[MAX_FPATH_LEN + 1];
	FILE *fh;
	char buf[65];
	size_t len;
    int ret;
	int err;

	if (argc != 3)
	{
		fprintf(stderr, "Usage: %s <progressfile> <standbyname>\n", argv[0]);
		exit(1);
	}

    /* silently exit if the standby isn't connected */
    if (get_standby_location(argv[2], buf) == 0)
        exit(2);

    /*
     * Record the location, but write to a temp file first to make sure the
     * operation is atomic.
     */
    ret = snprintf(tmpfilename, MAX_FPATH_LEN, "%s.tmp", argv[1]);
    if (ret < 0 || ret >= MAX_FPATH_LEN)
    {
        fprintf(stderr, "%s: could not format temporary file name: %s\n",
                    progname, strerror(errno));
        exit(1);
    }

	fh = fopen(tmpfilename, "w");
	if (!fh)
	{
		fprintf(stderr, "%s: could not open file %s for reading: %s\n",
                    progname, argv[1], strerror(errno));
		exit(1);
	}

    len = strlen(buf);
    if (fwrite(buf, 1, len, fh) != len)
    {
        fprintf(stderr, "%s: could not write to temporary file: %s\n",
                    progname, strerror(ferror(fh)));
        exit(1);
    }
	if ((err = ferror(fh)) != 0)
	{
		fprintf(stderr, "%s: could not read from progress file: %s\n",
                    progname, strerror(err));
		exit(1);
	}
	if (fclose(fh) != 0)
    {
        fprintf(stderr, "%s: could not close file: %s\n",
                    progname, strerror(errno));
        exit(1);
    }

    err = rename(tmpfilename, argv[1]);
    if (err == -1)
    {
        fprintf(stderr, "%s: could not rename temporary file: %s\n",
                    progname, strerror(err));
        exit(1);
    }
    return 0;
}

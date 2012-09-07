#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/time.h>
#include <postgresql/libpq-fe.h>

// include common defines
#include "common.h"
#include "config.h"

// postgres functions
int     exec_query(PGconn *, char *);
PGconn *open_db(void);
void    init_db(PGconn *);
void    close_db(PGconn *);

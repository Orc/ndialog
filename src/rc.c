/*
 * rc: /etc/rc.d/rc as a C program, with i/o redirection, etcetera
 */
#include <stdio.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <string.h>
#include <unistd.h>
#include <syslog.h>
#include <malloc.h>
#include <dialog.h>

#if ACTUAL
# define RUNLEVEL "/etc/rc.d/runlevel"
# define RCPREFIX "/etc/rc.d/rc.%c"
#else
# define RUNLEVEL "RUNLEVEL"
# define RCPREFIX "/etc/rc.d/rc.%c"
#endif

char
getrunlevel()
{
    FILE *f;
    char c = 0;

    if ((f = fopen(RUNLEVEL, "r")) != (FILE*)0) {
	fscanf(f, "%c\n", &c);
	fclose(f);
    }
    return c;
}

void
saverunlevel(char runlevel)
{
    FILE *f;

    if ((f = fopen(RUNLEVEL, "w")) != (FILE*)0) {
	fprintf(f, "%c\n", runlevel);
	fclose(f);
    }
}


cmp(char **a, char **b)
{
    return strcmp(*a, *b);
}

char *
getSTDrc(char runlevel, char sw)
{
    DIR *dir;
    char bfr[200];
    struct stat st;
    struct dirent *rv;

    static char **rcfiles = 0;
    static int idx, count = 0;

    if (runlevel && sw) {
	sprintf(bfr, RCPREFIX, runlevel);
	dir = opendir(bfr);
	if (dir == (DIR*)0)
	    return (char*)0;
	if (rcfiles) {
	    for (idx=0; idx < count; idx++)
		free(rcfiles[idx]);
	    free(rcfiles);
	    rcfiles = 0;
	    count = 0;
	}
	do {
	    rv = readdir(dir);
	    if (rv && (rv->d_name[0] == sw)) {
		sprintf(bfr, RCPREFIX "/%s", runlevel, rv->d_name);
		if (stat(bfr, &st) == 0 && S_ISREG(st.st_mode)
				    && (st.st_mode & S_IXUSR)) {
		    rcfiles = realloc(rcfiles, (1+count)*sizeof rcfiles[0]);
		    rcfiles[count++] = strdup(bfr);
		}
	    }
	} while (rv != (struct dirent*)0);
	qsort(rcfiles, count, sizeof rcfiles[0], cmp);
	idx = 0;
    }
    if (idx < count)
	return rcfiles[idx++];
    return (char*)0;
}


void
runcommand(char *cmd, char *arg)
{
#if ACTUAL
    pid_t child;
    int status;
#endif
    char *basename;

    if ((basename = strrchr(cmd, '/')) != (char*)0)
	basename ++;
    else
	basename = cmd;

#if ACTUAL
    child = fork();
    if (child == 0) {
	execl(cmd, basename, arg, 0);
	perror(basename);
	exit(1);
    }
    else if (child > 0)
	wait(&status);
    else
	perror(basename);
#else
    printf("> %s %s %s\n", cmd, basename, arg);
#endif
}


void
main(int argc, char **argv)
{
    char *d;
    char oldrunlevel = getrunlevel();
    char newrunlevel;
    char bfr[200];

    if (argc < 2)
	newrunlevel = 'm';
    else
	newrunlevel = tolower(argv[1][0]);
    if (newrunlevel == 0 || strchr("s123456m", newrunlevel) == (char*)0) {
	if (newrunlevel)
	    syslog(LOG_ERR,
		    "rc called with bogus runlevel %c; using 'm' instead",
		    newrunlevel);
	newrunlevel = 'm';
    }

    if (newrunlevel == oldrunlevel)
	exit(0);

    sprintf(bfr, RCPREFIX, newrunlevel);

    if (access(bfr, X_OK) != 0) {
	syslog(LOG_ERR, "rc called for nonexistant runlevel %c", newrunlevel);
	exit(1);
    }

    saverunlevel(newrunlevel);

    if (oldrunlevel) {
	for (d = getSTDrc(oldrunlevel, 'K'); d; d = getSTDrc(0,0))
	    runcommand(d, "stop");
    }

    for (d = getSTDrc(newrunlevel, 'S'); d; d = getSTDrc(0,0))
	runcommand(d, "start");
    exit(0);
} /* rc */

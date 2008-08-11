#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dialog.h>
#include <unistd.h>

void
main(int argc, char **argv)
{
    char *document = (argc > 1) ? argv[1] : "index.html";
    char *fullpath;
    char working_directory[400];

    if (document[0] != '/') {
	getcwd(working_directory, sizeof working_directory);
	fullpath = malloc(strlen(working_directory) + 2 + strlen(document));
	if (fullpath)
	    sprintf(fullpath, "%s/%s", working_directory, document);
	else
	    perror("memory allocation");
    }
    else
	fullpath = document;

    init_dialog();
    _nd_help(fullpath);
    end_dialog();
}

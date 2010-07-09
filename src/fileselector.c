/*
 * need to abstract this into a general-purpose selector box.
 *
 * I need to pass (a) directory open/read/close routines [for
 * file selection via nfs] (b) flags to tell it whether to
 * 1) allow the user to CD up past the topdir, 2) allow the
 * user to select files (not just directories), 3) allow the
 * user to edit the pattern, 4) return without checking that
 * everything is okay.
 *
 * It also needs to be made reentrant, via hackery to the
 * internals of ndialog.
 */
#include <dialog.h>
#include <ndialog.h>

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>
#include <errno.h>
#include <regex.h>
#include <string.h>

/*
 * the selector is a pseudo-widget that fileselector() uses to keep
 * all the internal context it cares about
 */
static struct {
    char *curdir;		/* current directory */
    int szcurdir;
    char *pattern;		/* pattern to match files against */
    LIA directories;		/* directories in current dir (incl ..) */
    LIA files;			/* files in current dir */
    ndObject selection;		/* point at the selection object */
    ndObject dirlist;		/* ... the directory list */
    ndObject filelist;		/* ... and the file list */
    char result[1024];
} selector;


/*
 * populateSelector() fills the dirlist and filelist with
 *                    directory entries.
 */
static void
populateSelector(ndDisplay display)
{
    DIR* dir;
    struct dirent *de;
    struct stat st;
    regex_t re;
    char *tmp;
    int rc;

    deleteLIA(selector.directories);
    deleteLIA(selector.files);
    selector.directories = newLIA(0,0);
    selector.files = newLIA(0,0);

    if (selector.pattern)
	regcomp(&re, selector.pattern, REG_NOSUB);

    if ((dir = opendir(selector.curdir[0] ? selector.curdir : "/")) != 0) {
	for (de = readdir(dir); de; de = readdir(dir)) {
	    if (de->d_name[0] == '.' && !de->d_name[1])
		continue;
	    if ( tmp = malloc(strlen(selector.curdir)+strlen(de->d_name)+9) ) {
		sprintf(tmp, "%s/%s", selector.curdir, de->d_name);
		rc = stat(tmp, &st);
		free(tmp);

		if ( rc != 0 )
		    continue;
		
		if (S_ISDIR(st.st_mode))
		    addToLIA(selector.directories, 0, de->d_name, 0);
		else if (selector.pattern == 0 || regexec(&re, de->d_name, 0, 0, 0) == 0)
		    addToLIA(selector.files, 0, de->d_name, 0);
	    }
	}
	closedir(dir);
    }
    else 
	addToLIA(selector.directories, 0, "..", 0);/* always allow escape */
    regfree(&re);
    setObjData(selector.dirlist,0L,LIAlist(selector.directories),
				   LIAcount(selector.directories));
    if (LIAcount(selector.directories) == 0)
	setReadonly(selector.dirlist);
    else
	setWritable(selector.dirlist);

    setObjData(selector.filelist,0L,LIAlist(selector.files),
				    LIAcount(selector.files));
    if (LIAcount(selector.files) == 0)
	setReadonly(selector.filelist);
    else
	setWritable(selector.filelist);

    strcpy(selector.result, selector.curdir);
    strcat(selector.result, "/");
    setObjCursor(selector.selection, strlen(selector.result)-1);

    if (display) {
	drawObj(selector.filelist, display);
	drawObj(selector.dirlist, display);
	drawObj(selector.selection, display);
    }
    else {
	touchObj(selector.filelist);
	touchObj(selector.dirlist);
	touchObj(selector.selection);
    }
} /* populateSelector */


/*
 * fs_dir_cb() is fired when someone clicks on a directory
 */
static int
fs_dir_cb(ndObject obj, ndDisplay display)
{
    int i = getObjCursor(obj);
    ListItem *tmp = getObjList(obj);
    char *p;

    if (strcmp(tmp[i].item, "..") == 0) {
	/* going up one directory; trim a directory level
	 * from selector.curdir
	 */
	if ((p = strrchr(selector.curdir, '/')) != 0) {
	    *p = 0;
	    selector.szcurdir = (p-selector.curdir);
	}
    }
    else {
	/* going down a directory */
	selector.szcurdir += 1+strlen(tmp[i].item);
	selector.curdir = realloc(selector.curdir, 1+selector.szcurdir);
	strcat(selector.curdir, "/");
	strcat(selector.curdir, tmp[i].item);
    }
    populateSelector(display);
    return 0;
} /* fs_dir_cb */

/*
 * fs_sel_cb() deals with a user-entered selection
 */
static int
fs_sel_cb()
{
    if (access(selector.result, F_OK) != 0) {
	Error(selector.result);
	return 1;
    }
    return -1;
} /* fs_sel_cb */


/*
 * fs_file_cb() attaches a file to the selection and fails successfully
 */
static int
fs_file_cb(ndObject obj, ndDisplay display)
{
    strcat(selector.result, getObjList(obj)[getObjCursor(obj)].item);
    drawObj(selector.selection, display);
    return -1;
} /* fs_file_callback */



/*
 * fileselector() pops up a file selection box.  (This should be generalized
 *                into a widget, but will be kept as a user-level function
 *                until I've got it debugged.)
 */
char *
fileselector(int width, int depth, char *title, char *pattern, char *dir)
{
    ndObject chain;
    int rc;
    char *p;
    char *ret;

    if (width < 40 && depth < 10) {
	errno = EINVAL;
	return 0;
    }

    /* build the menu:  selection at the top, with directory and
     * file lists side-by-side below it.
     */
    selector.selection = chain = newString(0,0, width-12,
		      sizeof selector.result, selector.result,
		      0, "Selection:|", fs_sel_cb, 0);

    chain = ObjChain(chain,
		    selector.dirlist = newMenu(0,3, (width/2)-2,
					       depth-6, 0, 0, "Directories", "",
					       0, fs_dir_cb, 0));

    chain = ObjChain(chain,
		    selector.filelist = newMenu(width/2, 3, (width/2)-2,
						depth-6, 0, 0, "Files", "",
						0, fs_file_cb, 0));

#if DIRECTORY_SELECT_ONLY
    chain = ObjChain(chain, newOKButton(0, "OK", fs_sel_cb, 0));
#endif
    chain = ObjChain(chain, newCancelButton(1, "Cancel", 0, 0));

    /* populate the selector pseudo-widget */
    selector.directories = newLIA(0,0);
    selector.files = newLIA(0,0);
    selector.pattern = malloc(strlen(pattern) * 2);

    /* convert shell pattern to regex() pattern
     */
    for (p=selector.pattern; *pattern; ) {
	if (*pattern == '\\' && pattern[1]) {
	    *p++ = *pattern++;
	    *p++ = *pattern++;
	}
	else if (*pattern == '*') {	/* expand * to .* */
	    *p++ = '.';
	    *p++ = *pattern++;
	}
	else if (*pattern == '?') {	/* ? maps to . */
	    *p++ = '.';
	    pattern++;
	}
	else if (*pattern == '.') {	/* . expands to \. */
	    *p++ = '\\';
	    *p++ = *pattern++;
	}
	else				/* pass everything else */
	    *p++ = *pattern++;
    }
    *p = 0;

    selector.szcurdir = 1024;
    selector.curdir = strdup(dir);
    memset(selector.result, 0, sizeof selector.result);

    populateSelector(0);

    
    rc = MENU(chain,-1,-1,title ? title : "File selector",0,0);

    deleteObjChain(chain);

    if (rc == MENU_OK)
	ret = strdup(selector.result);
    else
	ret = 0;

    /* wipe out the contents of the selector
     */
    deleteLIA(selector.directories);
    deleteLIA(selector.files);
    if (selector.pattern)
	free(selector.pattern);
    if (selector.curdir)
	free(selector.curdir);

    return ret;
} /* fileselector */


#ifdef TEST
int
main()
{
    char *res;
    char here[1024];

    init_dialog();
    getcwd(here, sizeof here);
    res = fileselector(50,18,"Test", "*.c", here);
    end_dialog();

    if (res)
	puts(res);
    exit(0);
}
#endif

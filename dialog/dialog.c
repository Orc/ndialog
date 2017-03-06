#include <dialog.h>
#include <ndialog.h>
#include <curse.h>

#include <stdio.h>
#include "basis/options.h"
#include <unistd.h>
#include <stdlib.h>
#include <term.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/mman.h>

extern int errno;


enum boxtypes { YESNO=1, MSG, INPUT, INFO, TEXT, LIST, CHECK, RADIO };

struct x_option opts[] = {
    { 'c',   0, "clear",   0,       "clear the screen when finished" },
    { 't',   0, "title",   "TITLE", "the title for the dialog box" },
    { 'h',   0, "hline",   "TITLE", "a title for the bottom of the dialog box" },
    { 'f',   0, "hfile",   "FILE",  "a helpfile to be displayed by ? or F1" },
    { YESNO, 0, "yesno",   0,       "a yes/no dialog box (text height width)" },
    { MSG,   0, "msgbox",  0,       "a message box (text height width)" },
    { INFO,  0, "infobox", 0,       "an info box (text height width)" },
    { INPUT, 0, "inputbox",0,       "an input box (text height width)" },
    { TEXT,  0, "textbox", 0,       "a text box (file height width)" },
    { LIST,  0, "menu",    0,       "a menu (text height width menu-height items)" },
    { CHECK, 0, "checklist",0,      "a checklist (text height width list-height items)" },
    { RADIO, 0, "radiolist",0,      "a radiolist (text height width list-height items)" },
    
};

struct box {
    enum boxtypes boxtype;
    char *text;
    int height;
    int width;
    int display_height;
    int nrdata;	/* number of items for menu or checklist */
    char **data;
};

struct box *boxes = 0;
int nrboxes = 0;
    
char *title = 0;

#define NROPTS (sizeof opts / sizeof opts[0])

void
usage(int retcode)
{
    fprintf(stderr, "usage: dialog --clear\n"
		    "       dialog [--title title] [--clear] [--hline line] "
		    "[--hfile file] box-args\n");
    exit(retcode);
}


char *
whatis(enum boxtypes boxtype)
{
    for ( int i=0; i < NROPTS; i++ )
	if ( opts[i].optval == boxtype )
	    return opts[i].name;

    fprintf(stderr, "dialog: internal error (add_box of unknown type %d)\n",
		    boxtype);
    exit(1);
}


int
box_atoi(char *num, int min)
{
    int ret = atoi(num);

    if ( ret < 0 ) return ret;

    if ( ret < min ) return min;

    return ret;
}


void
add_box(enum boxtypes boxtype, int argc, char **argv)
{
    int count, i;

    if ( nrboxes == 0 )
	boxes = malloc(sizeof(struct box));
    else {
	/*allow multiples later? */
	/*boxes = realloc(boxes, (1+nrboxes)*sizeof(struct box));*/
	usage(1);
    }

    if ( boxes == 0 ) {
	fprintf(stderr, "dialog: %s creating box[] item\n", strerror(errno));
	exit(1);
    }
    boxes[nrboxes].boxtype = boxtype;
    
    count = argc - x_optind;
    switch (boxtype) {
    case YESNO:
    case MSG:
    case INPUT:
    case INFO:
    case TEXT:	/* all need 3 more arguments */
		if ( count != 3 ) {
		    fprintf(stderr, "usage; dialog --%s arg height width\n",
					whatis(boxtype));
		    exit(1);
		}
		boxes[nrboxes].text = argv[x_optind++];
		boxes[nrboxes].height = box_atoi(argv[x_optind++], (boxtype=INFO) ? 3 : 7);
		boxes[nrboxes].width = box_atoi(argv[x_optind++], 2);
		break;
    case LIST:
		if ( (count < 5) || (count % 2 == 0) ) {
		    /* need at least 5 arguments, and #args must be odd
		     */
		    fprintf(stderr, "usage: dialog --menu text height width "
				    "[tag item] ...\n");
		    exit(1);
		}
		boxes[nrboxes].text = argv[x_optind++];
		boxes[nrboxes].height = box_atoi(argv[x_optind++], 7);
		boxes[nrboxes].width = box_atoi(argv[x_optind++], 2);
		boxes[nrboxes].nrdata = (argc - x_optind) / 2;

		boxes[nrboxes].data = calloc((1+argc-x_optind), sizeof(char*));
		if ( boxes[nrboxes].data == 0 ) {
		    fprintf(stderr, "dialog: %s allocating menu data\n",
				    strerror(errno));
		    exit(1);
		}
		for (i=0; x_optind < argc; )
		    boxes[nrboxes].data[i++] = argv[x_optind++];
		break;
    case RADIO:
    case CHECK:
		if ( (count < 6) || ((count % 3) != 0) ) {
		    /* need at least 6 arguments and #args must be a multiple
		     * of 3.
		     */
		    fprintf(stderr, "usage: dialog --menu text height width "
				    "[tag item status] ...\n");
		    exit(1);
		}
		boxes[nrboxes].text = argv[x_optind++];
		boxes[nrboxes].height = box_atoi(argv[x_optind++], 7);
		boxes[nrboxes].width = box_atoi(argv[x_optind++], 2);
		boxes[nrboxes].nrdata = (argc - x_optind) / 3;

		boxes[nrboxes].data = calloc((1+argc-x_optind), sizeof(char*));
		if ( boxes[nrboxes].data == 0 ) {
		    fprintf(stderr, "dialog: %s allocating %s data\n",
				    strerror(errno),
				    (boxtype==RADIO)?"radiolist":"checklist");
		    exit(1);
		}
		for (i=0; x_optind < argc; )
		    boxes[nrboxes].data[i++] = argv[x_optind++];
		break;
    default:
		fprintf(stderr, "dialog: internal error; tried to add unknown box type\n");
		exit(1);
    }
    nrboxes ++;
}

int
scrollable_box(struct box *box, char *data, off_t size)
{
    void *chain;
    int rc;

    chain = ObjChain(newOKButton(1,"OK",0, 0),
		      newText(0,0,box->width,box->height,size,0,0,data,0,0));
    chain = ObjChain(chain,
		     newCancelButton(2,"CANCEL", 0, 0));

    rc = MENU(chain, -1, -1, title, box->text, 0);
    deleteObjChain(chain);
    return rc;
}


int
text_box(struct box *box)
{
    int fd = open(box->text, O_RDONLY);
    struct stat finfo;
    char  *fdata;
    int rc = -1;

    if ( fd == -1 )
	return -1;

    /* can't do a text box if we can't stat it or it's not a regular
     * file, because we mmap() the file into our address space
     */
    if ( fstat(fd, &finfo) == 0 && S_ISREG(finfo.st_mode) ) {
	if ( (fdata = mmap(0, finfo.st_size, PROT_READ, MAP_SHARED, fd, 0)) ) {
	    rc = scrollable_box(box, fdata, finfo.st_size);
	    munmap(fdata, finfo.st_size);
	}
    }
    close(fd);
    return rc;
}


int
handle_box(struct box *box)
{
    char *result;
    int rc;
    int size;
    int ch=0, sc=0;

    /*{ YESNO, MSG, INPUT, INFO, TEXT, LIST, CHECK };*/
    switch (box->boxtype) {
    case YESNO:
		return dialog_yesno(title, box->text, box->height, box->width);
    case MSG:
		return dialog_mesgbox(title, box->text, box->height, box->width);
    case INFO:
		return dialog_msgbox(title, box->text, box->height, box->width, 0);
    case TEXT:
		return text_box(box);

    case INPUT:
		if ( box->width < 0 )
		    result = alloca(strwidth(box->text)+3);
		else
		    result = alloca(box->width);
		result[0] = 0;
		rc = dialog_inputbox(title,box->text,box->height,box->width, result);
		if ( rc == 0 )
		    fprintf(stderr, "%s\n", result);
		return rc;
    case LIST:
		size = 0;
		for ( int i = 0; i < box->nrdata; i++ )
		    if ( strlen(box->data[i*2]) > size )
			size = strlen(box->data[i*2]);

		result = alloca(size+1);

		rc = dialog_menu(title,box->text,box->height,box->width,
				    -1, box->nrdata,
				    box->data, result, &ch, &sc);
		if ( rc == 0 )
		    fprintf(stderr, "%s\n", result);
		return rc;
    case CHECK:
    case RADIO:
		size = 0;
		for ( int i = 0; i < box->nrdata; i++ )
		    size += strlen(box->data[1+(i*3)]) + 1;

		result = alloca(size+1);

		if ( box->boxtype == RADIO )
		    rc = dialog_radiolist(title, box->text,
					  box->height, box->width,
					  -1, box->nrdata, box->data, result);
		else
		    rc = dialog_checklist(title, box->text,
					  box->height, box->width,
					  -1, box->nrdata, box->data, result);
		if ( rc == 0 )
		    fprintf(stderr, "%s\n", result);
		return rc;
		    
    }
    return 1;
}


int
main(argc, argv)
char **argv;
{
    int toclear = 0;
    char *bottomtitle = 0;
    char *helpfile = 0;
    char *cl = 0, *term, *bfr = alloca(5120);
    int rc, opt;

    x_opterr = 1;
    
    while ( (opt = x_getopt(argc, argv, NROPTS, opts)) != EOF ) {
	switch (opt) {
	case 'c' :  toclear = 1;
		    break;
	case 't' :  title = x_optarg;
		    break;
	case 'h' :  bottomtitle = x_optarg;
		    break;
	case 'f' :  helpfile = x_optarg;
		    break;
	case YESNO:
	case MSG:
	case INPUT:
	case INFO:
	case TEXT:
	case LIST:
	case RADIO:
	case CHECK: add_box(opt, argc, argv);
		    break;
	
	default :   usage(1);
	}
    }

    if ( (x_optind < argc) || ( (nrboxes == 0) && !toclear) )
	usage(1);

    if ( toclear && (tgetent(bfr, getenv("TERM")) == 1) )
	cl = tgetstr("cl", &bfr);

    if ( nrboxes > 0 ) {
	init_dialog();

	if ( bottomtitle )
	    use_helpline(bottomtitle);

	for ( int x = 0; x < nrboxes; x++ )
	    rc = handle_box(&boxes[x]);
	end_dialog();
    }
    
    if ( toclear && cl )
	tputs(cl,1,putchar);
    
    exit(rc);
}

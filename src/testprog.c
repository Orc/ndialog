#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <dialog.h>
#include "ndialog.h"


ndObject list_ying, list_yang;

int
menucallback(void *obj, void* display)
{
    char message[200];

    int sel = currentSelection(obj);

    if (sel >= 0) {
	sprintf(message, "menucallback: %s", getObjList(obj)[sel].item);

	dialog_notify(message);
    }
    else
	Error("currentSelection");
    return 1;
}


void
main(int argc, char **argv)
{
    void *chain;
    int st;
    char check1 = 0, check2 = 1;
    char name[20];
    char password[20];
    char message[80];
    LIA foo;

    static char msg[] = "This is some text\n"
			"That can be found\n"
			"In a text object\n"
			"At the bottom of the sea\n";

    static ListItem menu[] = {
	/* menuitems need hotkeys */
	{ "A", "Choice A" },
	{ "B", "Choice B" },
	{ "C", "Choice C" },
	{ "D", "Choice D" },
	{ "E", "Choice E" },
	{ "F", "Choice F" },
	{ "G", "Choice G" },
	{ "H", "Choice H" },
    };
    #define NR_MENU	(sizeof menu / sizeof menu[0])

    static ListItem list[] = {
	{ "A", "Choice A", 0, 1 },
	{ "B", "Choice B", 0, 0 },
	{ "C", "Choice C", 0, 0 },
    } ;
    #define NR_LIST	(sizeof list / sizeof list[0])

    strcpy(name, "Fernando Poo");
    name[0] = 0xff;
    strcpy(password, "Sekret");
    strcpy(message, "A MESSAGE");

    setHelpRoot(".");

    chain = ObjChain(newOKButton(1,"OK", 0, 0),
		     newCheck(0,0,"A CHECK ITEM GOES PLONK",0,&check1,0,"demo.html"));
    chain = ObjChain(chain, newCancelButton(2,"CANCEL", 0, "demo.html"));
    chain = ObjChain(chain, newCheck(0,2,0,"Check | your hat?",&check2,0,"demo.html"));
    chain = ObjChain(chain, newString(4,3,10, sizeof name, name,
				      "Name", "Your name", 0, "demo.html"));
    chain = ObjChain(chain, newPWString(0,7,10, sizeof password, password,
				      0, "Your password", 0, "demo.html"));
    chain = ObjChain(chain, newString(0,10,10, sizeof message, message,
				      "String Title", 0, 0, "demo.html"));

    chain = ObjChain(chain, newMenu(30,0,-1,-1,
				    NR_MENU,menu,"A MENU","",
				    MENU_SELECTION, menucallback,"demo.html"));
    chain = ObjChain(chain, list_ying = newList(40,0,-1,-1,
				    NR_LIST,list,"A LIST","",
				    CHECK_SELECTED,0,"demo.html"));
    chain = ObjChain(chain, list_yang = newRadioList(54,0,-1,-1,
					 NR_LIST,list,"SAME LIST","",
					 HIGHLIGHT_SELECTED,0,"demo.html"));

    chain = ObjChain(chain, newText(40,7,20,3,strlen(msg),"Text",0,msg,0,"demo.html"));

    init_dialog();
    if (argc > 1) {
	switch (atoi(argv[1])) {
	default:
		use_helpline("[TAB] to move, [ESC] to exit");
		st = MENU(chain, -1, -1, "title",
		   "SCIENTIFIC PROGRESS\nGOES BONK", FANCY_MENU|ALIGN_RIGHT);
		if (st == MENU_ERROR)
		    perror("MENU");
		break;

	case 1:
		use_helpline("This is an error message");
		Error("Nothing in particular");
		break;

	case 2:
		use_helpline("Counting down");
		errno = 0;
		MENU(0,-1,-1 ,0, " ONE ", 0);
		usleep(400000);
		use_helpline("tick");
		MENU(0,-1,-1, 0, " TWO ", 0);
		usleep(400000);
		use_helpline("tick");
		MENU(0,-1,-1, 0, "THREE", 0);
		usleep(400000);
		break;
	case 4:
		foo = newLIA(0,0);
		addToLIA(foo, "1", "first", 0);
		addToLIA(foo, "2", "second", 0);
		addToLIA(foo, "3", "third", 0);
		addToLIA(foo, "4", "forth", 0);
		addToLIA(foo, "5", "fifth", 0);

		chain = newListWidget(0,0,18,3,foo,"title", 0,0,0,0);
		chain = ObjChain(chain, newCancelButton(0, "Done", 0, 0));

		st = MENU(chain, -1, -1, "list widget", 0, 0);
		if (st == MENU_ERROR)
		    perror("MENU");
		break;
	}
    }
    else {
	use_helpline("[TAB] or the mouse to navigate, [ESC] to exit");
	st = MENU(chain, -1, -1, "title",
		    "SCIENTIFIC PROGRESS\nGOES BONK", ALIGN_RIGHT);
	if (st == MENU_ERROR)
	    perror("MENU");
    }
    end_dialog();
}

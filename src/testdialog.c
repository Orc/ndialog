#include <config.h>

#include <stdio.h>
#include <dialog.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>

static char *choices[] = {
    "menu", "dialog_menu",
    "check", "dialog_checklist",
    "radio", "dialog_radiolist",
    "notify", "dialog_notify",
    "mesg", "dialog_mesgbox",
    "msg", "dialog_msgbox",
    "yesno", "dialog_yesno",
    "gauge", "dialog_gauge",
};
#define NRCHOICES	(sizeof choices / sizeof choices[0])/2

static char *menu[] = {
    "AA", "Choice A",
    "BB", "Choice B",
    "CC", "Choice C",
    "DD", "Choice D", 
    "EE", "Choice E", 
    "FF", "Choice F", 
    "GG", "Choice G", 
};

static char *checklist[] = {
    "AA", "Choice A",  "off",
    "BB", "Choice B",  "on",
    "CC", "Choice C",  "off",
    "DD", "Choice D",  "off",
    "EE", "Choice E",  "off",
    "FF", "Choice F",  "off",
    "GG", "Choice G",  "off",
};

void		/* ANSI C can bite me */
main()
{
    int rc;
    char result[20];
    int sc=0, ch=0;

    extern int LINES, COLS;

    init_dialog();

    while (1) {
	rc = dialog_menu(0, "Choose a dialog function to test",
			    LINES-2, COLS-2, NRCHOICES, NRCHOICES,
			    choices, result, &sc, &ch);

	if (rc != 0)
	    break;

	if (strstr(result, "menu")) {
	    int sc2=0, ch2=0;
	    rc = dialog_menu("menu", "menu box",
				-1, -1, 4, 7, menu, result, &sc2, &ch2);
	    if (rc == 0)
		dialog_notify(result);
	    else if (rc == 1)
		dialog_notify("CANCEL");
	    else
		dialog_notify("Ooops");

	    sc2=0, ch2=0;
	    rc = dialog_menu("menu", "menu box\nnumber 2",
				13, 35, 4, 7, menu, result, &sc2, &ch2);
	    if (rc == 0)
		dialog_notify(result);
	    else if (rc == 1)
		dialog_notify("CANCEL");
	    else
		dialog_notify("Ooops");
	}
	else if (strstr(result,"check")) {
	    rc = dialog_checklist("checklist", "checklist box",
				    -1, -1, 4, 7, checklist, result);
	    if (rc == 0)
		dialog_notify(result);
	    else if (rc == 1)
		dialog_notify("CANCEL");
	    else
		dialog_notify("Ooops");
	}
	else if (strstr(result,"radio")) {
	    rc = dialog_radiolist("radiolist", "radiolist box",
				    -1, -1, 4, 7, checklist, result);
	    if (rc == 0)
		dialog_notify(result);
	    else if (rc == 1)
		dialog_notify("CANCEL");
	    else
		dialog_notify("Ooops");
	}
	else if (strstr(result, "notify"))
	    dialog_notify("A Notification box");
	else if (strstr(result, "msg")) {
	    dialog_msgbox("msgbox", "Sleeping 2 seconds on a\n"
				    "msg box without user input", -1, -1, 0);
	    sleep(1);
	    dialog_msgbox("msgbox", "Sleeping 1 second on a\n"
				    "msg box without user input", -1, -1, 0);
	    sleep(1);
	    dialog_msgbox("msgbox", "A msg box that wants user input",
				    -1, -1, 1);
	}
	else if (strstr(result, "mesg"))
	    dialog_mesgbox("mesgbox", "A mesg box", -1, -1);
	else if (strstr(result, "yesno")) {
	    rc = dialog_yesno("yesno", "Test me\nPress YES or NO", -1, -1);

	    if (rc == 0)
		dialog_notify("You pressed YES");
	    else if (rc > 0)
		dialog_notify("You pressed NO");
	    else
		dialog_notify("Something Wicked Happened!");
	}
	else if (strstr(result, "gauge")) { 
	    int x;
	    for (x=0; x<=20; x++) {
		dialog_gauge("test", "gauge", -1, -1, -1, 40, x*5);
		usleep(300000);
	    }
	}
    }
    end_dialog();
    adump();
    exit((rc < 0) ? 1 : 0);
}

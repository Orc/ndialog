#include <ndialog.h>
#include <stdio.h>
#include <pwd.h>
#include <sys/types.h>


main()
{
    struct passwd *pw;
    char bfr[200];

    extern struct passwd *getuser(int);


    init_dialog();
    if ((pw = getuser(10)) == 0)
	Error("you've been a bad user");
    end_dialog();
    if (pw)
	printf("user: %s\n", ((struct passwd*)pw)->pw_name);
    exit(0);
}

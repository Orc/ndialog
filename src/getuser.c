/*
 * cc -c getuser.o getuser.c
 */
#include <ndialog.h>
#include <pwd.h>
#include <sys/types.h>
#include <unistd.h>

#define ESIZE		20
static char user[ESIZE];
static char pass[ESIZE];
static struct passwd *pw;
static int retries;

/*
 * pwcallback() is a local function that authenticates the user after they
 * enter a password.
 */
static int
pwcallback(void *obj)
{
    char *ency;

    if ((pw = getpwnam(user)) != 0) {
	if (pw->pw_passwd[0] == 0 && pass[0] == 0) {
	    /* no password, so don't even check */
	    return -1;
	}
	ency = crypt(pass, pw->pw_passwd);
	memset(pass, 0, ESIZE);

	if (strcmp(ency, pw->pw_passwd) == 0) {
	    /* password matched; we can leave now */
	    return -1;
	}
    }
    /* complain bitterly and return */
    Error("login incorrect");
    return (--retries >= 0) ? 0 : -1;
} /* pwcallback */


/*
 * getuser() asks for a username and password.  If it's given a valid username
 * and password, it returns a pwent for that user, otherwise it spits up a
 * `login incorrect' error box asks again, up to _login_attempts times.  After
 * that, it returns 0 and leaves the proper punishment up to the caller.
 */
struct passwd *
getuser(int login_attempts)
{
    int rc;
    void *u_obj, *p_obj, *cancel, *chain;

    /* allocate the display fields */
    u_obj = newString(  0,0, ESIZE, ESIZE, user, 0, "Username", 0, 0);
    p_obj = newPWString(0,3, ESIZE, ESIZE, pass, 0, "Password", pwcallback, 0);
    cancel= newCancelButton(0, "Forget it", 0, 0);
    retries = login_attempts;

    /* build them into a ObjChain */
    chain = ObjChain(ObjChain(u_obj, p_obj), cancel);

    /* get the user input and clean up */
    rc = MENU(chain, -1, -1, 0, "Enter your username and password", 0);
    deleteObjChain(chain);

    /* and return the user info if there is any */
    return (rc == MENU_OK) ? pw : 0;
} /* getuser */


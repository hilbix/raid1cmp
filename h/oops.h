/* Some routine wrappers which OOPS() on error
 */

#ifndef	OOPS

#include <stdio.h>
#include <errno.h>
#include <signal.h>
#include <stdlib.h>                                                                                                
#include <termios.h>
#include <time.h>
#include <unistd.h>

#include "cpptricks.h"

/* A,B,C,D,..	=> A B , C D , ..	*/
#define	Otwo(A...)		EVAL(FOR(Otwo,A))
#define	Otwo_0(A,B,C...)
#define	Otwo_1(A,B,C...)	A B
#define	Otwo_2(A,B,C...)	C
#define	Otwo_3(...)		,
#define	Otwo_4()

/* A,B,C,D,..	=> B , D , ..	*/
#define	O2nd(A...)		EVAL(FOR(O2nd,A))
#define	O2nd_0(A,B,C...)
#define	O2nd_1(A,B,C...)	B
#define	O2nd_2(A,B,C...)	C
#define	O2nd_3(...)		,
#define	O2nd_4()

/* Create our own checked wrappers from the prototypes
 * void Ofunctionname()
 * has the same calling signature as
 * int functioname()
 * but OOPSes when an error occurs.
 */
#define	O(A,B...)	static void O##A(Otwo(B)) { if (A(O2nd(B))) OOPS(#A); }
#define	I(A,B...)	static void O##A(Otwo(B)) { while (A(O2nd(B))) { if (errno!=EINTR) OOPS(#A); } }

/* Just bail out on OOPS() for now	*/

#define	OOPS(A...)	OOPS_(__FILE__, __LINE__, __func__, A, NULL)
#define	FATAL(A,B...)	do { if (A) OOPS_(__FILE__, __LINE__, __func__, #A, ##B, NULL); } while (0)

static void
OOPS_(const char *name, int line, const char *fn, const char *err, ...)
{
  fprintf(stderr, "OOPS %s:%d: in %s: ", name, line, fn);
  perror(err);
  exit(23); abort(); for(;;);
}

/* The wrappers needed	*/

O(sigemptyset, sigset_t *,sigset);
O(sigaction, int,signum, const struct sigaction *,act, struct sigaction *,oldact);
O(timer_create, clockid_t,clockid, struct sigevent *,sevp, timer_t *,timerid);
O(siginterrupt,int,sig, int,flag);
O(raise,int,sig);
O(sigprocmask,int,how, const sigset_t *,set, sigset_t *,oldset);
O(sigfillset,sigset_t *,set);
O(sigaddset,sigset_t *,set, int,signum);
O(sigdelset,sigset_t *,set, int,signum);
I(tcgetattr,int,fd, struct termios *,termios_p);
I(tcsetattr,int,fd, int,optional_actions, const struct termios *,termios_p);

#undef	O
#undef	I
#endif


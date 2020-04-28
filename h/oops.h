/* Some routine wrappers which OOPS() on error
 */

#ifndef	OOPS

#include <stdio.h>

#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
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
#define	C(A,B...)	A(B)
#define	O(A,B...)	static void O##A(Otwo(B)) { if (C(A,O2nd(B))) OOPS(#A "() failed"); }
#define	I(A,B...)	static void O##A(Otwo(B)) { while (C(A,O2nd(B))) { if (errno!=EINTR) OOPS(#A "() failed"); } }
#define	R(X,A,B...)	static X O##A(Otwo(B)) { X _; _=C(A,O2nd(B)); if (!_) OOPS(#A "() failed"); return _; }

/* Just bail out on OOPS() for now	*/

#define	OOPS(A...)	OOPS_(__FILE__, __LINE__, __func__, A, NULL)
#define	FATAL(A,B...)	do { if (A) OOPS(#A, ##B); } while (0)

static void
OOPS_(const char *name, int line, const char *fn, const char *err, const char *more, ...)
{
  int e	= errno;

  fprintf(stderr, "OOPS %s:%d", name, line);
  if (fn)
    fprintf(stderr, ": in %s", fn);
  if (more)
    {
      va_list	list;

      fprintf(stderr, ": ");
      va_start(list, more);
      vfprintf(stderr, more, list);
      va_end(list);
    }
  if (err)
    fprintf(stderr, ": %s", err);
  if (e)
    fprintf(stderr, ": %s", strerror(e));
  fprintf(stderr, "\n");
  fflush(stderr);
  exit(23);	/* run atexit() */
  /* should never return */
  abort();
  for(;;);
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
O(setenv,const char *,name, const char *,value, int,overwrite);
O(unsetenv,const char *,name);
R(struct tm *,gmtime_r,const time_t *,timep, struct tm *,result);
R(struct tm *,localtime_r,const time_t *,timep, struct tm *,result);
R(void *,malloc,size_t,size);
R(void *,realloc,void *,ptr,size_t,size);
R(char *,strdup,const char *,s);
R(char *,strndup,const char *,s, size_t,n);
O(stat,const char *,pathname, struct stat *,statbuf);
O(fstat,int,fd, struct stat *,statbuf);
O(lstat,const char *,pathname, struct stat *,statbuf);

static void Otime(time_t *t) { if (time(t)==(time_t)-1) OOPS("time() failed"); }

#undef	O
#undef	I
#endif


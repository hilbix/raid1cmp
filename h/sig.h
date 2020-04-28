/* We do not want nor need threads	*/

#include "oops.h"

struct sig_set
  {
    int			sig;	/* signal num	*/
    struct sigaction	sa;	/* signal state	*/
    sigset_t		mask;	/* program mask	*/
  };

typedef void sig_fn(int, siginfo_t *, void *);

#if 0
static void *
Osignal(int signum, void *handler)
{
  handler	= signal(signum, handler);
  FATAL(handler==SIG_ERR);
  return handler;
}
#endif

static void
sig_set(struct sig_set *st)
{
  Osigaction(st->sig, &st->sa, NULL);
  Osiginterrupt(st->sig, 1);
}

static void
sig_init(int sig, sig_fn *action, void *user)
{
  static struct sig_set	s;

  action(-1, NULL, user);	/* pass user pointer to handler	*/

  s.sig	= sig;
  Osigfillset(&s.sa.sa_mask);	/* block all other signals during handler	*/
  s.sa.sa_flags	= SA_SIGINFO;
  s.sa.sa_sigaction = action;
  sig_set(&s);
}

static void
sig_save(struct sig_set *old, int sig)
{
  FATAL(!old);
  old->sig	= sig;
  Osigaction(sig, NULL, &old->sa);
  Osigprocmask(SIG_UNBLOCK, NULL, &old->mask);
}

static void
sig_load(struct sig_set *old)
{
  FATAL(!old);
  Osigprocmask(SIG_SETMASK, &old->mask, NULL);
  sig_set(old);
}

static void
sig_raise(int sig, struct sig_set *old)
{
  sigset_t	msk;

  sig_save(old, sig);
  Oraise(sig);
  Osigemptyset(&msk);
  Osigaddset(&msk, sig);
  Osigprocmask(SIG_UNBLOCK, &msk, &old->mask);

  /* sig delivered here	*/
}

/* handle SIGTSTP	*/
static void
sig_tstp(void)
{
  struct sig_set	old;

  sig_raise(SIGTSTP, &old);

  /* resumes here	*/

  sig_load(&old);
}


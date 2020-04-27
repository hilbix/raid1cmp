/* We do not want nor need threads	*/

#include "oops.h"

struct sig_old
  {
    int		sig;
    void	*handler;
    sigset_t	mask;
  };

typedef void sig_fn(int, siginfo_t *, void *);

static void *
Osignal(int signum, void *handler)
{
  handler	= signal(signum, handler);
  FATAL(handler==SIG_ERR);
  return handler;
}

static void
sig_store(int sig, sig_fn *action)
{
  struct sigaction sa;

  Osigemptyset(&sa.sa_mask);
  sa.sa_flags	= SA_SIGINFO;
  sa.sa_sigaction = action;
  Osigaction(sig, &sa, NULL);
  Osiginterrupt(sig, 1);
}

static void
sig_set(int sig, sig_fn *action, void *user)
{
  action(-1, NULL, user);	/* pass user pointer to handler	*/
  sig_store(sig, action);
}

static void
sig_save(struct sig_old *old, int sig)
{
  FATAL(!old);
  old->sig	= sig;
  old->handler	= Osignal(sig, SIG_DFL);
  Osigprocmask(SIG_UNBLOCK, NULL, &old->mask);
}

static void
sig_load(int sig, struct sig_old *old)
{
  FATAL(!old);
  Osigprocmask(SIG_SETMASK, &old->mask, NULL);
  sig_store(old->sig, old->handler);
}

static void
sig_raise(int sig, struct sig_old *old)
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
  struct sig_old	old;

  sig_raise(SIGTSTP, &old);

  /* resumes here	*/

  sig_load(SIGTSTP, &old);
}


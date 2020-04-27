/* We do not want nor need threads	*/

#include "h/oops.h"
#include "h/tty-cbreak.h"
#include "h/sig.h"

#define	CONF	r1c_conf *C
typedef struct r1c_conf
  {
    timer_t	timer;
    int		timing;

    struct tty_cbreak	tty;
  } r1c_conf;

typedef void setup_sig_fn(int, siginfo_t *, void *);

static void
setup_sig(int sig, setup_sig_fn *action)
{
  struct sigaction sa = {0};

  Osigemptyset(&sa.sa_mask);
  sa.sa_flags	= SA_SIGINFO;
  sa.sa_sigaction = action;
  Osigaction(SIGALRM, &sa, NULL);
  Osiginterrupt(SIGALRM, 1);
}

static void
handle_timer(int sig, siginfo_t *so, void *u)
{
  static CONF;

  if (sig<0 && !so) C = u;
  C->timing++;
}

static void
handle_quit(int sig, siginfo_t *so, void *u)
{
  static CONF;

  if (sig<0 && !so) C = u;
  tty_orig(&C->tty);
}

static void
handle_stop(int sig, siginfo_t *so, void *u)
{
  static CONF;

  if (sig<0 && !so) C = u;
  tty_orig(&C->tty);
  sig_tstp();
  tty_own(&C->tty);
}

static void
setup_timer(CONF)
{
  setup_sig(SIGALRM, handle_timer);
  Otimer_create(CLOCK_MONOTONIC, NULL, &C->timer);
}

static void
setup_tty(CONF)
{
  const	int fd=0;

  if (!isatty(fd))
    return;

  setup_sig(SIGQUIT, handle_quit);
  setup_sig(SIGINT, handle_quit);
  setup_sig(SIGTSTP, handle_stop);

  tty_init(&C->tty, fd);
  tty_cbreak(&C->tty);
}

static void
loop(void)
{
  int		got;
  unsigned char	c;

  while ((got=read(0, &c, 1))!=0)
    {
      if (got<0)
        {
          int e = errno;
          putchar('.');
          if (e==EINTR)
            continue;
          OOPS("stdin");
        }
      fprintf(stderr, "got %d (%c)\n", c, c);
    }
}

int
main(int argc, char **argv)
{
  static struct r1c_conf conf;
  CONF = &conf;

  setup_timer(C);
  setup_tty(C);

  loop();
  000;

  return 0;
}


/* We do not want nor need threads	*/

#include "oops.h"

struct tty_attr
  {
    int			tty, own;
    struct termios	att, org;
  };

static void
tty_init(struct tty_attr *tty, int fd)
{
  tty->own	= 0;
  tty->tty	= fd;

  Otcgetattr(fd, &tty->org);
  tty->att	= tty->org;
}

static void
tty_orig(struct tty_attr *tty)
{
  if (!tty->own)
    return;
  tty->own	= 0;
  Otcsetattr(tty->tty, TCSADRAIN, &tty->org);
}

static void
tty_own(struct tty_attr *tty)
{
  Otcsetattr(tty->tty, TCSADRAIN, &tty->att);
  tty->own	= 1;
}

static void
tty_cbreak(struct tty_attr *tty)
{
  tty->att	= tty->org;
  tty->att.c_lflag	&= ~(ICANON|ECHO);
  tty->att.c_lflag	|=  (ISIG);
  tty->att.c_iflag	&= ~(ICRNL);
  tty->att.c_cc[VMIN]	=  1;
  tty->att.c_cc[VTIME]=  0;

  tty_own(tty);
}

static void
tty_raw(struct tty_attr *tty)
{
  tty->att	= tty->org;
  tty->att.c_lflag	&= ~(ICANON|ECHO|ISIG|IEXTEN);
  tty->att.c_iflag	&= ~(ICRNL|BRKINT|IGNBRK|IGNCR|INLCR|INPCK|ISTRIP|IXON|PARMRK);
  tty->att.c_oflag	&= ~(OPOST);
  tty->att.c_cc[VMIN]	=  1;
  tty->att.c_cc[VTIME]=  0;

  tty_own(tty);
}


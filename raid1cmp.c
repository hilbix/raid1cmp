/* We do not want nor need threads	*/

#define	_GNU_SOURCE

#include <stdint.h>
#include <ctype.h>

#include "h/oops.h"
#include "h/tty-attr.h"
#include "h/sig.h"

#define	KiB1	(1024)
#define	KiB4	(4096)
#define	MiB128	(128*1024*1024)
#define	GiB1	(1024*1024*1024)

typedef struct r1c_dev r1c_dev;
struct r1c_dev
  {
    r1c_dev	*next;
    int		fd;
    int		directio;
    uint64_t	blocks;
    const char	*name;
    int		nr;
    struct stat	stat;
  };

#define	CONF	r1c_conf *C
typedef struct r1c_conf
  {
    uint64_t	round, block, errors, mismatches;

    uint64_t	blocks;
    const char	*directio_str, *dev_str;

    r1c_dev	*dev;

    timer_t	timer;
    int		timing;

    struct tty_attr	tty;

    uint64_t	blocksize, blockcount, chunksize;
    const char	*directio;
  } r1c_conf;


/*
 * Helpers
 */

static void *
alloc0(size_t len)
{
  void *ptr;

  ptr	= Omalloc(len);
  memset(ptr, 0, len);
  return ptr;
}


/*
 * Handlers
 */

static void
handle_timer(int sig, siginfo_t *so, void *u)
{
  static CONF;

  if (sig<0 && !so) { C = u; return; }
  exit(66);
  C->timing++;
}

static void
handle_quit(int sig, siginfo_t *so, void *u)
{
  static CONF;

  if (sig<0 && !so) { C = u; return; }
  exit(77);
  tty_orig(&C->tty);
  sig_raise(sig, NULL);
}

static void
handle_stop(int sig, siginfo_t *so, void *u)
{
  static CONF;

  if (sig<0 && !so) { C = u; return; }
  exit(88);
  tty_orig(&C->tty);
  sig_tstp();
  tty_own(&C->tty);
}

static void
init_timer(CONF)
{
  sig_init(SIGALRM, handle_timer, C);
  Otimer_create(CLOCK_MONOTONIC, NULL, &C->timer);
}

struct tty_attr	*switchback_arg;

static void
switchback(void)
{
  tty_orig(switchback_arg);
}

static void
init_tty(CONF)
{
  const	int fd=0;

  if (!isatty(fd))
    return;

  sig_init(SIGTERM, handle_quit, C);
  sig_init(SIGQUIT, handle_quit, C);
  sig_init(SIGINT, handle_quit, C);
  sig_init(SIGTSTP, handle_stop, C);

  tty_init(&C->tty, fd);
  tty_cbreak(&C->tty);

  switchback_arg	= &C->tty;
  atexit(switchback);
}

/*
 * Logging
 */

static void
init_log(CONF)
{
  tzset();		/* POSIX requires tzset() before localtime_r()	*/
}

static void
put(CONF, FILE *out, int round, const char *msg, va_list list)
{
  char		buf[200];
  time_t	t;
  struct tm	tm;

  Otime(&t);
  strftime(buf, sizeof buf, "%Y%m%d %H%M%S", Olocaltime_r(&t, &tm));
  fprintf(out, "%s %d ", buf, round);
  vfprintf(out, msg, list);
  fprintf(out, "\n");
}

static void
info(CONF, const char *s, ...)
{
  va_list	list;

  va_start(list, s);
  put(C, stderr, 0, s, list);
  va_end(list);
  fflush(stderr);
}

/*
 * ENV and arguments
 */

static void
infs(CONF, const char *what, const char *s)
{
  char	str[100];

  if (!s) s="";
  snprintf(str, sizeof str, "THE%s", what);
  Osetenv(str, s, 1);
  info(C, "%20s=%s", what, s);
}

static void
infu(CONF, const char *what, uint64_t u)
{
  char	buf[22];

  snprintf(buf, sizeof buf, "%llu", (unsigned long long)u);
  infs(C, what, buf);
}

static void
inf(CONF)
{
  infu(C, "BLOCKSIZE",	C->blocksize);
  infu(C, "BLOCKCOUNT",	C->blockcount);
  infu(C, "CHUNKSIZE",	C->chunksize);
  infs(C, "DIRECTIO",	C->directio_str);
  infu(C, "PID",	(uint64_t)getpid());
  infs(C, "DEVS",	C->dev_str);
  infu(C, "BLOCK",	C->block);
  infu(C, "BLOCKCNT",	C->blocks);
  infu(C, "POS",	C->block*C->blocksize);
  infu(C, "ERRORCNT",	C->errors);
  infu(C, "ROUND",	C->round);
  infu(C, "MISMATCHCNT",C->mismatches);
}

static uint64_t
env64(const char *key)
{
  const char	*val;

  for (val=getenv(key); val && *val; val++)
    {
      int	ret;
      char	*end;

      if (isspace(*val) || *val=='0')	/* ignore leading SPC or 0	*/
        continue;
      if (*val=='x' || *val=='$')	/* xHEX, 0xHEX, $HEX, 0$HEX	*/
        ret	= strtoull(val+1, &end, 16);
      else
        ret	= strtoull(val, &end, 10);
      if (end && !*end)
        return ret;
    }
  return 0;
}

static void
init_env(CONF)
{
  uint64_t	tmp;
  const char	*s;

  tmp	= env64("BLOCKSIZE");
  while (tmp&(tmp-1))
    tmp	&= tmp-1;
  if (!tmp)
    tmp	= KiB4;
  if (tmp>GiB1)
    tmp	= GiB1;
  C->blocksize	= tmp;

  tmp	= env64("CHUNKSIZE");
  if (!tmp)
    tmp	= MiB128;
  if (tmp<KiB1)
    tmp	= KiB1;
  if (tmp<C->blocksize)
    tmp	= C->blocksize;
  if (tmp%C->blocksize)
    tmp	-= tmp%C->blocksize;
  if (tmp>GiB1)
    tmp	= GiB1;
  C->chunksize	= tmp;

  tmp	= env64("BLOCKCOUNT");
  if (!tmp)
    tmp	= C->chunksize / C->blocksize;
  if (tmp * C->blocksize > GiB1)
    tmp	= GiB1 / C->blocksize;
  C->blockcount	= tmp;
  C->chunksize	= tmp * C->blocksize;

  s	= getenv("DIRECTIO");
  if (s && !*s)
    s	= 0;
  if (s)
    s	= Ostrdup(s);
  C->directio	= s; 
}

/* return 1 if name uses DIRECTIO	*/
static int
check_directio(CONF, const char *name)
{
  size_t	len;
  char		*pos;

  if (!C->directio)
    return 1;

  pos	= strstr(C->directio, name);
  if (!pos)
    return 0;

  len	= strlen(name);
  return (!pos[len] || isspace(pos[len])) && (pos==C->directio || isspace(pos[-1]));
}

static void
ensure_unique_dev(CONF, r1c_dev *dev)
{
  r1c_dev	*p;

  Ofstat(dev->fd, &dev->stat);
  for (p=C->dev; p; p=p->next)
    {

      if (!strcmp(dev->name, p->name))
        OOPS("please use unique names", "dev %d and dev %d have the same name", p->nr, dev->nr);
    }
}

static void
init_dev(CONF, const char * const *args)
{
  r1c_dev	**p;
  int		nr;

  p	= &C->dev;
  for (nr=1; *args; nr++)
    {
      r1c_dev	*dev;
      uint64_t	len;

      dev		= alloc0(sizeof *dev);
      dev->nr		= nr;
      dev->name		= Ostrdup(*args++);
      dev->directio	= check_directio(C, dev->name);
      dev->fd		= open(dev->name, O_RDONLY|O_CLOEXEC|O_NOFOLLOW|(dev->directio ? O_DIRECT : 0));
      if (dev->fd<0)
        OOPS(dev->name, "cannot open");

      len		= lseek(dev->fd, (off_t)0, SEEK_END);
      if (len % C->blocksize)
      info(C, "dev %s directio %d size %llu not multiple of blocksize %llu: %llu remainder", dev->name, dev->directio, (unsigned long long)len, (unsigned long long)C->blocksize, (unsigned long long)(len%C->blocksize));
      dev->blocks	= len/C->blocksize;
      info(C, "dev %s directio %d blocks %llu", dev->name, dev->directio, dev->blocks);

      ensure_unique_dev(C, dev);

      dev->next		= *p;
      *p		= dev;
      p			= &dev->next;
    }
  if (!C->dev || !C->dev->next)
    OOPS("too few devices to operate on");

}

/*
 * Main
 */

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

static void
raid1cmp(CONF)
{
  while (C->block < C->blocks)
    {
      OOPS("NY");
    }
}

int
main(int argc, const char * const *argv)
{
  static struct r1c_conf conf;	/* this is NULled	*/
  CONF = &conf;

  init_log(C);
  init_timer(C);
  init_tty(C);
  init_env(C);
  init_dev(C, argv+1);

  inf(C);

  raid1cmp(C);

  inf(C);

  return 0;
}


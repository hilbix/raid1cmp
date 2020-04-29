/* Easy buffers and strings
 *
 * This could be optimized in future using subbuffers
 */

#include "oops.h"

#define	B		_BUF_s02340202k03_donotuse_
#define	C		CAT(B,chunk)
#define	F(X,Y...)	CAT(X,B)(Y)

typedef struct C *C;
struct C
  {
    C		next;
    long	a, b, len;
    char	buf[0];
  };
typedef struct B
  {
    C		tail, head;
    long	total;
  } *B;
typedef struct { B B; } BUF;	/* real interface	*/

#define	ALLOC(c)	(c = Omalloc(sizeof *c))

/* create a new empty bucket	*/
static C
F(newc, int len)
{
  C	c;
  const int min	= 60-sizeof *c;

  if (len<min)
    len	= min;
  c	= Omalloc(len + sizeof *c);
  c->len= len;
  c->a	= 0;
  c->b	= 0;
  return c;
}

/* get the next nonempty bucket	*/
static C
F(next, C p)
{
  if (!p->next)
    return 0;
 for (;;)
   {
     C c = p->next;

     if (c->b > c->a)
       return c;

     if (!c->next)
       {
         c->a = c->b = 0;
         return c;	/* Empty last bucket, don't touch	*/
       }

     p->next	= c->next;
     free(c);
   }
}

/* get the head's first nonempty bucket	*/
static C
F(head, B b)
{
  C c;

  while ((c = b->head)!=0)
    {
      if (c->a < c->b)
        return c;

      /* release empty first bucket	*/
      b->head	= c->next;
      free(c);
    }

  /* emptied all	*/
  FATAL(b->tail != c, "BUG: broken BUF chain");
  b->tail = 0;
  FATAL(b->total, "BUG: unclean empty BUF");
  return 0;
}

/* expand the BUF chain the given bytes, at least
 *
 * only call this after F(room) reports too few space
 */
static void
F(expand, B b, int len)
{
  C	c;

  if ((c=b->tail)!=0)
    {
      FATAL(!b->head, "BUG: inkonsistent BUF");

      /* nonempty first bucket, append a bit more	*/
      if (F(head, b) && len < b->total/4)
        len	= b->total/4;
    }

  /* append a fresh and empty to the tail	*/
  c		= F(newc, len);
  if (b->tail)
    {
      FATAL(!b->head, "BUG: broken BUF chain");
      b->tail->next= c;
    }
  else
    {
      FATAL(b->head || b->total, "BUG: unclean empty BUF");
      b->head	= c;
    }
  b->tail	= c;
}

/* find out how much room is left in the last bucket	*/
static long
F(room, B b)
{
  C c;

  if (!(c=b->tail))
    return 0;

  if (!c->b)
    return c->len;

  /* check the last bucket for being empty	*/
  if (c->b <= c->a)
    c->a = c->b = 0;

  return c->len - c->b;
}

/* append data to the bucket chain	*/
static void
F(add, B b, const void *ptr, size_t len)
{
  long	left;

  left	= F(room, b);
  if (left)
    {
      if (left>len)
        left	= len;
      memcpy(b->tail->buf + b->tail->b, ptr, left);
      ptr		= ((char *)ptr)+left;
      b->total		+= left;
      b->tail->b	+= left;
      len		-= left;
      if (!len)
        return;
    }
  /* the last bucket is full and we still have data	*/
  F(expand, b, len);
  b->total	+= len;
  b->tail->b	= len;
  memcpy(b->tail->buf, ptr, len);
}

/* clean the BUF, this means put everyting into a single bucket
 *
 * This possibly leaves an completely empty BUF
 */
static void
F(clean, B b)
{
  C	c;
  long	tot;

  c = F(head, b);
  if (!c)
    return;

  tot		= b->total;
  b->total	= 0;
  b->head	=
  b->tail	= F(newc, tot+tot/4+1);

  do
    {
      C	d;
      if (c->a < c->b)
        F(add, b, c->buf+c->a, c->b - c->a);
      d	= c;
      c	= c->next;
      free(d);
    } while (c);
  FATAL(tot != b->total, "BUF size mismatch, expected %lu got %lu", tot, b->total);
}

/*
 * Public Interface
 */

static BUF
Bnew(void)
{
  BUF	ret;
  B 	b;

  b		= Omalloc(sizeof *b);
  b->tail	= 0;
  b->head	= 0;
  b->total	= 0;
  ret.B		= b;
  return ret;
}

static BUF
Breset(BUF _)
{
  B b = _.B;
  C c;

  FATAL(!b, "BUF not initialized with Bnew()");
  for (c=F(head, b); c; c=F(next, c))
    c->a = c->b = 0;
  return _;
}

/* add a single char to BUF	*/
static BUF
Baddc(BUF _, char c)
{
  B b = _.B;

  if (!F(room, b))
    F(expand, b, 1);
  b->tail->buf[b->tail->b++] = c;
  b->total++;
  return _;
}

/* add a memory area to BUF	*/
static BUF
Badd(BUF _, const void *ptr, size_t len)
{
  if (!len)
    return _;
  FATAL(!ptr, "NULL pointer with length %llu", (unsigned long long)len);
  F(add, _.B, ptr, len);
  return _;
}

/* add a string to BUF	*/
static BUF
Badds(BUF _, const char *a)
{
  return a ? Badd(_, a, strlen(a)) : _;
}

/* add a BUF to a BUF */
static BUF
BaddB(BUF _, BUF a)
{
  B	b = _.B;
  C	c;
  int	left;

  left	= F(room, b);
  if (left < a.B->total)
    F(expand, b, a.B->total - left);

  for (c=F(head, a.B); c; c=F(next, c))
    F(add, b, c->buf+c->a, c->b-c->a);

  return _;
}

/* fetch entire buffer as string	*/
static const char *
Bgets(BUF _)
{
  B b = _.B;

  if (b->head != b->tail || !F(room, b))
    F(clean, b);	/* leaves at least 1 byte	*/
  if (!b->tail)
    F(expand, b, 1);	/* empty BUF	*/
  b->tail->buf[b->tail->b] = 0;
  return b->tail->buf + b->tail->a;
}

/* Get the size of data in BUF	*/
static size_t
Blen(BUF _)
{
  return _.B->total;
}

#undef	ALLOC
#undef	F
#undef	C
#undef	B


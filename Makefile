#

PROG=raid1cmp
CFLAGS=-Wall -O3 -Wno-unused-function
LDLIBS=-lrt

.PHONY:	love
love:	all

.PHONY:	all
all:	$(PROG)

.PHONY:	clean
clean:
	$(RM) $(PROG)

$(PROG).o:	$(wildcard h/*.h)


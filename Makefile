#
#
#

LIBRARY := lnet.a
LIBDBG := lnet-dbg.a

SOURCES := src/net.c

#
#
#

OBJECTS := ${SOURCES:.c=.o}
DBGOBJS := ${SOURCES:.c=.d}

default: ${LIBRARY}

all: ${LIBRARY} ${LIBDBG}

debug: ${LIBDBG}

clean: 
	/bin/rm -f ${LIBRARY} ${LIBDBG} ${OBJECTS} ${DBGOBJS}


${LIBRARY}: ${OBJECTS}
	ar -rcs $@ $^

${LIBDBG}: ${DBGOBJS}
	ar -rcs $@ $^

%.o : %.c
	gcc -c -o $@ $^

%.d : %.c
	gcc -g -D DEBUG -c -o $@ $^

%.c : %.h


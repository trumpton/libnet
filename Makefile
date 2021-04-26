#
#
#

default: net.o net.d

clean: 
	/bin/rm -f net.o net.d


%.o : %.c
	gcc -c -o $@ $^

%.d : %.c
	gcc -g -D DEBUG -c -o $@ $^

%.c : %.h


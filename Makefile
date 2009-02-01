
all: tt-demo

.c.o:
	gcc -Wall -g -O2 -c $<

tinytest.o: tinytest.h

tinytest_demo.o: tinytest_macros.h tinytest.h

OBJS=tinytest.o tinytest_demo.o

tt-demo: $(OBJS)
	gcc -Wall -g -O2 $(OBJS) -o tt-demo

lines:
	wc -l tinytest.c tinytest_macros.h tinytest.h

clean:
	rm -f *.o *~ tt-demo
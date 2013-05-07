CC = gcc 
CFLAGS = -Wall -W -Wshadow -std=gnu99 
TARGETS = traceroute
 
all: $(TARGETS)

traceroute: traceroute.o sockwrap.o icmp.o net.o

clean:
	rm -f *.o	

distclean: clean
	rm -f $(TARGETS)

FUSE_LIBS=$(shell pkg-config fuse --libs)
VOLD_LIBS=$(shell pkg-config voldemort --libs)
FUSE_FLAG=$(shell pkg-config fuse --cflags)
VOLD_FLAG=$(shell pkg-config fuse --cflags)

khan : khan.o log.o
	g++ -L/usr/local/lib -L/usr/lib khan.o log.o -o khan -lfuse -lvoldemort -lrt -lhiredis

khan.o : khan.cpp log.h khan.h 
	g++ $(FUSE_FLAG) $(VOLD_FLAG) -c khan.cpp

log.o : log.cpp log.h 
	g++ $(FUSE_FLAG) $(VOLD_FLAG) -c log.cpp

clean :
	rm -f khan *.o
	rm -rf logFile
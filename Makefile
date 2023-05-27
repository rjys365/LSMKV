
LINK.o = $(LINK.cc)
CXXFLAGS = -std=c++14 -Wall -g

all: correctness persistence

correctness: kvstore.o correctness.o

persistence: kvstore.o persistence.o

clean:
	-rm -f correctness persistence *.o

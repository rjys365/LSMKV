
LINK.o = $(LINK.cc)
CXXFLAGS = -std=c++14 -Wall -g

all: correctness persistence benchmark1

correctness: kvstore.o correctness.o

persistence: kvstore.o persistence.o

benchmark1: kvstore.o benchmark1.o

clean:
	-rm -f correctness persistence benchmark1 *.o

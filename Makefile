
LINK.o = $(LINK.cc)
CXXFLAGS = -std=c++14 -Wall -g

all: correctness persistence benchmark1 benchmark2 benchmark3

correctness: kvstore.o correctness.o

persistence: kvstore.o persistence.o

benchmark1: kvstore.o benchmark1.o

benchmark2: kvstore.o benchmark2.o

benchmark3: kvstore.o benchmark3.o

clean:
	-rm -f correctness persistence benchmark1 benchmark2 benchmark3 *.o

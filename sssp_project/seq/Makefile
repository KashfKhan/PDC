CC = g++
CFLAGS = -O2 -std=c++17
LDFLAGS =

all: sssp_sequential

sssp_sequential: sssp_sequential.o graph_loader.o
	$(CC) $(CFLAGS) $(LDFLAGS) -o sssp_sequential sssp_sequential.o graph_loader.o

sssp_sequential.o: sssp_sequential.cpp graph_loader.h
	$(CC) $(CFLAGS) -c sssp_sequential.cpp

graph_loader.o: graph_loader.cpp graph_loader.h
	$(CC) $(CFLAGS) -c graph_loader.cpp

clean:
	rm -f *.o sssp_sequential

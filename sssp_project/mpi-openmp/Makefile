CC = mpic++
CFLAGS = -O2 -std=c++17 -fopenmp
LDFLAGS = -fopenmp

all: sssp_mpi

sssp_mpi: main.o graph_loader.o sssp_mpi.o
	 $(CC) $(LDFLAGS) -o sssp_mpi main.o graph_loader.o sssp_mpi.o

main.o: main.cpp sssp_mpi.h graph_loader.h
	$(CC) $(CFLAGS) -c main.cpp

graph_loader.o: graph_loader.cpp graph_loader.h
	$(CC) $(CFLAGS) -c graph_loader.cpp

sssp_mpi.o: sssp_mpi.cpp sssp_mpi.h
	$(CC) $(CFLAGS) -c sssp_mpi.cpp

clean:
	rm -f *.o sssp_mpi

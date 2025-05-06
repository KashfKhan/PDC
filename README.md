SSSP Project
This project implements the Single-Source Shortest Paths (SSSP) problem with dynamic updates in three versions: Sequential, MPI, and MPI+OpenMP. The implementations are designed to run on the UTM cluster, handling large graphs like the Facebook dataset.
Project Structure

sssp_project/seq/: Sequential implementation.
sssp_project/mpi/: MPI-based distributed implementation.
sssp_project/mpi-openmp/: Hybrid MPI+OpenMP implementation.

Sequential Implementation
Location
sssp_project/seq/
Files

sssp_sequential.cpp: Main program for sequential SSSP with dynamic updates.
graph_loader.h, graph_loader.cpp (assumed): Graph loading utilities.
Makefile: Build script.

Makefile
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

How to Run on UTM Cluster

Transfer Files:scp -r sssp_project/seq username@utm-cluster:/path/to/destination


Log in:ssh username@utm-cluster


Load Modules:module load gcc


Compile:cd /path/to/sssp_project/seq
make


Prepare Input Files:Ensure facebook_combined.txt and facebook_graph.txt.part.8 are in /mirror:cp facebook_combined.txt facebook_graph.txt.part.8 /mirror/


Submit Job (run_seq.sh):#!/bin/bash
#SBATCH --job-name=sssp_sequential
#SBATCH --nodes=1
#SBATCH --ntasks=1
#SBATCH --time=01:00:00
#SBATCH --mem=8GB
#SBATCH --output=sssp_seq_%j.out
#SBATCH --error=sssp_seq_%j.err

./sssp_sequential

sbatch run_seq.sh


Check Output:squeue -u $USER
cat sssp_seq_%j.out



Precautions

Verify /mirror exists and is writable, or modify file paths in the code.
Adjust --mem and --time based on graph size.
Ensure GCC supports C++17 (e.g., module load gcc/9.3.0 if needed).

MPI Implementation
Location
sssp_project/mpi/
Files

main.cpp: Main MPI program.
sssp_mpi.h, sssp_mpi.cpp: Core SSSP functions.
graph_loader.h, graph_loader.cpp: Graph loading utilities.
Makefile: Build script.

Makefile
CC = mpicxx
CFLAGS = -O2 -std=c++17
LDFLAGS =

all: sssp_mpi

sssp_mpi: main.o sssp_mpi.o graph_loader.o
	$(CC) $(CFLAGS) $(LDFLAGS) -o sssp_mpi main.o sssp_mpi.o graph_loader.o

main.o: main.cpp sssp_mpi.h graph_loader.h
	$(CC) $(CFLAGS) -c main.cpp

sssp_mpi.o: sssp_mpi.cpp sssp_mpi.h graph_loader.h
	$(CC) $(CFLAGS) -c sssp_mpi.cpp

graph_loader.o: graph_loader.cpp graph_loader.h
	$(CC) $(CFLAGS) -c graph_loader.cpp

clean:
	rm -f *.o sssp_mpi

How to Run on UTM Cluster

Transfer Files:scp -r sssp_project/mpi username@utm-cluster:/path/to/destination


Log in:ssh username@utm-cluster


Load Modules:module load mpi
module load gcc


Compile:cd /path/to/sssp_project/mpi
make


Prepare Input Files:Ensure test_graph.txt and test_graph.txt.part.8 are in /mirror:cp test_graph.txt test_graph.txt.part.8 /mirror/


Submit Job (run_mpi.sh):#!/bin/bash
#SBATCH --job-name=sssp_mpi
#SBATCH --nodes=2
#SBATCH --ntasks-per-node=4
#SBATCH --time=01:00:00
#SBATCH --mem=8GB
#SBATCH --output=sssp_mpi_%j.out
#SBATCH --error=sssp_mpi_%j.err

module load mpi
mpirun -np 8 ./sssp_mpi

sbatch run_mpi.sh


Check Output:squeue -u $USER
cat sssp_mpi_%j.out



Precautions

Ensure test_graph.txt is formatted for a 6-vertex graph.
Adjust mpirun -np to match the partition file and cluster capacity.
Verify MPI module compatibility (e.g., OpenMPI or MPICH).
Monitor MPI_Allreduce performance for large graphs.

MPI+OpenMP Implementation
Location
sssp_project/mpi-openmp/
Files

main.cpp: Main hybrid program.
sssp_mpi.h, sssp_mpi.cpp: Core SSSP functions with OpenMP.
graph_loader.h, graph_loader.cpp: Graph loading utilities.
Makefile: Build script.

Makefile
CC = mpicxx
CFLAGS = -O2 -std=c++17 -fopenmp
LDFLAGS = -fopenmp

all: sssp_mpi_openmp

sssp_mpi_openmp: main.o sssp_mpi.o graph_loader.o
	$(CC) $(CFLAGS) $(LDFLAGS) -o sssp_mpi_openmp main.o sssp_mpi.o graph_loader.o

main.o: main.cpp sssp_mpi.h graph_loader.h
	$(CC) $(CFLAGS) -c main.cpp

sssp_mpi.o: sssp_mpi.cpp sssp_mpi.h graph_loader.h
	$(CC) $(CFLAGS) -c sssp_mpi.cpp

graph_loader.o: graph_loader.cpp graph_loader.h
	$(CC) $(CFLAGS) -c graph_loader.cpp

clean:
	rm -f *.o sssp_mpi_openmp

How to Run on UTM Cluster

Transfer Files:scp -r sssp_project/mpi-openmp username@utm-cluster:/path/to/destination


Log in:ssh username@utm-cluster


Load Modules:module load mpi
module load gcc


Compile:cd /path/to/sssp_project/mpi-openmp
make


Prepare Input Files:Ensure facebook_graph.txt and facebook_graph.txt.part.8 are in /mirror:cp facebook_graph.txt facebook_graph.txt.part.8 /mirror/


Submit Job (run_mpi_openmp.sh):#!/bin/bash
#SBATCH --job-name=sssp_mpi_openmp
#SBATCH --nodes=2
#SBATCH --ntasks-per-node=4
#SBATCH --cpus-per-task=4
#SBATCH --time=01:00:00
#SBATCH --mem=16GB
#SBATCH --output=sssp_mpi_openmp_%j.out
#SBATCH --error=sssp_mpi_openmp_%j.err

module load mpi
export OMP_NUM_THREADS=4
mpirun -np 8 ./sssp_mpi_openmp

sbatch run_mpi_openmp.sh


Check Output:squeue -u $USER
cat sssp_mpi_openmp_%j.out



Precautions

Ensure /mirror is accessible and input files are correctly formatted.
Set --cpus-per-task=4 to match OMP_NUM_THREADS=4.
Adjust --mem and --time for larger graphs.
Verify MPI and OpenMP compatibility with cluster modules.
Monitor synchronization overhead from MPI_Allreduce and OpenMP critical sections.

General Notes

Input Files: Place input files in /mirror or modify code paths if /mirror is unavailable.
Cluster Policies: Check UTM cluster documentation for job limits and resource constraints.
Performance: The MPI+OpenMP version may require tuning (e.g., thread count, process count) for optimal performance.
Debugging: Check .err files for errors related to file access or resource limits.


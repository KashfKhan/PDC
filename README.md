# 🔍 SSSP Project

This repository implements the **Single-Source Shortest Paths (SSSP)** problem with **dynamic updates**, in three versions:

- **Sequential**
- **MPI (Distributed)**
- **MPI + OpenMP (Hybrid)**

These implementations are designed for execution on the **UTM computing cluster** and handle large graphs such as the Facebook dataset.

---

## 📁 Project Structure

```
sssp_project/
├── seq/           # Sequential implementation
├── mpi/           # MPI-based distributed implementation
└── mpi-openmp/    # Hybrid MPI + OpenMP implementation
```

---

## 🧭 Table of Contents

- [Sequential Implementation](#-sequential-implementation)
- [MPI Implementation](#-mpi-implementation)
- [MPI + OpenMP Implementation](#-mpi--openmp-implementation)
- [General Notes](#-general-notes)

---

## 🚀 Sequential Implementation

### 📂 Location
`sssp_project/seq/`

### 📄 Files
- `sssp_sequential.cpp` — Main SSSP logic with dynamic updates  
- `graph_loader.cpp`, `graph_loader.h` — Graph parsing utilities  
- `Makefile` — Build script  

### 🛠️ Makefile
<details>
<summary>Click to expand</summary>

```makefile
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
```
</details>

### 🧪 Running on UTM Cluster

```bash
# Transfer files
scp -r sssp_project/seq username@utm-cluster:/path/to/destination

# Login to cluster
ssh username@utm-cluster

# Load necessary modules
module load gcc

# Compile
cd /path/to/sssp_project/seq
make

# Copy input files to /mirror
cp facebook_combined.txt facebook_graph.txt.part.8 /mirror/
```

#### 📝 Submit Job (`run_seq.sh`)
```bash
#!/bin/bash
#SBATCH --job-name=sssp_sequential
#SBATCH --nodes=1
#SBATCH --ntasks=1
#SBATCH --time=01:00:00
#SBATCH --mem=8GB
#SBATCH --output=sssp_seq_%j.out
#SBATCH --error=sssp_seq_%j.err

./sssp_sequential
```
```bash
sbatch run_seq.sh
```

#### 📈 Check Output
```bash
squeue -u $USER
cat sssp_seq_%j.out
```

---

## 🌐 MPI Implementation

### 📂 Location
`sssp_project/mpi/`

### 📄 Files
- `main.cpp` — MPI entry point  
- `sssp_mpi.cpp`, `sssp_mpi.h` — SSSP logic using MPI  
- `graph_loader.*` — Graph utilities  
- `Makefile` — Build instructions  

### 🛠️ Makefile
<details>
<summary>Click to expand</summary>

```makefile
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
```
</details>

### 🧪 Running on UTM Cluster

```bash
scp -r sssp_project/mpi username@utm-cluster:/path/to/destination
ssh username@utm-cluster

module load mpi
module load gcc

cd /path/to/sssp_project/mpi
make

cp test_graph.txt test_graph.txt.part.8 /mirror/
```

#### 📝 Submit Job (`run_mpi.sh`)
```bash
#!/bin/bash
#SBATCH --job-name=sssp_mpi
#SBATCH --nodes=2
#SBATCH --ntasks-per-node=4
#SBATCH --time=01:00:00
#SBATCH --mem=8GB
#SBATCH --output=sssp_mpi_%j.out
#SBATCH --error=sssp_mpi_%j.err

module load mpi
mpirun -np 8 ./sssp_mpi
```
```bash
sbatch run_mpi.sh
```

#### 📈 Check Output
```bash
squeue -u $USER
cat sssp_mpi_%j.out
```

---

## 🧵 MPI + OpenMP Implementation

### 📂 Location
`sssp_project/mpi-openmp/`

### 📄 Files
- `main.cpp` — Hybrid parallel main program  
- `sssp_mpi.*` — Core MPI + OpenMP implementation  
- `graph_loader.*` — Graph loader  
- `Makefile` — Compilation rules  

### 🛠️ Makefile
<details>
<summary>Click to expand</summary>

```makefile
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
```
</details>

### 🧪 Running on UTM Cluster

```bash
scp -r sssp_project/mpi-openmp username@utm-cluster:/path/to/destination
ssh username@utm-cluster

module load mpi
module load gcc

cd /path/to/sssp_project/mpi-openmp
make

cp facebook_graph.txt facebook_graph.txt.part.8 /mirror/
```

#### 📝 Submit Job (`run_mpi_openmp.sh`)
```bash
#!/bin/bash
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
```
```bash
sbatch run_mpi_openmp.sh
```

#### 📈 Check Output
```bash
squeue -u $USER
cat sssp_mpi_openmp_%j.out
```

---

## ⚠️ General Notes

- **Input Location:** Place all input files in `/mirror`, or update paths in the code.
- **Resource Allocation:** Tune `--mem`, `--time`, and parallelism based on graph size.
- **Compatibility:** Ensure proper module versions for **GCC**, **MPI**, and **OpenMP**.
- **Debugging:** Check `.err` files if jobs fail or hang.
- **Performance Tips:**
  - Sequential: Optimize with compiler flags
  - MPI: Tune `-np` to match dataset partitions
  - MPI+OpenMP: Match `--cpus-per-task` with `OMP_NUM_THREADS`

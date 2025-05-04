#include <mpi.h>
#include <iostream>
#include <vector>
#include <set>
#include <tuple>
#include <limits>
#include <fstream>
#include "sssp_mpi.h"
#include "graph_loader.h"

int main(int argc, char** argv) {
    MPI_Init(&argc, &argv);

    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    std::string graphFile = "/mirror/test_graph.txt";
    std::string partFile = "/mirror/test_graph.txt.part.8";
    std::cout << "Rank " << rank << ": Reading graph from " << graphFile << ", partition from " << partFile << "\n";
    Graph graph = load_partitioned_graph(graphFile, partFile, rank);

    int num_vertices = 0;
    std::ifstream pfile(partFile);
    if (!pfile) {
        std::cerr << "Rank " << rank << ": Error opening partition file " << partFile << "\n";
        MPI_Abort(MPI_COMM_WORLD, 1);
    }
    std::string line;
    while (std::getline(pfile, line)) {
        if (!line.empty()) {
            num_vertices++;
        }
    }
    pfile.close();
    std::cout << "Rank " << rank << ": num_vertices = " << num_vertices << "\n";

    if (num_vertices != 6) {
        std::cerr << "Rank " << rank << ": Error: Expected 6 vertices, got " << num_vertices << "\n";
        MPI_Abort(MPI_COMM_WORLD, 1);
    }

    const int INF = std::numeric_limits<int>::max();
    std::vector<int> Dist(num_vertices, INF);
    std::vector<int> Parent(num_vertices, -1);
    std::vector<int> AffectedDel(num_vertices, 0);
    std::vector<int> Affected(num_vertices, 0);
    std::vector<std::tuple<int, int, int>> Gu;
    std::set<std::pair<int, int>> Tree;

    std::cout << "Rank " << rank << ": Initializing tree\n";
    if (rank == 0) {
        Dist[0] = 0;
        Dist[1] = 1;
        Dist[2] = 3;
        Dist[3] = 4;
        Dist[4] = 7;
        Dist[5] = 8;

        Tree.insert({0, 1});
        Tree.insert({1, 2});
        Tree.insert({2, 3});
        Tree.insert({3, 4});
        Tree.insert({4, 5});

        Parent[1] = 0;
        Parent[2] = 1;
        Parent[3] = 2;
        Parent[4] = 3;
        Parent[5] = 4;
    }

    std::cout << "Rank " << rank << ": Broadcasting Dist\n";
    MPI_Bcast(Dist.data(), num_vertices, MPI_INT, 0, MPI_COMM_WORLD);
    std::cout << "Rank " << rank << ": Broadcasting Parent\n";
    MPI_Bcast(Parent.data(), num_vertices, MPI_INT, 0, MPI_COMM_WORLD);
    std::vector<int> tree_data;
    if (rank == 0) {
        for (const auto& [u, v] : Tree) {
            tree_data.push_back(u);
            tree_data.push_back(v);
        }
    }
    int tree_size = tree_data.size();
    std::cout << "Rank " << rank << ": Broadcasting tree_size\n";
    MPI_Bcast(&tree_size, 1, MPI_INT, 0, MPI_COMM_WORLD);
    tree_data.resize(tree_size);
    std::cout << "Rank " << rank << ": Broadcasting tree_data\n";
    MPI_Bcast(tree_data.data(), tree_size, MPI_INT, 0, MPI_COMM_WORLD);
    if (rank != 0) {
        for (int i = 0; i < tree_size; i += 2) {
            Tree.insert({tree_data[i], tree_data[i + 1]});
        }
    }

    std::vector<std::pair<int, int>> Delk = {{2, 3}};
    std::vector<std::tuple<int, int, int>> Insk = {{1, 5, 2}};

    std::cout << "Rank " << rank << ": Before barrier\n";
    MPI_Barrier(MPI_COMM_WORLD);
    std::cout << "Rank " << rank << ": After barrier\n";
    double start = MPI_Wtime();

    std::cout << "Rank " << rank << ": Starting ProcessCE\n";
    ProcessCE(graph, Delk, Insk, Dist, Parent, AffectedDel, Affected, Gu, Tree, rank, size);
    std::cout << "Rank " << rank << ": Starting UpdateAffectedVertices\n";
    UpdateAffectedVertices(graph, Gu, Tree, Dist, Parent, AffectedDel, Affected, rank, size);

    std::cout << "Rank " << rank << ": Before final barrier\n";
    MPI_Barrier(MPI_COMM_WORLD);
    std::cout << "Rank " << rank << ": After final barrier\n";
    double end = MPI_Wtime();

    // Synchronize output to make it cleaner
    for (int r = 0; r < size; r++) {
        if (rank == r) {
            if (rank == 0) {
                std::cout << "Final Distances:\n";
                for (int i = 0; i < num_vertices; ++i) {
                    std::cout << "Node " << i << ": " << (Dist[i] == INF ? -1 : Dist[i]) << "\n";
                }
                std::cout << "Execution Time: " << (end - start) << " seconds\n";
            }
            std::cout << "Rank " << rank << " processed vertices: ";
            for (int v : graph.localVertices) std::cout << v << " ";
            std::cout << "\n";
        }
        MPI_Barrier(MPI_COMM_WORLD);
    }

    MPI_Finalize();
    return 0;
}

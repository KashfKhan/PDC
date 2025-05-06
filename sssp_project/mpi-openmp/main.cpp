#include <mpi.h>
#include <omp.h>
#include <iostream>
#include <vector>
#include <set>
#include <tuple>
#include <limits>
#include <fstream>
#include <queue>
#include <chrono>
#include <algorithm>
#include "sssp_mpi.h"
#include "graph_loader.h"

int main(int argc, char** argv) {
    MPI_Init(&argc, &argv);
    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    omp_set_num_threads(4);
    #pragma omp parallel
    {
        if (omp_get_thread_num() == 0 && rank == 0) {
            std::cout << "Rank " << rank << ": Using " << omp_get_num_threads() << " OpenMP threads per MPI rank\n";
        }
    }

    std::string graphFile = "/mirror/facebook_graph.txt";
    std::string partFile = "/mirror/facebook_graph.txt.part.8";
    if (rank == 0) std::cout << "Rank " << rank << ": Reading graph from " << graphFile << ", partition from " << partFile << "\n";
    Graph graph = load_partitioned_graph(graphFile, partFile, rank);

    int num_vertices = 0;
    if (rank == 0) {
        std::ifstream pfile(partFile);
        if (!pfile) {
            std::cerr << "Rank " << rank << ": Error opening partition file " << partFile << "\n";
            MPI_Abort(MPI_COMM_WORLD, 1);
        }
        std::string line;
        while (std::getline(pfile, line)) {
            if (!line.empty()) num_vertices++;
        }
        pfile.close();
        std::cout << "Rank " << rank << ": num_vertices = " << num_vertices << "\n";
    }
    MPI_Bcast(&num_vertices, 1, MPI_INT, 0, MPI_COMM_WORLD);

    const long long INF = std::numeric_limits<long long>::max();
    std::vector<long long> Dist(num_vertices, INF);
    std::vector<int> Parent(num_vertices, -1);
    std::vector<int> AffectedDel(num_vertices, 0);
    std::vector<int> Affected(num_vertices, 0);
    std::vector<std::tuple<int, int, int>> Gu;
    std::set<std::pair<int, int>> Tree;

    // Sequential Dijkstra
    auto start_seq = std::chrono::high_resolution_clock::now();
    if (rank == 0) {
        Dist[0] = 0;
        std::priority_queue<std::pair<long long, int>, std::vector<std::pair<long long, int>>, std::greater<>> pq;
        pq.push({0, 0});
        std::vector<bool> visited(num_vertices, false);
        while (!pq.empty()) {
            int u = pq.top().second;
            long long d = pq.top().first;
            pq.pop();
            if (visited[u]) continue;
            visited[u] = true;
            auto it = graph.adjacencyList.find(u);
            if (it != graph.adjacencyList.end()) {
                for (const auto& edge : it->second) {
                    int v = edge.dest;
                    long long w = edge.weight;
                    if (v >= 0 && v < num_vertices && !visited[v] && Dist[v] > Dist[u] + w) {
                        Dist[v] = Dist[u] + w;
                        Parent[v] = u;
                        pq.push({Dist[v], v});
                    }
                }
            }
        }
        for (int v = 0; v < num_vertices; ++v) {
            if (Parent[v] != -1) Tree.insert({Parent[v], v});
        }
        // Log initial distances
        for (int i = 0; i < std::min(10, num_vertices); ++i) {
            std::cout << "Sequential Initial Dist[" << i << "]: " << (Dist[i] == INF ? -1 : Dist[i]) << "\n";
        }
    }
    auto end_seq = std::chrono::high_resolution_clock::now();
    double seq_time = std::chrono::duration<double>(end_seq - start_seq).count();

    // Broadcast sequential results
    MPI_Bcast(Dist.data(), num_vertices, MPI_LONG_LONG, 0, MPI_COMM_WORLD);
    MPI_Bcast(Parent.data(), num_vertices, MPI_INT, 0, MPI_COMM_WORLD);

    auto start_mpi = std::chrono::high_resolution_clock::now();
    std::priority_queue<std::pair<long long, int>, std::vector<std::pair<long long, int>>, std::greater<>> pq;
    std::set<int> in_pq;

    for (int v : graph.localVertices) {
        if (Dist[v] != INF) {
            pq.push({Dist[v], v});
            in_pq.insert(v);
        }
    }

    int global_changed = 1;
    while (global_changed) {
        bool local_changed = false;
        while (!pq.empty()) {
            int u = pq.top().second;
            pq.pop();
            in_pq.erase(u);

            if (std::find(graph.localVertices.begin(), graph.localVertices.end(), u) != graph.localVertices.end()) {
                auto it = graph.adjacencyList.find(u);
                if (it != graph.adjacencyList.end()) {
                    std::vector<std::pair<long long, int>> local_pq_entries[4];
                    #pragma omp parallel
                    {
                        int tid = omp_get_thread_num();
                        #pragma omp for
                        for (size_t i = 0; i < it->second.size(); ++i) {
                            int v = it->second[i].dest;
                            if (v >= 0 && v < num_vertices) {
                                long long w = it->second[i].weight;
                                long long new_dist = Dist[u] + w;
                                if (new_dist < Dist[v]) {
                                    #pragma omp critical
                                    {
                                        Dist[v] = new_dist;
                                        Parent[v] = u;
                                        Affected[v] = 1;
                                        local_changed = true;
                                    }
                                    local_pq_entries[tid].push_back({new_dist, v});
                                }
                            }
                        }
                    }
                    for (int tid = 0; tid < 4; ++tid) {
                        for (const auto& entry : local_pq_entries[tid]) {
                            long long new_dist = entry.first;
                            int v = entry.second;
                            if (!in_pq.count(v)) {
                                pq.push({new_dist, v});
                                in_pq.insert(v);
                            }
                        }
                    }
                }
            }
        }

        int local_changed_int = local_changed ? 1 : 0;
        int global_changed_int = 0;
        MPI_Allreduce(&local_changed_int, &global_changed_int, 1, MPI_INT, MPI_LOR, MPI_COMM_WORLD);
        global_changed = global_changed_int;

        MPI_Allreduce(MPI_IN_PLACE, Dist.data(), Dist.size(), MPI_LONG_LONG, MPI_MIN, MPI_COMM_WORLD);
        MPI_Allreduce(MPI_IN_PLACE, Parent.data(), Parent.size(), MPI_INT, MPI_MAX, MPI_COMM_WORLD);
        MPI_Allreduce(MPI_IN_PLACE, Affected.data(), Affected.size(), MPI_INT, MPI_MAX, MPI_COMM_WORLD);
    }

    for (int v = 0; v < num_vertices; ++v) {
        if (Parent[v] != -1) Tree.insert({Parent[v], v});
    }

    std::vector<int> tree_data;
    if (rank == 0) {
        for (const auto& [u, v] : Tree) {
            tree_data.push_back(u);
            tree_data.push_back(v);
        }
    }
    int tree_size = tree_data.size();
    MPI_Bcast(&tree_size, 1, MPI_INT, 0, MPI_COMM_WORLD);
    tree_data.resize(tree_size);
    MPI_Bcast(tree_data.data(), tree_size, MPI_INT, 0, MPI_COMM_WORLD);
    if (rank != 0) {
        Tree.clear();
        for (int i = 0; i < tree_size; i += 2) {
            Tree.insert({tree_data[i], tree_data[i + 1]});
        }
    }
    auto end_mpi = std::chrono::high_resolution_clock::now();
    double mpi_time = std::chrono::duration<double>(end_mpi - start_mpi).count();

    // Multiple dynamic updates
    auto start_update = std::chrono::high_resolution_clock::now();
    const int num_updates = 20;
    for (int update = 0; update < num_updates; ++update) {
        std::vector<std::pair<int, int>> Delk;
        std::vector<std::tuple<int, int, int>> Insk;
        if (rank == 0) {
            // Select tree edge for deletion
            if (!Tree.empty()) {
                auto it = Tree.begin();
                std::advance(it, update % Tree.size());
                Delk.push_back(*it);
            }
            // Select non-existent edge for insertion
            for (int i = 0; i < num_vertices; ++i) {
                for (int j = i + 1; j < num_vertices; ++j) {
                    bool connected = false;
                    auto it = graph.adjacencyList.find(i);
                    if (it != graph.adjacencyList.end()) {
                        for (const auto& edge : it->second) {
                            if (edge.dest == j) {
                                connected = true;
                                break;
                            }
                        }
                    }
                    if (!connected) {
                        Insk.emplace_back(i, j, 1);
                        break;
                    }
                }
                if (!Insk.empty()) break;
            }
        }

        // Broadcast Delk and Insk
        int delk_size = Delk.size();
        MPI_Bcast(&delk_size, 1, MPI_INT, 0, MPI_COMM_WORLD);
        Delk.resize(delk_size);
        if (rank == 0) {
            for (size_t i = 0; i < Delk.size(); ++i) {
                tree_data[i * 2] = Delk[i].first;
                tree_data[i * 2 + 1] = Delk[i].second;
            }
        }
        tree_data.resize(delk_size * 2);
        MPI_Bcast(tree_data.data(), delk_size * 2, MPI_INT, 0, MPI_COMM_WORLD);
        if (rank != 0) {
            Delk.clear();
            for (int i = 0; i < delk_size; ++i) {
                Delk.emplace_back(tree_data[i * 2], tree_data[i * 2 + 1]);
            }
        }

        int insk_size = Insk.size();
        MPI_Bcast(&insk_size, 1, MPI_INT, 0, MPI_COMM_WORLD);
        Insk.resize(insk_size);
        std::vector<int> insk_data(insk_size * 3);
        if (rank == 0) {
            for (size_t i = 0; i < Insk.size(); ++i) {
                insk_data[i * 3] = std::get<0>(Insk[i]);
                insk_data[i * 3 + 1] = std::get<1>(Insk[i]);
                insk_data[i * 3 + 2] = std::get<2>(Insk[i]);
            }
        }
        MPI_Bcast(insk_data.data(), insk_size * 3, MPI_INT, 0, MPI_COMM_WORLD);
        if (rank != 0) {
            Insk.clear();
            for (int i = 0; i < insk_size; ++i) {
                Insk.emplace_back(insk_data[i * 3], insk_data[i * 3 + 1], insk_data[i * 3 + 2]);
            }
        }

        ProcessCE(graph, Delk, Insk, Dist, Parent, AffectedDel, Affected, Gu, Tree, rank, size);
        UpdateAffectedVertices(graph, Gu, Tree, Dist, Parent, AffectedDel, Affected, rank, size);
    }

    auto end_update = std::chrono::high_resolution_clock::now();
    double update_time = std::chrono::duration<double>(end_update - start_update).count();

    MPI_Barrier(MPI_COMM_WORLD);
    if (rank == 0) {
        std::cout << "Sequential Dijkstra Time: " << seq_time << " seconds\n";
        std::cout << "MPI+OpenMP Dijkstra Time: " << mpi_time << " seconds\n";
        std::cout << "Speedup (Dijkstra): " << seq_time / mpi_time << "x\n";
        std::cout << "Update Phase Time: " << update_time << " seconds\n";
        std::cout << "Final Distances:\n";
        for (int i = 0; i < std::min(6, num_vertices); ++i) {
            std::cout << "Node " << i << ": " << (Dist[i] == INF ? -1 : Dist[i]) << "\n";
        }
    }
    MPI_Barrier(MPI_COMM_WORLD);
    std::cout << "Rank " << rank << " processed vertices: ";
    for (int v : graph.localVertices) std::cout << v << " ";
    std::cout << "\n";
    MPI_Barrier(MPI_COMM_WORLD);

    MPI_Finalize();
    return 0;
}

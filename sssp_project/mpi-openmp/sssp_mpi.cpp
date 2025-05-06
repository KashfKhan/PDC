#include "sssp_mpi.h"
#include <omp.h>
#include <algorithm>
#include <queue>
#include <limits>
#include <mpi.h>
#include <iostream>

void ProcessCE(
    Graph& graph,
    std::vector<std::pair<int, int>>& Delk,
    std::vector<std::tuple<int, int, int>>& Insk,
    std::vector<long long>& Dist,
    std::vector<int>& Parent,
    std::vector<int>& AffectedDel,
    std::vector<int>& Affected,
    std::vector<std::tuple<int, int, int>>& Gu,
    std::set<std::pair<int, int>>& Tree,
    int rank,
    int size
) {
    const long long INF = std::numeric_limits<long long>::max();
    std::fill(AffectedDel.begin(), AffectedDel.end(), 0);
    std::fill(Affected.begin(), Affected.end(), 0);

    #pragma omp parallel for
    for (size_t i = 0; i < Delk.size(); ++i) {
        int u = Delk[i].first;
        int v = Delk[i].second;
        if (u >= 0 && u < Dist.size() && v >= 0 && v < Dist.size()) {
            if (std::find(graph.localVertices.begin(), graph.localVertices.end(), u) != graph.localVertices.end() ||
                std::find(graph.localVertices.begin(), graph.localVertices.end(), v) != graph.localVertices.end()) {
                if (Tree.count({u, v}) || Tree.count({v, u})) {
                    int y = Dist[u] > Dist[v] ? u : v;
                    #pragma omp critical
                    {
                        if (y >= 0 && y < Dist.size()) {
                            std::cout << "Rank " << rank << ": ProcessCE Delk set Dist[" << y << "] to INF\n";
                            Dist[y] = INF;
                            AffectedDel[y] = 1;
                            Affected[y] = 1;
                        }
                    }
                }
            }
        }
    }

    #pragma omp parallel for
    for (size_t i = 0; i < Insk.size(); ++i) {
        int u = std::get<0>(Insk[i]);
        int v = std::get<1>(Insk[i]);
        int w = std::get<2>(Insk[i]);
        if (u >= 0 && u < Dist.size() && v >= 0 && v < Dist.size()) {
            if (std::find(graph.localVertices.begin(), graph.localVertices.end(), u) != graph.localVertices.end() ||
                std::find(graph.localVertices.begin(), graph.localVertices.end(), v) != graph.localVertices.end()) {
                int x = Dist[u] > Dist[v] ? v : u;
                int y = (x == u) ? v : u;
                #pragma omp critical
                {
                    if (x >= 0 && x < Dist.size() && y >= 0 && y < Dist.size() && Dist[x] != INF) {
                        if (Dist[y] > Dist[x] + w) {
                            std::cout << "Rank " << rank << ": ProcessCE Insk updated Dist[" << y << "] to " << Dist[x] + w << "\n";
                            Dist[y] = Dist[x] + w;
                            Parent[y] = x;
                            Affected[y] = 1;
                        }
                        Gu.push_back({u, v, w});
                    }
                }
            }
        }
    }

    // Update adjacency list
    for (const auto& [u, v, w] : Insk) {
        if (std::find(graph.localVertices.begin(), graph.localVertices.end(), u) != graph.localVertices.end()) {
            graph.adjacencyList[u].push_back({v, w});
        }
        if (std::find(graph.localVertices.begin(), graph.localVertices.end(), v) != graph.localVertices.end()) {
            graph.adjacencyList[v].push_back({u, w});
        }
    }

    MPI_Allreduce(MPI_IN_PLACE, Dist.data(), Dist.size(), MPI_LONG_LONG, MPI_MIN, MPI_COMM_WORLD);
    MPI_Allreduce(MPI_IN_PLACE, Parent.data(), Parent.size(), MPI_INT, MPI_MAX, MPI_COMM_WORLD);
    MPI_Allreduce(MPI_IN_PLACE, Affected.data(), Affected.size(), MPI_INT, MPI_MAX, MPI_COMM_WORLD);
}

void UpdateAffectedVertices(
    Graph& graph,
    std::vector<std::tuple<int, int, int>>& Gu,
    std::set<std::pair<int, int>>& Tree,
    std::vector<long long>& Dist,
    std::vector<int>& Parent,
    std::vector<int>& AffectedDel,
    std::vector<int>& Affected,
    int rank,
    int size
) {
    const long long INF = std::numeric_limits<long long>::max();
    std::vector<std::vector<int>> children(Dist.size());
    std::vector<int> visited(Dist.size(), 0); // Track visited vertices in deletion phase
    const int max_iterations = Dist.size(); // Limit iterations to vertex count

    // Build children array
    for (int v = 0; v < Dist.size(); ++v) {
        if (Parent[v] != -1 && Parent[v] >= 0 && Parent[v] < Dist.size()) {
            children[Parent[v]].push_back(v);
        }
    }

    // Deletion phase
    int global_del_changed = 1;
    int iteration = 0;
    while (global_del_changed && iteration < max_iterations) {
        bool local_del_changed = false;
        #pragma omp parallel for
        for (int v : graph.localVertices) {
            if (v >= 0 && v < Dist.size() && AffectedDel[v] && !visited[v]) {
                #pragma omp critical
                {
                    AffectedDel[v] = 0;
                    visited[v] = 1; // Mark as processed
                    for (int c : children[v]) {
                        if (c >= 0 && c < Dist.size() && !visited[c]) {
                            std::cout << "Rank " << rank << ": UpdateAffectedVertices Del set Dist[" << c << "] to INF (iteration " << iteration << ")\n";
                            Dist[c] = INF;
                            AffectedDel[c] = 1;
                            Affected[c] = 1;
                            local_del_changed = true;
                        }
                    }
                }
            }
        }

        int local_del_changed_int = local_del_changed ? 1 : 0;
        int global_del_changed_int = 0;
        MPI_Allreduce(&local_del_changed_int, &global_del_changed_int, 1, MPI_INT, MPI_LOR, MPI_COMM_WORLD);
        global_del_changed = global_del_changed_int;

        MPI_Allreduce(MPI_IN_PLACE, Dist.data(), Dist.size(), MPI_LONG_LONG, MPI_MIN, MPI_COMM_WORLD);
        MPI_Allreduce(MPI_IN_PLACE, AffectedDel.data(), AffectedDel.size(), MPI_INT, MPI_MAX, MPI_COMM_WORLD);
        MPI_Allreduce(MPI_IN_PLACE, Affected.data(), Affected.size(), MPI_INT, MPI_MAX, MPI_COMM_WORLD);
        iteration++;
    }

    if (iteration >= max_iterations) {
        std::cout << "Rank " << rank << ": Warning: Deletion phase reached max iterations (" << max_iterations << "), possible cycle in Parent\n";
    }

    // Update phase
    int global_changed = 1;
    while (global_changed) {
        bool local_changed = false;
        #pragma omp parallel for
        for (int v : graph.localVertices) {
            if (v >= 0 && v < Dist.size() && Affected[v]) {
                Affected[v] = 0;
                auto it = graph.adjacencyList.find(v);
                if (it != graph.adjacencyList.end()) {
                    for (const auto& edge : it->second) {
                        int n = edge.dest;
                        long long w = edge.weight;
                        if (n >= 0 && n < Dist.size()) {
                            #pragma omp critical
                            {
                                if (Dist[v] != INF && Dist[n] > Dist[v] + w) {
                                    std::cout << "Rank " << rank << ": UpdateAffectedVertices set Dist[" << n << "] to " << Dist[v] + w << "\n";
                                    Dist[n] = Dist[v] + w;
                                    Parent[n] = v;
                                    Affected[n] = 1;
                                    local_changed = true;
                                } else if (Dist[n] != INF && Dist[v] > Dist[n] + w) {
                                    std::cout << "Rank " << rank << ": UpdateAffectedVertices set Dist[" << v << "] to " << Dist[n] + w << "\n";
                                    Dist[v] = Dist[n] + w;
                                    Parent[v] = n;
                                    Affected[v] = 1;
                                    local_changed = true;
                                }
                            }
                        }
                    }
                }
            }
        }

        int local_del_changed_int = local_changed ? 1 : 0;
        int global_changed_int = 0;
        MPI_Allreduce(&local_del_changed_int, &global_changed_int, 1, MPI_INT, MPI_LOR, MPI_COMM_WORLD);
        global_changed = global_changed_int;

        MPI_Allreduce(MPI_IN_PLACE, Dist.data(), Dist.size(), MPI_LONG_LONG, MPI_MIN, MPI_COMM_WORLD);
        MPI_Allreduce(MPI_IN_PLACE, Parent.data(), Parent.size(), MPI_INT, MPI_MAX, MPI_COMM_WORLD);
        MPI_Allreduce(MPI_IN_PLACE, Affected.data(), Affected.size(), MPI_INT, MPI_MAX, MPI_COMM_WORLD);
    }
}

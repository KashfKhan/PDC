#include "sssp_mpi.h"
#include "graph_loader.h"

void ProcessCE(
    const Graph& graph,
    const std::vector<std::pair<int, int>>& Delk,
    const std::vector<std::tuple<int, int, int>>& Insk,
    std::vector<int>& Dist,
    std::vector<int>& Parent,
    std::vector<int>& AffectedDel,
    std::vector<int>& Affected,
    std::vector<std::tuple<int, int, int>>& Gu,
    const std::set<std::pair<int, int>>& T,
    int rank,
    int size
) {
    std::cout << "Rank " << rank << ": Processing deletions\n";
    for (size_t i = 0; i < Delk.size(); ++i) {
        int u = Delk[i].first;
        int v = Delk[i].second;
        if (T.count({u, v}) || T.count({v, u})) {
            int y = (Dist[u] > Dist[v]) ? u : v;
            if (std::find(graph.localVertices.begin(), graph.localVertices.end(), y) != graph.localVertices.end()) {
                Dist[y] = INF;
                AffectedDel[y] = 1;
                Affected[y] = 1;
            }
        }
    }

    std::cout << "Rank " << rank << ": Allreduce after deletions\n";
    MPI_Allreduce(MPI_IN_PLACE, Dist.data(), Dist.size(), MPI_INT, MPI_MIN, MPI_COMM_WORLD);
    MPI_Allreduce(MPI_IN_PLACE, AffectedDel.data(), AffectedDel.size(), MPI_INT, MPI_MAX, MPI_COMM_WORLD);
    MPI_Allreduce(MPI_IN_PLACE, Affected.data(), Affected.size(), MPI_INT, MPI_MAX, MPI_COMM_WORLD);

    std::cout << "Rank " << rank << ": Processing insertions\n";
    for (size_t i = 0; i < Insk.size(); ++i) {
        int u, v, w;
        std::tie(u, v, w) = Insk[i];

        int x, y;
        if (Dist[u] > Dist[v]) { x = v; y = u; }
        else { x = u; y = v; }

        if (std::find(graph.localVertices.begin(), graph.localVertices.end(), y) != graph.localVertices.end()) {
            if (Dist[y] > Dist[x] + w) {
                Dist[y] = Dist[x] + w;
                Parent[y] = x;
                Affected[y] = 1;
            }
        }

        Gu.push_back({u, v, w});
    }

    std::cout << "Rank " << rank << ": Allreduce after insertions\n";
    MPI_Allreduce(MPI_IN_PLACE, Dist.data(), Dist.size(), MPI_INT, MPI_MIN, MPI_COMM_WORLD);
    MPI_Allreduce(MPI_IN_PLACE, Parent.data(), Parent.size(), MPI_INT, MPI_MAX, MPI_COMM_WORLD);
    MPI_Allreduce(MPI_IN_PLACE, Affected.data(), Affected.size(), MPI_INT, MPI_MAX, MPI_COMM_WORLD);
    std::cout << "Rank " << rank << ": Finished ProcessCE\n";
}

void UpdateAffectedVertices(
    const Graph& graph,
    const std::vector<std::tuple<int, int, int>>& Gu,
    const std::set<std::pair<int, int>>& Tree,
    std::vector<int>& Dist,
    std::vector<int>& Parent,
    std::vector<int>& AffectedDel,
    std::vector<int>& Affected,
    int rank,
    int size
) {
    const int MAX_ITERATIONS = 100;
    int iteration = 0;

    std::cout << "Rank " << rank << ": Updating affected vertices (deletions)\n";
    while (std::any_of(AffectedDel.begin(), AffectedDel.end(), [](int b) { return b != 0; }) && iteration < MAX_ITERATIONS) {
        std::cout << "Rank " << rank << ": Deletion loop iteration " << iteration << "\n";
        for (int v : graph.localVertices) {
            if (AffectedDel[v]) {
                std::cout << "Rank " << rank << ": Processing vertex " << v << " with AffectedDel[" << v << "] = 1\n";
                AffectedDel[v] = 0;

                for (int c = 0; c < Dist.size(); ++c) {
                    if (Parent[c] == v) {
                        std::cout << "Rank " << rank << ": Setting Dist[" << c << "] = INF because Parent[" << c << "] = " << v << "\n";
                        Dist[c] = INF;
                        AffectedDel[c] = 1;
                        Affected[c] = 1;
                    }
                }
            }
        }

        std::cout << "Rank " << rank << ": Allreduce in deletion loop\n";
        MPI_Allreduce(MPI_IN_PLACE, Dist.data(), Dist.size(), MPI_INT, MPI_MIN, MPI_COMM_WORLD);
        MPI_Allreduce(MPI_IN_PLACE, AffectedDel.data(), AffectedDel.size(), MPI_INT, MPI_MAX, MPI_COMM_WORLD);
        MPI_Allreduce(MPI_IN_PLACE, Affected.data(), Affected.size(), MPI_INT, MPI_MAX, MPI_COMM_WORLD);
        iteration++;
    }
    if (iteration >= MAX_ITERATIONS) {
        std::cerr << "Rank " << rank << ": Warning: Deletion loop exceeded " << MAX_ITERATIONS << " iterations\n";
    }

    std::cout << "Rank " << rank << ": Updating affected vertices (general)\n";
    iteration = 0;
    std::vector<int> update_count(Dist.size(), 0);
    const int MAX_UPDATES_PER_VERTEX = 5; // Lowered to prevent excessive updates

    while (std::any_of(Affected.begin(), Affected.end(), [](int b) { return b != 0; }) && iteration < MAX_ITERATIONS) {
        std::cout << "Rank " << rank << ": General loop iteration " << iteration << "\n";
        std::cout << "Rank " << rank << ": Affected = [";
        for (int i = 0; i < Affected.size(); ++i) {
            std::cout << Affected[i] << (i < Affected.size() - 1 ? ", " : "");
        }
        std::cout << "]\n";

        for (int v : graph.localVertices) {
            if (Affected[v]) {
                std::cout << "Rank " << rank << ": Processing vertex " << v << " with Affected[" << v << "] = 1\n";
                Affected[v] = 0;

                auto it = graph.adjacencyList.find(v);
                if (it != graph.adjacencyList.end()) {
                    for (const auto& edge : it->second) {
                        int n = edge.dest;
                        int w = edge.weight;

                        if (Dist[v] != INF && update_count[n] < MAX_UPDATES_PER_VERTEX) { // Only update n if v has a finite distance
                            if (Dist[n] > Dist[v] + w) {
                                std::cout << "Rank " << rank << ": Updating Dist[" << n << "] from " << Dist[n] << " to " << (Dist[v] + w) << " via vertex " << v << "\n";
                                Dist[n] = Dist[v] + w;
                                Parent[n] = v;
                                Affected[n] = 1;
                                update_count[n]++;
                            }
                        }
                    }
                }
            }
        }

        std::cout << "Rank " << rank << ": Allreduce in general loop\n";
        MPI_Allreduce(MPI_IN_PLACE, Dist.data(), Dist.size(), MPI_INT, MPI_MIN, MPI_COMM_WORLD);
        MPI_Allreduce(MPI_IN_PLACE, Parent.data(), Parent.size(), MPI_INT, MPI_MAX, MPI_COMM_WORLD);
        MPI_Allreduce(MPI_IN_PLACE, Affected.data(), Affected.size(), MPI_INT, MPI_MAX, MPI_COMM_WORLD);
        iteration++;
    }
    if (iteration >= MAX_ITERATIONS) {
        std::cerr << "Rank " << rank << ": Warning: General loop exceeded " << MAX_ITERATIONS << " iterations\n";
    }
    std::cout << "Rank " << rank << ": Finished UpdateAffectedVertices\n";
}

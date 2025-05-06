#include <iostream>
#include <vector>
#include <set>
#include <tuple>
#include <limits>
#include <fstream>
#include <queue>
#include <unordered_map>
#include <algorithm>
#include <sstream>
#include <chrono>

struct Edge {
    int dest;
    int weight;
};

struct Graph {
    std::vector<std::vector<Edge>> adjacencyList; // Use vector for zero-based IDs
    std::vector<int> localVertices;
};

void buildTree(std::vector<int>& Parent, std::set<std::pair<int, int>>& Tree) {
    Tree.clear();
    for (int v = 0; v < Parent.size(); ++v) {
        if (Parent[v] != -1) {
            Tree.insert({Parent[v], v});
        }
    }
    std::cout << "Tree built with " << Tree.size() << " edges\n";
}

void ProcessCE(Graph& graph, std::vector<std::pair<int, int>>& Delk, std::vector<std::tuple<int, int, int>>& Insk,
               std::vector<long long>& Dist, std::vector<int>& Parent, std::vector<bool>& AffectedDel,
               std::vector<bool>& Affected, Graph& Gu, std::set<std::pair<int, int>>& Tree) {
    const long long INF = std::numeric_limits<long long>::max();
    std::fill(AffectedDel.begin(), AffectedDel.end(), false);
    std::fill(Affected.begin(), Affected.end(), false);

    std::cout << "Processing " << Delk.size() << " deletions\n";
    for (const auto& edge : Delk) {
        int u = edge.first;
        int v = edge.second;
        std::cout << "Deleting edge (" << u << ", " << v << ")\n";
        if (Tree.count({u, v}) || Tree.count({v, u})) {
            int y = (Dist[u] > Dist[v]) ? u : v;
            if (y < Dist.size()) {
                Dist[y] = INF;
                AffectedDel[y] = true;
                Affected[y] = true;
                std::cout << "Set Dist[" << y << "] to INF\n";
            }
        }

        graph.adjacencyList[u].erase(std::remove_if(graph.adjacencyList[u].begin(), graph.adjacencyList[u].end(),
                                                    [v](const Edge& e) { return e.dest == v; }),
                                     graph.adjacencyList[u].end());
        graph.adjacencyList[v].erase(std::remove_if(graph.adjacencyList[v].begin(), graph.adjacencyList[v].end(),
                                                    [u](const Edge& e) { return e.dest == u; }),
                                     graph.adjacencyList[v].end());
    }

    std::cout << "Processing " << Insk.size() << " insertions\n";
    for (const auto& edge : Insk) {
        int u = std::get<0>(edge);
        int v = std::get<1>(edge);
        int w = std::get<2>(edge);
        std::cout << "Inserting edge (" << u << ", " << v << ", " << w << ")\n";

        graph.adjacencyList[u].push_back({v, w});
        graph.adjacencyList[v].push_back({u, w});
        Gu.adjacencyList[u].push_back({v, w});
        Gu.adjacencyList[v].push_back({u, w});

        int x = (Dist[u] <= Dist[v]) ? u : v;
        int y = (x == u) ? v : u;

        if (x < Dist.size() && y < Dist.size() && Dist[x] != INF &&
            (Dist[y] == INF || Dist[y] > Dist[x] + w)) {
            Dist[y] = Dist[x] + w;
            Parent[y] = x;
            Affected[y] = true;
            std::cout << "Updated Dist[" << y << "] to " << Dist[y] << "\n";
        }
    }

    buildTree(Parent, Tree);
}

void UpdateAffectedVertices(Graph& graph, Graph& Gu, std::set<std::pair<int, int>>& Tree,
                            std::vector<long long>& Dist, std::vector<int>& Parent,
                            std::vector<bool>& AffectedDel, std::vector<bool>& Affected) {
    const long long INF = std::numeric_limits<long long>::max();
    std::vector<std::vector<int>> children(Dist.size());

    for (int v = 0; v < Parent.size(); ++v) {
        if (Parent[v] != -1 && Parent[v] < children.size()) {
            children[Parent[v]].push_back(v);
        }
    }

    bool any_del_affected = true;
    int del_iterations = 0;
    while (any_del_affected) {
        any_del_affected = false;
        for (int v = 0; v < AffectedDel.size(); ++v) {
            if (AffectedDel[v]) {
                AffectedDel[v] = false;
                for (int c : children[v]) {
                    if (c < Dist.size()) {
                        Dist[c] = INF;
                        AffectedDel[c] = true;
                        Affected[c] = true;
                        any_del_affected = true;
                        std::cout << "Set Dist[" << c << "] to INF (child of " << v << ")\n";
                    }
                }
            }
        }
        ++del_iterations;
    }
    std::cout << "Deletion phase completed in " << del_iterations << " iterations\n";

    bool any_affected = true;
    int aff_iterations = 0;
    while (any_affected) {
        any_affected = false;
        for (int v = 0; v < Affected.size(); ++v) {
            if (Affected[v]) {
                Affected[v] = false;
                for (const auto& edge : graph.adjacencyList[v]) {
                    int n = edge.dest;
                    int w = edge.weight;
                    if (Dist[v] != INF && (Dist[n] == INF || Dist[n] > Dist[v] + w)) {
                        Dist[n] = Dist[v] + w;
                        Parent[n] = v;
                        Affected[n] = true;
                        any_affected = true;
                        std::cout << "Updated Dist[" << n << "] to " << Dist[v] + w << " via " << v << "\n";
                    } else if (Dist[n] != INF && (Dist[v] == INF || Dist[v] > Dist[n] + w)) {
                        Dist[v] = Dist[n] + w;
                        Parent[v] = n;
                        Affected[v] = true;
                        any_affected = true;
                        std::cout << "Updated Dist[" << v << "] to " << Dist[n] + w << " via " << n << "\n";
                    }
                }
            }
        }
        ++aff_iterations;
    }
    std::cout << "Affected phase completed in " << aff_iterations << " iterations\n";

    buildTree(Parent, Tree);
}

int main() {
    auto start_total = std::chrono::high_resolution_clock::now();
    std::string graphFile = "/mirror/facebook_combined.txt";
    std::string partFile = "/mirror/facebook_graph.txt.part.8";

    Graph graph;
    auto start_load = std::chrono::high_resolution_clock::now();
    std::ifstream infile(graphFile);
    if (!infile) {
        std::cerr << "Error opening graph file " << graphFile << "\n";
        return 1;
    }

    std::string line;
    std::getline(infile, line);
    std::istringstream header(line);
    int num_vertices, num_edges;
    header >> num_vertices >> num_edges;
    std::cout << "Header: " << num_vertices << " vertices, " << num_edges << " edges\n";

    // Resize adjacency list for zero-based nodes
    graph.adjacencyList.resize(num_vertices);

    // Read edges and ensure unique undirected edges
    std::set<std::pair<int, int>> unique_edges;
    while (std::getline(infile, line)) {
        std::istringstream iss(line);
        int u, v;
        if (iss >> u >> v) {
            int min_idx = std::min(u, v);
            int max_idx = std::max(u, v);
            unique_edges.insert({min_idx, max_idx});
        }
    }
    infile.close();

    for (const auto& [u, v] : unique_edges) {
        graph.adjacencyList[u].push_back({v, 1});
        graph.adjacencyList[v].push_back({u, 1});
        std::cout << "Added edge (" << u << ", " << v << ")\n";
    }
    std::cout << "Graph built with " << unique_edges.size() << " unique edges\n";

    // Log neighbor counts for key nodes
    for (int i = 0; i < std::min(5, num_vertices); ++i) {
        std::cout << "Node " << i << " has " << graph.adjacencyList[i].size() << " neighbors\n";
    }

    auto end_load = std::chrono::high_resolution_clock::now();
    std::cout << "Graph loading took "
              << std::chrono::duration<double>(end_load - start_load).count() << " seconds\n";

    std::ifstream pfile(partFile);
    if (!pfile) {
        std::cerr << "Error opening partition file " << partFile << "\n";
        return 1;
    }

    graph.localVertices.clear();
    int vertex_count = 0;
    while (std::getline(pfile, line)) {
        if (vertex_count < num_vertices) {
            graph.localVertices.push_back(vertex_count);
            ++vertex_count;
        }
    }
    pfile.close();
    std::cout << "Loaded " << vertex_count << " vertices from partition file\n";

    const long long INF = std::numeric_limits<long long>::max();
    std::vector<long long> Dist(num_vertices, INF);
    std::vector<int> Parent(num_vertices, -1);
    std::vector<bool> AffectedDel(num_vertices, false);
    std::vector<bool> Affected(num_vertices, false);
    std::vector<bool> visited(num_vertices, false);
    std::vector<long long> initialDist(num_vertices, INF);
    Graph Gu;
    Gu.adjacencyList.resize(num_vertices);
   std::set<std::pair<int, int>> Tree;
    // Dijkstra's algorithm
    auto start_dijkstra = std::chrono::high_resolution_clock::now();
    int source = 0;
    Dist[source] = 0;
    std::priority_queue<std::pair<long long, int>, std::vector<std::pair<long long, int>>, std::greater<>> pq;
    pq.push({0, source});
    std::cout << "Starting Dijkstra from source " << source << "\n";

    int nodes_processed = 0;
    while (!pq.empty()) {
        int u = pq.top().second;
        long long d = pq.top().first;
        pq.pop();

        if (visited[u]) continue;
        visited[u] = true;
        ++nodes_processed;

        std::cout << "Processing node " << u << " with distance " << d << "\n";
        for (const auto& edge : graph.adjacencyList[u]) {
            int v = edge.dest, w = edge.weight;
            if (!visited[v] && Dist[v] > Dist[u] + w) {
                Dist[v] = Dist[u] + w;
                Parent[v] = u;
                pq.push({Dist[v], v});
                std::cout << "Updated Dist[" << v << "] to " << Dist[v] << " via " << u << "\n";
            }
        }
    }
    std::cout << "Dijkstra completed, processed " << nodes_processed << " nodes\n";

    // Store and log initial distances
    initialDist = Dist;
    std::vector<int> dist_count(10, 0);
    for (int i = 0; i < num_vertices; ++i) {
        if (initialDist[i] != INF && initialDist[i] < 10) {
            dist_count[initialDist[i]]++;
        }
    }
    for (int d = 0; d < 10; ++d) {
        std::cout << "Nodes at distance " << d << ": " << dist_count[d] << "\n";
    }
    for (int i = 0; i < std::min(10, num_vertices); ++i) {
        std::cout << "Initial Dist[" << i << "]: " << (initialDist[i] == INF ? -1 : initialDist[i]) << "\n";
    }

    auto end_dijkstra = std::chrono::high_resolution_clock::now();
    std::cout << "Dijkstra took "
              << std::chrono::duration<double>(end_dijkstra - start_dijkstra).count() << " seconds\n";

    buildTree(Parent, Tree);

    // Collect tree and non-tree edges
    std::vector<std::pair<int, int>> treeEdges, nonTreeEdges;
    for (int u = 0; u < num_vertices; ++u) {
        for (const auto& edge : graph.adjacencyList[u]) {
            int v = edge.dest;
            if (u < v) {
                if (Tree.count({u, v}) || Tree.count({v, u})) {
                    treeEdges.push_back({u, v});
                } else {
                    nonTreeEdges.push_back({u, v});
                }
            }
        }
    }
    std::cout << "Found " << treeEdges.size() << " tree edges and " << nonTreeEdges.size() << " non-tree edges\n";

    // Multiple dynamic updates
    auto start_updates = std::chrono::high_resolution_clock::now();
    const int num_updates = 20; // Adjust to target ~80s
    for (int update = 0; update < num_updates; ++update) {
        std::cout << "\nDynamic update iteration " << update + 1 << "\n";
        std::vector<std::pair<int, int>> Delk;
        std::vector<std::tuple<int, int, int>> Insk;

        if (!treeEdges.empty()) {
            Delk.push_back(treeEdges[update % treeEdges.size()]);
        } else if (!nonTreeEdges.empty()) {
            Delk.push_back(nonTreeEdges[update % nonTreeEdges.size()]);
        }
        std::cout << "Selected " << Delk.size() << " edge(s) for deletion\n";

        for (int i = 0; i < num_vertices; ++i) {
            for (int j = i + 1; j < num_vertices; ++j) {
                if (Dist[i] != INF && Dist[j] != INF) {
                    bool connected = false;
                    for (const auto& edge : graph.adjacencyList[i]) {
                        if (edge.dest == j) {
                            connected = true;
                            break;
                        }
                    }
                    if (!connected) {
                        Insk.emplace_back(i, j, 1);
                        std::cout << "Selected edge (" << i << ", " << j << ") for insertion\n";
                        break;
                    }
                }
            }
            if (!Insk.empty()) break;
        }

        ProcessCE(graph, Delk, Insk, Dist, Parent, AffectedDel, Affected, Gu, Tree);
        UpdateAffectedVertices(graph, Gu, Tree, Dist, Parent, AffectedDel, Affected);
    }

    auto end_updates = std::chrono::high_resolution_clock::now();
    std::cout << "Dynamic updates took "
              << std::chrono::duration<double>(end_updates - start_updates).count() << " seconds\n";

    // Output final distances
    for (int i = 0; i < std::min(10, num_vertices); ++i) {
        std::cout << "Node " << i << ": " << (Dist[i] == INF ? -1 : Dist[i]) << "\n";
    }

    // Log final distance distribution
    std::fill(dist_count.begin(), dist_count.end(), 0);
    for (int i = 0; i < num_vertices; ++i) {
        if (Dist[i] != INF && Dist[i] < 10) {
            dist_count[Dist[i]]++;
        }
    }
    for (int d = 0; d < 10; ++d) {
        std::cout << "Final nodes at distance " << d << ": " << dist_count[d] << "\n";
    }

    auto end_total = std::chrono::high_resolution_clock::now();
    std::cout << "Total execution took "
              << std::chrono::duration<double>(end_total - start_total).count() << " seconds\n";

    return 0;
}

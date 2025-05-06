#include "graph_loader.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <set>
#include <unordered_map>
#include <algorithm>

Graph load_partitioned_graph(const std::string& graphFile, const std::string& partFile, int rank) {
    std::ifstream gfile(graphFile);
    std::ifstream pfile(partFile);
    Graph graph;

    if (!gfile || !pfile) {
        std::cerr << "Rank " << rank << ": Error opening graph or partition file\n";
        return graph;
    }

    // Read header
    std::string line;
    std::getline(gfile, line);
    std::istringstream header(line);
    int num_vertices, num_edges;
    header >> num_vertices >> num_edges;
    if (rank == 0) {
        std::cout << "Rank " << rank << ": Header: " << num_vertices << " vertices, " << num_edges << " edges\n";
    }

    // Read partitions
    std::vector<int> partitions(num_vertices, -1);
    int vertex_id = 0;
    while (pfile >> partitions[vertex_id]) {
        if (vertex_id >= num_vertices) break;
        vertex_id++;
    }
    pfile.close();
    if (vertex_id != num_vertices) {
        std::cerr << "Rank " << rank << ": Partition file has " << vertex_id << " entries, expected " << num_vertices << "\n";
        return graph;
    }

    // Aggregate neighbors to handle duplicates
    std::unordered_map<int, std::set<int>> temp_graph;
    vertex_id = 0;
    while (std::getline(gfile, line)) {
        if (vertex_id >= num_vertices) break;
        std::istringstream iss(line);
        int node_id;
        iss >> node_id;
        int neighbor;
        while (iss >> neighbor) {
            if (neighbor > 0 && neighbor <= num_vertices) {
                temp_graph[node_id].insert(neighbor);
                temp_graph[neighbor].insert(node_id); // Undirected
            }
        }
        vertex_id++;
    }
    gfile.close();

    // Build graph for local vertices
    std::set<std::pair<int, int>> unique_edges;
    for (const auto& [node, neighbors] : temp_graph) {
        if (partitions[node - 1] == rank) {
            graph.localVertices.push_back(node - 1);
            for (int neighbor : neighbors) {
                int u = node - 1;
                int v = neighbor - 1;
                int min_idx = std::min(u, v);
                int max_idx = std::max(u, v);
                unique_edges.insert({min_idx, max_idx});
            }
        }
    }

    for (const auto& [u, v] : unique_edges) {
        if (std::find(graph.localVertices.begin(), graph.localVertices.end(), u) != graph.localVertices.end()) {
            graph.adjacencyList[u].push_back({v, 1});
            std::cout << "Rank " << rank << ": Added edge (" << u << ", " << v << ")\n";
        }
        if (std::find(graph.localVertices.begin(), graph.localVertices.end(), v) != graph.localVertices.end()) {
            graph.adjacencyList[v].push_back({u, 1});
            std::cout << "Rank " << rank << ": Added edge (" << v << ", " << u << ")\n";
        }
    }

    std::cout << "Rank " << rank << ": Graph built with " << unique_edges.size() << " unique edges, "
              << graph.localVertices.size() << " local vertices\n";

    return graph;
}

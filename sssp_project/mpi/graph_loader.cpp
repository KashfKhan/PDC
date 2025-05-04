#include "graph_loader.h"
#include <fstream>
#include <sstream>
#include <iostream>

Graph load_partitioned_graph(const std::string& graphFile, const std::string& partFile, int rank) {
    std::ifstream gfile(graphFile);
    std::ifstream pfile(partFile);
    Graph graph;
    std::vector<int> partitions;

    if (!gfile || !pfile) {
        std::cerr << "Error opening graph or partition file\n";
        return graph;
    }

    int p;
    while (pfile >> p) partitions.push_back(p);

    int vertex_id = 0;
    std::string line;
    while (std::getline(gfile, line)) {
        if (partitions[vertex_id] != rank) {
            vertex_id++;
            continue;
        }

        std::istringstream iss(line);
        int neighbor, weight = 1;
        while (iss >> neighbor) {
            graph.adjacencyList[vertex_id].push_back({neighbor - 1, weight});  // METIS uses 1-based indexing
        }
        graph.localVertices.push_back(vertex_id);
        vertex_id++;
    }

    return graph;
}

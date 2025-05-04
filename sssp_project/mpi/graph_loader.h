#pragma once
#include <vector>
#include <string>
#include <unordered_map>

struct Edge {
    int dest;
    int weight;
};

struct Graph {
    std::unordered_map<int, std::vector<Edge>> adjacencyList;
    std::vector<int> localVertices;
};

Graph load_partitioned_graph(const std::string& graphFile, const std::string& partFile, int rank);

#pragma once
#include "graph_loader.h"
#include <vector>
#include <set>
#include <tuple>

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
);

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
);

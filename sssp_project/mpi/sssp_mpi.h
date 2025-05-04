#pragma once
#include <vector>
#include <set>
#include <tuple>
#include <limits>
#include <algorithm>
#include <mpi.h>
#include <iostream>
#include "graph_loader.h"

const int INF = std::numeric_limits<int>::max();

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
);

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
);

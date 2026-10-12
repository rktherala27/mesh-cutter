#pragma once

#include <unordered_map>
#include <cstddef>
#include <string>
#include <vector>

#include "Mesh.hpp"

// Result of the closed/oriented checks
struct TopologyReport
{
    bool isClosed = true;
    bool isConsistentlyOriented = true;
    std::vector<std::string> problems;
};

// Build edge -> faces map
std::unordered_map<std::size_t, std::vector<std::size_t>> buildEdgeFaceMap(const Mesh &mesh);

// Check every edge has exactly 2 faces
TopologyReport checkClosed(const Mesh &mesh);

void makeMeshConsistent(Mesh &mesh,
                        const std::unordered_map<std::size_t,
                                                 std::vector<std::size_t>> &edgeFaceMap);

// Check shared edges are traversed in opposite directions
TopologyReport checkMeshConsistentlyOriented(const Mesh &mesh);

double computeVolume(const Mesh &mesh);

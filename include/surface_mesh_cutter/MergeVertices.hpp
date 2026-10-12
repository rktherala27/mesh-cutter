#pragma once

#include "Mesh.hpp"

// Merges duplicate vertices in the mesh that lie within `tolerance`
// of each other in a given mesh data.
std::size_t mergeVertices(Mesh &mesh, double tolerance);
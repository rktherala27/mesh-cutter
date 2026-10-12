#pragma once

#include <string>

#include "Mesh.hpp"

// Reads an ASCII STL file into a triangle-soup Mesh.
// At this stage, vertices are NOT merged (welding is a separate step).
Mesh readAsciiStl(const std::string &path);

// Writes a Mesh as ASCII STL.
// Assumes 'mesh.vertices' and 'mesh.faces' are consistent
// (each face references 3 valid vertex indices).
void writeAsciiStl(const Mesh &mesh, const std::string &path);
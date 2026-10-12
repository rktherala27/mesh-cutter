#pragma once

#include <array>
#include <cstddef>
#include <vector>

#include "Vec3.hpp"

// A triangle face: three vertex indices into Mesh::vertices.
using Face = std::array<std::size_t, 3>;

struct Mesh
{
    std::vector<Vec3> vertices;
    std::vector<Face> faces;

    std::size_t vertexCount() const { return vertices.size(); }
    std::size_t faceCount() const { return faces.size(); }
};
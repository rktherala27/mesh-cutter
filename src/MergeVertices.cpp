#include "surface_mesh_cutter/MergeVertices.hpp"

#include <cmath>
#include <cstddef>
#include <unordered_map>
#include <vector>

#include "surface_mesh_cutter/Vec3.hpp"

namespace
{

    std::size_t cellHashKey(int x, int y, int z)
    {
        return (static_cast<std::size_t>(x) * 73856093) ^
               (static_cast<std::size_t>(y) * 19349663) ^
               (static_cast<std::size_t>(z) * 83492791);
    }

} // namespace

std::size_t mergeVertices(Mesh &mesh, double tolerance)
{
    if (tolerance <= 0.0)
        return 0;

    const double cell = tolerance;

    const double tolSq = tolerance * tolerance;

    std::unordered_map<std::size_t, std::vector<std::size_t>> grid;

    std::vector<std::size_t> remap(mesh.vertices.size());
    // Initialize remap with its old vertex numbering; later assign its
    // new index in the algorithm
    for (std::size_t i = 0; i < remap.size(); ++i)
        remap[i] = i;

    std::vector<Vec3> newVertices;
    newVertices.reserve(mesh.vertices.size());

    for (std::size_t i = 0; i < mesh.vertices.size(); ++i)
    {
        const Vec3 &v = mesh.vertices[i];

        int cellX = static_cast<int>(std::floor(v.x / cell));
        int cellY = static_cast<int>(std::floor(v.y / cell));
        int cellZ = static_cast<int>(std::floor(v.z / cell));

        bool foundDup = false;

        for (int dx = -1; dx <= 1 && !foundDup; ++dx)
        {
            for (int dy = -1; dy <= 1 && !foundDup; ++dy)
            {
                for (int dz = -1; dz <= 1 && !foundDup; ++dz)
                {
                    std::size_t key = cellHashKey(cellX + dx, cellY + dy, cellZ + dz);
                    auto it = grid.find(key);

                    if (it == grid.end())
                        continue;

                    for (std::size_t j : it->second)
                    {
                        const Vec3 &other = newVertices[j];

                        double dx2 = v.x - other.x;
                        double dy2 = v.y - other.y;
                        double dz2 = v.z - other.z;

                        double distSq = dx2 * dx2 + dy2 * dy2 + dz2 * dz2;

                        if (distSq <= tolSq)
                        {
                            remap[i] = j;
                            foundDup = true;
                            break;
                        }
                    }
                }
            }
        }

        if (!foundDup)
        {
            std::size_t newIdx = newVertices.size();
            newVertices.push_back(v);
            remap[i] = newIdx;
            grid[cellHashKey(cellX, cellY, cellZ)].push_back(newIdx);
        }
    }

    for (Face &f : mesh.faces)
    {
        f[0] = remap[f[0]];
        f[1] = remap[f[1]];
        f[2] = remap[f[2]];
    }

    std::size_t removedCount = mesh.vertices.size() - newVertices.size();

    mesh.vertices = std::move(newVertices);

    return removedCount;
}
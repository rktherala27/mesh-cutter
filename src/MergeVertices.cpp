#include "surface_mesh_cutter/MergeVertices.hpp"

#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <stdexcept>
#include <unordered_map>
#include <utility>
#include <vector>

#include "surface_mesh_cutter/Vec3.hpp"

namespace
{
    using CellIndex = std::int64_t;

    // Safely convert a coordinate to a grid-cell index.
    CellIndex computeCellIndex(double coordinate, double cellSize)
    {
        if (!std::isfinite(coordinate))
        {
            throw std::invalid_argument(
                "Vertex coordinates must be finite.");
        }

        if (!std::isfinite(cellSize) || cellSize <= 0.0)
        {
            throw std::invalid_argument(
                "Grid cell size must be finite and positive.");
        }

        const double index =
            std::floor(coordinate / cellSize);

        // Exactly 2^63 for a signed 64-bit CellIndex.
        // Avoid converting INT64_MAX to double, which could round upward.
        const double limit =
            std::ldexp(
                1.0,
                std::numeric_limits<CellIndex>::digits);

        // Validate before the floating-point-to-integer conversion.
        if (!std::isfinite(index) ||
            index <= -limit ||
            index >= limit)
        {
            throw std::overflow_error(
                "Grid index is out of range: coordinate is too "
                "large or tolerance is too small.");
        }

        const CellIndex result =
            static_cast<CellIndex>(index);

        // Leave room for the neighbor offsets -1 and +1.
        if (result == std::numeric_limits<CellIndex>::min() ||
            result == std::numeric_limits<CellIndex>::max())
        {
            throw std::overflow_error(
                "Grid index leaves no room for neighbor search.");
        }

        return result;
    }

    // Hash the three grid-cell coordinates.
    std::size_t cellHashKey(
        CellIndex x,
        CellIndex y,
        CellIndex z)
    {
        return (static_cast<std::size_t>(x) * 73856093u) ^
               (static_cast<std::size_t>(y) * 19349663u) ^
               (static_cast<std::size_t>(z) * 83492791u);
    }
} // namespace

std::size_t mergeVertices(Mesh &mesh, double tolerance)
{
    if (!std::isfinite(tolerance) || tolerance < 0.0)
    {
        throw std::invalid_argument(
            "Merge tolerance must be finite and non-negative.");
    }

    // Preserve the existing behavior: zero tolerance disables merging.
    if (tolerance == 0.0)
    {
        return 0;
    }

    const double cellSize = tolerance;

    std::unordered_map<std::size_t, std::vector<std::size_t>> grid;

    std::vector<std::size_t> remap(mesh.vertices.size());

    // Initialize with the original vertex numbering.
    for (std::size_t i = 0; i < remap.size(); ++i)
    {
        remap[i] = i;
    }

    std::vector<Vec3> newVertices;
    newVertices.reserve(mesh.vertices.size());

    for (std::size_t i = 0; i < mesh.vertices.size(); ++i)
    {
        const Vec3 &v = mesh.vertices[i];

        const CellIndex cellX = computeCellIndex(v.x, cellSize);
        const CellIndex cellY = computeCellIndex(v.y, cellSize);
        const CellIndex cellZ = computeCellIndex(v.z, cellSize);

        bool foundDup = false;

        // Search the current cell and its 26 neighboring cells.
        for (int dx = -1; dx <= 1 && !foundDup; ++dx)
        {
            for (int dy = -1; dy <= 1 && !foundDup; ++dy)
            {
                for (int dz = -1; dz <= 1 && !foundDup; ++dz)
                {
                    const std::size_t key = cellHashKey(
                        cellX + dx,
                        cellY + dy,
                        cellZ + dz);

                    const auto it = grid.find(key);

                    if (it == grid.end())
                    {
                        continue;
                    }

                    for (const std::size_t j : it->second)
                    {
                        const Vec3 &other = newVertices[j];

                        const double differenceX = v.x - other.x;
                        const double differenceY = v.y - other.y;
                        const double differenceZ = v.z - other.z;

                        // Compare distance directly without squaring.
                        const double distance = std::hypot(
                            differenceX,
                            differenceY,
                            differenceZ);

                        if (distance <= tolerance)
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
            const std::size_t newIndex = newVertices.size();

            newVertices.push_back(v);
            remap[i] = newIndex;

            grid[cellHashKey(cellX, cellY, cellZ)]
                .push_back(newIndex);
        }
    }

    // Update every face to use the merged vertex numbering.
    for (Face &face : mesh.faces)
    {
        face[0] = remap[face[0]];
        face[1] = remap[face[1]];
        face[2] = remap[face[2]];
    }

    std::vector<Face> validFaces;
    validFaces.reserve(mesh.faces.size());

    for (const Face &face : mesh.faces)
    {
        const bool isDegenerate =
            face[0] == face[1] ||
            face[1] == face[2] ||
            face[2] == face[0];

        if (!isDegenerate)
        {
            validFaces.push_back(face);
        }
    }

    mesh.faces = std::move(validFaces);

    const std::size_t removedCount =
        mesh.vertices.size() - newVertices.size();

    mesh.vertices = std::move(newVertices);

    return removedCount;
}
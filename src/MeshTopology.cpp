#include "surface_mesh_cutter/MeshTopology.hpp"
#include "surface_mesh_cutter/VectorMath.hpp"

#include <algorithm>
#include <stdexcept>
#include <queue>

std::unordered_map<std::size_t, std::vector<std::size_t>> buildEdgeFaceMap(const Mesh &mesh)
{
    std::unordered_map<std::size_t, std::vector<std::size_t>> edgeFaceMap;

    for (std::size_t f = 0; f < mesh.faces.size(); ++f)
    {
        const Face &face = mesh.faces[f];

        // Each triangle has 3 edges: (v0,v1), (v1,v2), (v2,v0)
        for (int e = 0; e < 3; ++e)
        {
            std::size_t a = face[e];
            std::size_t b = face[(e + 1) % 3];

            // Swap operation: smaller index first, so (a,b) == (b,a)
            // Needed to have a common 64-bit key calculation for both scenarios
            if (a > b)
                std::swap(a, b);

            // Pack both indices into one 64-bit key
            std::size_t key = (a << 32) | b;

            edgeFaceMap[key].push_back(f);
        }
    }

    return edgeFaceMap;
}

TopologyReport checkClosed(const Mesh &mesh)
{
    TopologyReport report;
    auto edgeFaceMap = buildEdgeFaceMap(mesh);

    for (const auto &[key, faces] : edgeFaceMap)
    {
        if (faces.size() != 2)
        {
            report.isClosed = false;
        }
    }

    return report;
}

void makeMeshConsistent(Mesh &mesh,
                        const std::unordered_map<std::size_t,
                                                 std::vector<std::size_t>> &edgeFaceMap)
{
    if (mesh.faces.empty())
    {
        throw std::invalid_argument("Error: Mesh has no faces to process.");
    }

    std::vector<bool> visited(mesh.faces.size(), false);
    std::queue<std::size_t> q;
    q.push(0);
    visited[0] = true;

    while (!q.empty())
    {
        std::size_t f = q.front();
        q.pop();

        const Face face0 = mesh.faces[f];

        for (int e1 = 0; e1 < 3; ++e1)
        {
            std::size_t a = face0[e1];
            std::size_t b = face0[(e1 + 1) % 3];

            std::size_t lo = std::min(a, b);
            std::size_t hi = std::max(a, b);
            std::size_t key = (lo << 32) | hi;

            auto it = edgeFaceMap.find(key);

            if (it == edgeFaceMap.end())
            {
                throw std::runtime_error("makeConsistent: edge not found in edgeFaceMap map/mesh inconsistency");
            }

            for (std::size_t g : it->second)
            {
                if (g == f || visited[g])
                    continue;

                const Face face1 = mesh.faces[g];

                bool sameDirection = false;
                for (int e2 = 0; e2 < 3; ++e2)
                {
                    std::size_t c = face1[e2];
                    std::size_t d = face1[(e2 + 1) % 3];
                    if (c == a && d == b)
                    {
                        sameDirection = true;
                        break;
                    }
                }

                if (sameDirection)
                    std::swap(mesh.faces[g][1], mesh.faces[g][2]);

                visited[g] = true;
                q.push(g);
            }
        }
    }
}

TopologyReport checkMeshConsistentlyOriented(const Mesh &mesh)
{
    TopologyReport report;
    report.isConsistentlyOriented = true;

    auto edgeFaceMap = buildEdgeFaceMap(mesh);

    for (const auto &[key, faces] : edgeFaceMap)
    {
        if (faces.size() != 2)
            continue; // only properly shared edges

        std::size_t f0 = faces[0];
        std::size_t f1 = faces[1];

        const auto &face0 = mesh.faces[f0];
        const auto &face1 = mesh.faces[f1];

        // Examine each of the 3 directed edges (a to b) in face0
        for (int e = 0; e < 3; ++e)
        {
            std::size_t a = face0[e];
            std::size_t b = face0[(e + 1) % 3];

            bool isSharedEdge = false;
            bool foundReverse = false;

            // Look for this exact edge in face1
            for (int e2 = 0; e2 < 3; ++e2)
            {
                std::size_t c = face1[e2];
                std::size_t d = face1[(e2 + 1) % 3];

                if (c == b && d == a)
                {
                    // Found the edge, and it is correctly reversed (consistent)
                    isSharedEdge = true;
                    foundReverse = true;
                    break;
                }
                else if (c == a && d == b)
                {
                    // Found the edge, but it is facing the SAME direction (inconsistent)
                    isSharedEdge = true;
                    foundReverse = false;
                    break;
                }
            }

            // If this is the edge connecting the two faces, but they flow in the same direction
            if (isSharedEdge && !foundReverse)
            {
                report.isConsistentlyOriented = false;
                return report;
            }
        }
    }

    return report;
}

double computeVolume(const Mesh &mesh)
{
    double signedVolume = 0.0;

    for (const Face &face : mesh.faces)
    {
        const Vec3 &a = mesh.vertices[face[0]];
        const Vec3 &b = mesh.vertices[face[1]];
        const Vec3 &c = mesh.vertices[face[2]];

        // Volume of tetrahedron formed with the origin
        signedVolume += dot(a, cross(b, c));
    }

    return signedVolume / 6.0;
}
#include <array>
#include <cstddef>
#include <map>
#include <vector>
#include <set>
#include <limits>
#include <cmath>
#include <stdexcept>

#include "surface_mesh_cutter/PlaneCut.hpp"
#include "surface_mesh_cutter/PlaneCutInternal.hpp"

using plane_cut_internal::VertexSide;

namespace
{
    Mesh buildCutMesh(
        const Mesh &originalMesh,
        const std::vector<Vec3> &newVertices,
        const std::vector<Face> &cutFaces,
        std::map<std::size_t, std::size_t> &vertexIndexMap)
    {
        Mesh cutMesh;

        vertexIndexMap.clear();

        const auto addVertex =
            [&](std::size_t originalIndex) -> std::size_t
        {
            const auto existing = vertexIndexMap.find(originalIndex);

            if (existing != vertexIndexMap.end())
            {
                return existing->second;
            }

            const std::size_t newIndex = cutMesh.vertices.size();

            if (originalIndex < originalMesh.vertices.size())
            {
                cutMesh.vertices.push_back(
                    originalMesh.vertices[originalIndex]);
            }
            else
            {
                const std::size_t newVertexIndex =
                    originalIndex - originalMesh.vertices.size();

                cutMesh.vertices.push_back(
                    newVertices[newVertexIndex]);
            }

            vertexIndexMap[originalIndex] = newIndex;

            return newIndex;
        };

        for (const Face &face : cutFaces)
        {
            Face remappedFace{};

            for (std::size_t localIndex = 0;
                 localIndex < 3;
                 ++localIndex)
            {
                remappedFace[localIndex] =
                    addVertex(face[localIndex]);
            }

            cutMesh.faces.push_back(remappedFace);
        }

        return cutMesh;
    }

    std::vector<std::size_t>
    orderCutLoop(const std::vector<std::array<std::size_t, 2>> &cutEdges)
    {
        // Each vertex maps to the unique vertices connected to it.
        std::map<std::size_t, std::set<std::size_t>> adjacency;

        for (const auto &edge : cutEdges)
        {
            adjacency[edge[0]].insert(edge[1]);
            adjacency[edge[1]].insert(edge[0]);
        }

        std::vector<std::size_t> loop;

        // Safely return empty loop if no cutEdges!
        if (cutEdges.empty())
        {
            return loop;
        }

        const std::size_t start = cutEdges[0][0];
        std::size_t current = start;
        std::size_t previous = std::numeric_limits<std::size_t>::max();

        do
        {
            loop.push_back(current);

            const std::set<std::size_t> &neighbors =
                adjacency[current];

            // Choose the first neighbor that is not the vertex
            // we just came from.
            std::size_t next = *neighbors.begin();

            if (next == previous && neighbors.size() > 1)
            {
                next = *std::next(neighbors.begin());
            }

            previous = current;
            current = next;

        } while (current != start);

        return loop;
    }

    // Compute the geometric normal of a polygon loop
    // using Newell's method. This works for non-planar-ish
    // polygons too, though our cut loops should be planar.
    Vec3 computeLoopNormal(const std::vector<std::size_t> &loop,
                           const Mesh &mesh)
    {
        Vec3 normal{0.0, 0.0, 0.0};

        const std::size_t count = loop.size();

        for (std::size_t i = 0; i < count; ++i)
        {
            const Vec3 &current =
                mesh.vertices[loop[i]];

            const Vec3 &next =
                mesh.vertices[loop[(i + 1) % count]];

            normal.x += (current.y - next.y) *
                        (current.z + next.z);

            normal.y += (current.z - next.z) *
                        (current.x + next.x);

            normal.z += (current.x - next.x) *
                        (current.y + next.y);
        }

        return normal;
    }

    void addCapFromLoop(Mesh &mesh,
                        const std::vector<std::size_t> &loop,
                        const Vec3 &desiredNormal)
    {
        if (loop.size() < 3)
        {
            return;
        }

        const Vec3 loopNormal =
            computeLoopNormal(loop, mesh);

        const double alignment =
            dot(loopNormal, desiredNormal);

        if (!std::isfinite(alignment) || alignment == 0.0)
        {
            throw std::runtime_error(
                "Cannot create cap: invalid or degenerate loop normal.");
        }

        Vec3 center{0.0, 0.0, 0.0};

        for (const std::size_t index : loop)
        {
            const Vec3 &vertex = mesh.vertices.at(index);

            center.x += vertex.x;
            center.y += vertex.y;
            center.z += vertex.z;
        }

        const double loopCount =
            1.0 / static_cast<double>(loop.size());

        center = center * loopCount;

        const std::size_t centerIndex =
            mesh.vertices.size();

        mesh.vertices.push_back(center);

        if (alignment > 0.0)
        {
            // Loop direction already matches the desired outward normal.
            for (std::size_t i = 0; i < loop.size(); ++i)
            {
                const std::size_t current = loop[i];

                const std::size_t next =
                    loop[(i + 1) % loop.size()];

                mesh.faces.push_back(
                    Face{centerIndex, current, next});
            }
        }
        else
        {
            // Reverse every cap triangle to face outward.
            for (std::size_t i = 0; i < loop.size(); ++i)
            {
                const std::size_t current = loop[i];

                const std::size_t next =
                    loop[(i + 1) % loop.size()];

                mesh.faces.push_back(
                    Face{centerIndex, next, current});
            }
        }
    }
} // namespace

PlaneCutResult cutMeshWithPlane(
    const Mesh &mesh,
    const Plane &plane,
    double tolerance)
{
    PlaneCutResult result;

    const auto vertexData =
        plane_cut_internal::classifyMeshVertices(
            mesh, plane, tolerance);

    for (const VertexSide side : vertexData.sides)
    {
        if (side == VertexSide::OnPlane)
        {
            result.messages.push_back(
                "Vertices exactly on the plane are not supported yet.");

            return result;
        }
    }

    std::vector<Vec3> newVertices;
    std::size_t nextNewVertexIndex = mesh.vertices.size();

    std::map<std::array<std::size_t, 2>, std::size_t>
        intersectionVertexCache;
    std::vector<std::array<std::size_t, 2>> cutEdges;

    std::vector<Face> aboveFaces;
    std::vector<Face> belowFaces;

    for (const Face &face : mesh.faces)
    {
        const auto segment =
            plane_cut_internal::intersectFace(
                mesh, face, vertexData);

        if (!segment.has_value())
        {
            const plane_cut_internal::VertexSide side =
                vertexData.sides[face[0]];

            if (side == plane_cut_internal::VertexSide::Above)
            {
                aboveFaces.push_back(face);
            }
            else
            {
                belowFaces.push_back(face);
            }

            continue;
        }

        const auto split =
            plane_cut_internal::splitFace(
                mesh,
                face,
                vertexData,
                *segment,
                newVertices,
                nextNewVertexIndex,
                intersectionVertexCache);

        for (const Face &aboveFace : split.aboveFaces)
        {
            aboveFaces.push_back(aboveFace);
        }

        for (const Face &belowFace : split.belowFaces)
        {
            belowFaces.push_back(belowFace);
        }
        cutEdges.push_back(split.cutEdge);
    }

    std::map<std::size_t, std::size_t> aboveVertexIndexMap;
    std::map<std::size_t, std::size_t> belowVertexIndexMap;

    result.above = buildCutMesh(
        mesh,
        newVertices,
        aboveFaces,
        aboveVertexIndexMap);

    result.below = buildCutMesh(
        mesh,
        newVertices,
        belowFaces,
        belowVertexIndexMap);

    result.cutPerformed =
        !result.above.faces.empty() &&
        !result.below.faces.empty();

    // Build the closed intersection loop from the recorded cut edges.
    const std::vector<std::size_t> cutLoop = orderCutLoop(cutEdges);

    if (cutLoop.empty())
    {
        result.messages.push_back(
            "No intersection loop was found; no caps were created.");

        return result;
    }

    std::vector<std::size_t> aboveLoop;
    std::vector<std::size_t> belowLoop;

    aboveLoop.reserve(cutLoop.size());
    belowLoop.reserve(cutLoop.size());

    for (const std::size_t oldIndex : cutLoop)
    {
        aboveLoop.push_back(
            aboveVertexIndexMap.at(oldIndex));

        belowLoop.push_back(
            belowVertexIndexMap.at(oldIndex));
    }

    // Make the plane normal a unit vector.
    const Vec3 unitPlaneNormal = plane.normal * (1.0 / length(plane.normal));

    // The above solid occupies the positive side of the plane,
    // so its cap must face opposite to the plane normal.
    const Vec3 aboveCapNormal = unitPlaneNormal * -1.0;

    // The below solid occupies the negative side of the plane,
    // so its cap must face along the plane normal.
    const Vec3 belowCapNormal = unitPlaneNormal;

    // Add one correctly oriented cap to each output mesh.
    addCapFromLoop(result.above, aboveLoop, aboveCapNormal);
    addCapFromLoop(result.below, belowLoop, belowCapNormal);

    result.cutPerformed =
        !result.above.faces.empty() &&
        !result.below.faces.empty();

    result.messages.push_back("Created cap faces from the remapped cut boundary loop.");

    return result;
}

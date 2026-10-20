#include <array>
#include <cstddef>
#include <map>
#include <vector>

#include "surface_mesh_cutter/PlaneCut.hpp"
#include "surface_mesh_cutter/PlaneCutInternal.hpp"

using plane_cut_internal::VertexSide;

namespace
{
    Mesh buildCutMesh(
        const Mesh &originalMesh,
        const std::vector<Vec3> &newVertices,
        const std::vector<Face> &cutFaces)
    {
        Mesh cutMesh;

        std::map<std::size_t, std::size_t> vertexIndexMap;

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
    }

    result.above = buildCutMesh(
        mesh,
        newVertices,
        aboveFaces);

    result.below = buildCutMesh(
        mesh,
        newVertices,
        belowFaces);

    result.cutPerformed =
        !result.above.faces.empty() &&
        !result.below.faces.empty();

    result.messages.push_back(
        "Intersection loop extraction is not implemented yet.");

    return result;
}

#pragma once

#include <vector>
#include <array>
#include <cstddef>
#include <optional>
#include <map>

#include "surface_mesh_cutter/PlaneCut.hpp"

namespace plane_cut_internal
{

    enum class VertexSide
    {
        Below,
        OnPlane,
        Above
    };

    struct VertexPlaneData
    {
        std::vector<double> distances;
        std::vector<VertexSide> sides;
    };

    struct FaceIntersectionSegment
    {
        // Two endpoints of the intersection segment.
        std::array<Vec3, 2> points;

        // The mesh edge containing each endpoint.
        // Each edge stores its two vertex indices in ascending order.
        std::array<std::array<std::size_t, 2>, 2> sourceEdges;
    };

    struct SplitFaceResult
    {
        std::vector<Face> aboveFaces;
        std::vector<Face> belowFaces;

        // The two intersection vertices created for this face.
        // This edge lies on the final cut boundary.
        std::array<std::size_t, 2> cutEdge{};
    };

    std::vector<double> computeSignedDistances(
        const Mesh &mesh,
        const Plane &plane);

    VertexPlaneData classifyMeshVertices(
        const Mesh &mesh,
        const Plane &plane,
        double tolerance);

    // std::optional lets to return std::nullopt if there is no
    // intersection to a mesh face
    std::optional<FaceIntersectionSegment> intersectFace(
        const Mesh &mesh,
        const Face &face,
        const VertexPlaneData &vertexData);

    SplitFaceResult splitFace(
        const Mesh &mesh,
        const Face &face,
        const VertexPlaneData &vertexData,
        const FaceIntersectionSegment &segment,
        std::vector<Vec3> &newVertices,
        std::size_t &nextNewVertexIndex,
        std::map<std::array<std::size_t, 2>, std::size_t>
            &intersectionVertexCache);

} // namespace plane_cut_internal
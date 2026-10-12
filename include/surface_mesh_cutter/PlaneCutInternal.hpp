#pragma once

#include <vector>

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

    std::vector<double> computeSignedDistances(
        const Mesh &mesh,
        const Plane &plane);

    VertexPlaneData classifyMeshVertices(
        const Mesh &mesh,
        const Plane &plane,
        double tolerance);

} // namespace plane_cut_internal
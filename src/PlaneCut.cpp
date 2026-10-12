#include "surface_mesh_cutter/PlaneCutInternal.hpp"

#include <cmath>
#include <stdexcept>
#include <vector>

#include "surface_mesh_cutter/VectorMath.hpp"

namespace plane_cut_internal
{

    std::vector<double> computeSignedDistances(const Mesh &mesh,
                                               const Plane &plane)
    {
        const double normalLength = length(plane.normal);

        if (!std::isfinite(normalLength) || normalLength == 0.0)
        {
            throw std::invalid_argument(
                "Plane normal must be finite and non-zero.");
        }

        const Vec3 unitNormal = plane.normal * (1.0 / normalLength);

        std::vector<double> distances;
        distances.reserve(mesh.vertices.size());

        for (const Vec3 &vertex : mesh.vertices)
        {
            const double distance = dot(vertex - plane.point, unitNormal);

            if (!std::isfinite(distance))
            {
                throw std::invalid_argument(
                    "Plane or mesh contains non-finite coordinates.");
            }

            distances.push_back(distance);
        }
        return distances;
    }

    VertexPlaneData classifyMeshVertices(
        const Mesh &mesh,
        const Plane &plane,
        double tolerance)
    {
        if (!std::isfinite(tolerance) || tolerance < 0.0)
        {
            throw std::invalid_argument(
                "Plane-cut tolerance must be finite and non-negative.");
        }

        const double normalLength = length(plane.normal);

        if (!std::isfinite(normalLength) || normalLength == 0.0)
        {
            throw std::invalid_argument(
                "Plane normal must be finite and non-zero.");
        }

        const Vec3 unitNormal = plane.normal * (1.0 / normalLength);

        VertexPlaneData data;
        data.distances.reserve(mesh.vertices.size());
        data.sides.reserve(mesh.vertices.size());

        for (const Vec3 &vertex : mesh.vertices)
        {
            const double distance = dot(vertex - plane.point, unitNormal);

            if (!std::isfinite(distance))
            {
                throw std::invalid_argument(
                    "Mesh or plane contains non-finite coordinates.");
            }

            data.distances.push_back(distance);

            if (distance > tolerance)
            {
                data.sides.push_back(VertexSide::Above);
            }
            else if (distance < -tolerance)
            {
                data.sides.push_back(VertexSide::Below);
            }
            else
            {
                data.sides.push_back(VertexSide::OnPlane);
            }
        }

        return data;
    }
}
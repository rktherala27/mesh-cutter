#include <cmath>
#include <stdexcept>
#include <vector>
#include <algorithm>
#include <limits>

#include "surface_mesh_cutter/VectorMath.hpp"
#include "surface_mesh_cutter/PlaneCutInternal.hpp"

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

    std::optional<FaceIntersectionSegment> intersectFace(
        const Mesh &mesh,
        const Face &face,
        const VertexPlaneData &vertexData)
    {
        std::optional<std::size_t> onPlaneVertex;

        // Validate the three vertex indices and reject unsupported cases.
        for (std::size_t index : face)
        {
            if (index >= mesh.vertices.size() ||
                index >= vertexData.distances.size() ||
                index >= vertexData.sides.size())
            {
                throw std::out_of_range(
                    "Face index is outside the mesh or vertex-plane data.");
            }

            if (vertexData.sides[index] == VertexSide::OnPlane)
            {
                if (onPlaneVertex.has_value())
                {
                    throw std::invalid_argument(
                        "On-plane edges and coplanar faces "
                        "are not supported yet.");
                }

                onPlaneVertex = index;
            }
        }

        FaceIntersectionSegment segment{};
        std::size_t pointCount = 0;

        // Reuse the existing vertex rather than interpolating it.
        if (onPlaneVertex.has_value())
        {
            const std::size_t index = onPlaneVertex.value();

            segment.points[pointCount] = mesh.vertices[index];

            // {index, index} identifies an original mesh vertex,
            // rather than a crossing in the interior of an edge.
            segment.sourceEdges[pointCount] = {index, index};

            ++pointCount;
        }

        // Visit the face's three edges.
        for (std::size_t localIndex = 0; localIndex < 3; ++localIndex)
        {
            std::size_t a = face[localIndex];
            std::size_t b = face[(localIndex + 1) % 3];

            const VertexSide sideA = vertexData.sides[a];
            const VertexSide sideB = vertexData.sides[b];

            const bool oppositeSides =
                (sideA == VertexSide::Above &&
                 sideB == VertexSide::Below) ||
                (sideA == VertexSide::Below &&
                 sideB == VertexSide::Above);

            if (!oppositeSides)
            {
                continue;
            }

            // Use the same ordering for a shared mesh edge.
            if (a > b)
            {
                std::swap(a, b);
            }

            const double da = vertexData.distances[a];
            const double db = vertexData.distances[b];

            const double t = da / (da - db);

            const Vec3 intersection =
                mesh.vertices[a] +
                (mesh.vertices[b] - mesh.vertices[a]) * t;

            if (pointCount >= segment.points.size())
            {
                throw std::logic_error(
                    "A triangle produced more than two intersection points.");
            }

            segment.points[pointCount] = intersection;
            segment.sourceEdges[pointCount] = {a, b};

            ++pointCount;
        }

        // No intersection, or only a single-vertex touch.
        if (pointCount < 2)
        {
            return std::nullopt;
        }

        return segment;
    }

    std::size_t getOrCreateIntersectionVertex(
        const FaceIntersectionSegment &segment,
        const std::array<std::size_t, 2> &edge,
        std::vector<Vec3> &newVertices,
        std::size_t &nextNewVertexIndex,
        std::map<std::array<std::size_t, 2>, std::size_t>
            &intersectionVertexCache)
    {
        const auto existing = intersectionVertexCache.find(edge);

        if (existing != intersectionVertexCache.end())
        {
            return existing->second;
        }

        // Find which of the two segment endpoints (intersection points) came from this edge.
        for (std::size_t i = 0; i < segment.sourceEdges.size(); ++i)
        {
            if (segment.sourceEdges[i] == edge)
            {
                const std::size_t newVertexIndex = nextNewVertexIndex;

                newVertices.push_back(segment.points[i]);

                intersectionVertexCache[edge] = newVertexIndex;

                ++nextNewVertexIndex;

                return newVertexIndex;
            }
        }

        throw std::logic_error(
            "Intersection segment does not contain the expected edge.");
    }

    void triangulatePolygon(
        const std::vector<std::size_t> &polygon,
        std::vector<Face> &faces)
    {
        if (polygon.size() < 3)
        {
            return;
        }

        // Split a polygon into triangles using a fan from its first vertex.
        for (std::size_t i = 1; i + 1 < polygon.size(); ++i)
        {
            faces.push_back(Face{
                polygon[0],
                polygon[i],
                polygon[i + 1]});
        }
    }

    SplitFaceResult splitFace(
        const Mesh &mesh,
        const Face &face,
        const VertexPlaneData &vertexData,
        const FaceIntersectionSegment &segment,
        std::vector<Vec3> &newVertices,
        std::size_t &nextNewVertexIndex,
        std::map<std::array<std::size_t, 2>, std::size_t>
            &intersectionVertexCache)
    {
        SplitFaceResult result;

        std::vector<std::size_t> abovePolygon;
        std::vector<std::size_t> belowPolygon;

        std::size_t firstIntersectionIndex =
            std::numeric_limits<std::size_t>::max();

        std::size_t secondIntersectionIndex =
            std::numeric_limits<std::size_t>::max();

        for (std::size_t localIndex = 0; localIndex < 3; ++localIndex)
        {
            const std::size_t current = face[localIndex];
            const std::size_t next = face[(localIndex + 1) % 3];

            const VertexSide currentSide = vertexData.sides[current];
            const VertexSide nextSide = vertexData.sides[next];

            // Add the original vertex to the polygon for its side.
            if (currentSide == VertexSide::Above)
            {
                abovePolygon.push_back(current);
            }
            else if (currentSide == VertexSide::Below)
            {
                belowPolygon.push_back(current);
            }
            else // OnPlane
            {
                abovePolygon.push_back(current);
                belowPolygon.push_back(current);
            }

            if (currentSide != nextSide)
            {
                std::array<std::size_t, 2> edge;

                if (currentSide == VertexSide::OnPlane)
                {
                    // current itself is the intersection point.
                    edge = {current, current};
                }
                else if (nextSide == VertexSide::OnPlane)
                {
                    // next itself is the intersection point.
                    edge = {next, next};
                }
                else
                {
                    // Ordinary crossing: the plane crosses the interior
                    // of edge current-next.
                    edge = {
                        std::min(current, next),
                        std::max(current, next)};
                }

                const std::size_t intersectionIndex =
                    getOrCreateIntersectionVertex(
                        segment,
                        edge,
                        newVertices,
                        nextNewVertexIndex,
                        intersectionVertexCache);

                // Store the first and second intersection vertices
                //  in the order they are encountered around the triangle.
                if (firstIntersectionIndex ==
                    std::numeric_limits<std::size_t>::max())
                {
                    firstIntersectionIndex = intersectionIndex;
                }
                else
                {
                    secondIntersectionIndex = intersectionIndex;
                }

                // This edge is one boundary edge of the final cut loop.
                // We intentionally do not sort these indices.
                result.cutEdge = {
                    firstIntersectionIndex,
                    secondIntersectionIndex};

                abovePolygon.push_back(intersectionIndex);
                belowPolygon.push_back(intersectionIndex);
            }
        }

        triangulatePolygon(abovePolygon, result.aboveFaces);
        triangulatePolygon(belowPolygon, result.belowFaces);

        return result;
    }
} // namespace plane_cut_internal
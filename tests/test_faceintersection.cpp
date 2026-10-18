#include "surface_mesh_cutter/PlaneCutInternal.hpp"
#include "TestHelpers.hpp"

#include <array>
#include <cmath>
#include <cstddef>
#include <iostream>

namespace
{

    // Shared triangle in the XY plane.
    Mesh makeTriangle()
    {
        Mesh mesh;

        mesh.vertices = {
            {0.0, 0.0, 0.0}, // A: vertex 0
            {2.0, 0.0, 0.0}, // B: vertex 1
            {0.0, 2.0, 0.0}  // C: vertex 2
        };

        mesh.faces.push_back(Face{0, 1, 2});

        return mesh;
    }

    std::optional<std::size_t> findIntersectionPointBySourceEdge(
        const plane_cut_internal::FaceIntersectionSegment &segment,
        const std::array<std::size_t, 2> &expectedEdge)
    {
        for (std::size_t i = 0; i < segment.sourceEdges.size(); ++i)
        {
            if (segment.sourceEdges[i] == expectedEdge)
            {
                return i;
            }
        }

        return std::nullopt;
    }

    void checkIntersectionPoint(
        const plane_cut_internal::FaceIntersectionSegment &segment,
        const std::array<std::size_t, 2> &expectedEdge,
        const Vec3 &expectedPoint,
        const std::string &message)
    {
        const auto endpointIndex =
            findIntersectionPointBySourceEdge(segment, expectedEdge);

        check(endpointIndex.has_value(), message);

        check(endpointIndex.has_value() &&
                  pointsAreClose(
                      segment.points[*endpointIndex],
                      expectedPoint,
                      1e-9),
              message);
    }

    // Tests 1 and 2: all vertices are on the same side,
    // so the face should not produce an intersection segment.
    void testAllAboveOrBelow()
    {
        const Mesh mesh = makeTriangle();

        // Test 1: all vertices are above x = -1.
        {
            const Plane plane{
                Vec3{-1.0, 0.0, 0.0},
                Vec3{1.0, 0.0, 0.0}};

            const auto vertexData =
                plane_cut_internal::classifyMeshVertices(
                    mesh, plane, 1e-9);

            const auto result =
                plane_cut_internal::intersectFace(
                    mesh, mesh.faces[0], vertexData);

            check(!result.has_value(),
                  "Test 1: all-above vertices, triangle should not produce a segment.");
        }

        // Test 2: all vertices are below x = 3.
        {
            const Plane plane{
                Vec3{3.0, 0.0, 0.0},
                Vec3{1.0, 0.0, 0.0}};

            const auto vertexData =
                plane_cut_internal::classifyMeshVertices(
                    mesh, plane, 1e-9);

            const auto result =
                plane_cut_internal::intersectFace(
                    mesh, mesh.faces[0], vertexData);

            check(!result.has_value(),
                  "Test 2: all-below vertices, triangle should not produce a segment.");
        }
    }

    // Tests 3 and 4: ordinary crossings.
    // The plane crosses two edges of the triangle and should return
    // the correct interpolated intersection points and source edges.
    void testOrdinaryCrossings()
    {
        const Mesh mesh = makeTriangle();

        // Test 3: one above, two below.
        // Plane x = 1 intersects edge A-B at (1, 0, 0)
        // and edge B-C at (1, 1, 0).
        {
            const Plane plane{
                Vec3{1.0, 0.0, 0.0},
                Vec3{1.0, 0.0, 0.0}};

            const auto vertexData =
                plane_cut_internal::classifyMeshVertices(
                    mesh, plane, 1e-9);

            const auto result =
                plane_cut_internal::intersectFace(
                    mesh, mesh.faces[0], vertexData);

            check(result.has_value(),
                  "Test 3: plane x = 1 should cross the triangle.");

            checkIntersectionPoint(
                *result,
                {0, 1},
                Vec3{1.0, 0.0, 0.0},
                "Test 3: intersection point on edge A-B should be (1, 0, 0).");

            checkIntersectionPoint(
                *result,
                {1, 2},
                Vec3{1.0, 1.0, 0.0},
                "Test 3: intersection point on edge B-C should be (1, 1, 0).");
        }

        // Test 4: one below, two above.
        // Plane through (0.5, 0.5, 0) with normal (1, 1, 0).
        // A is below; B and C are above.
        // It intersects edge A-B at (1, 0, 0)
        // and edge A-C at (0, 1, 0).
        {
            const Plane plane{
                Vec3{0.5, 0.5, 0.0},
                Vec3{1.0, 1.0, 0.0}};

            const auto vertexData =
                plane_cut_internal::classifyMeshVertices(
                    mesh, plane, 1e-9);

            const auto result =
                plane_cut_internal::intersectFace(
                    mesh, mesh.faces[0], vertexData);

            check(result.has_value(),
                  "Test 4: plane should cross the triangle.");

            checkIntersectionPoint(
                *result,
                {0, 1},
                Vec3{1.0, 0.0, 0.0},
                "Test 4: intersection point on edge A-B should be (1, 0, 0).");

            checkIntersectionPoint(
                *result,
                {0, 2},
                Vec3{0.0, 1.0, 0.0},
                "Test 4: intersection point on edge A-C should be (0, 1, 0).");
        }
    }

    // Tests 5 and 6: non-midpoint interpolation and reversed winding.
    void testInterpolationAndWinding()
    {
        const Mesh mesh = makeTriangle();

        // Test 5: crossing is not at the edge midpoints.
        // Plane x = 0.5 intersects edge A-B at t = 0.25,
        // giving (0.5, 0, 0).
        // It intersects edge B-C at t = 0.75,
        // giving (0.5, 1.5, 0).
        {
            const Plane plane{
                Vec3{0.5, 0.0, 0.0},
                Vec3{1.0, 0.0, 0.0}};

            const auto vertexData =
                plane_cut_internal::classifyMeshVertices(
                    mesh, plane, 1e-9);

            const auto result =
                plane_cut_internal::intersectFace(
                    mesh, mesh.faces[0], vertexData);

            check(result.has_value(),
                  "Test 5: plane x = 0.5 should cross the triangle.");

            checkIntersectionPoint(
                *result,
                {0, 1},
                Vec3{0.5, 0.0, 0.0},
                "Test 5: intersection point on edge A-B should be (0.5, 0, 0).");

            checkIntersectionPoint(
                *result,
                {1, 2},
                Vec3{0.5, 1.5, 0.0},
                "Test 5: intersection point on edge B-C should be (0.5, 1.5, 0).");
        }

        // Test 6: reverse the triangle's winding.
        // The same plane x = 1 should produce the same two intersection
        // points and the same source edges, ignoring endpoint order.
        {
            Mesh reversedMesh = makeTriangle();

            // Original winding: {0, 1, 2}
            // Reversed winding: {0, 2, 1}
            reversedMesh.faces[0] = Face{0, 2, 1};

            const Plane plane{
                Vec3{1.0, 0.0, 0.0},
                Vec3{1.0, 0.0, 0.0}};

            const auto vertexData =
                plane_cut_internal::classifyMeshVertices(
                    reversedMesh, plane, 1e-9);

            const auto result =
                plane_cut_internal::intersectFace(
                    reversedMesh, reversedMesh.faces[0], vertexData);

            check(result.has_value(),
                  "Test 6: plane x = 1 should cross the reversed triangle.");

            checkIntersectionPoint(
                *result,
                {0, 1},
                Vec3{1.0, 0.0, 0.0},
                "Test 6: intersection point on edge A-B should be (1, 0, 0).");

            checkIntersectionPoint(
                *result,
                {1, 2},
                Vec3{1.0, 1.0, 0.0},
                "Test 6: intersection point on edge B-C should be (1, 1, 0).");
        }
    }

    // Tests 7, 8, and 9: one vertex on a tilted plane.
    void testVertexOnPlane()
    {
        const Mesh mesh = makeTriangle();

        // Test 7: one exactly on-plane vertex,
        // with the other two vertices on opposite sides.
        // Plane x + y = 0 contains vertex A.
        // B is above; C is below.
        {
            Mesh testMesh = makeTriangle();

            // Move C below the tilted plane.
            testMesh.vertices[2] = Vec3{-2.0, 1.0, 0.0};

            const Plane plane{
                Vec3{0.0, 0.0, 0.0},
                Vec3{1.0, 1.0, 0.0}};

            const auto vertexData =
                plane_cut_internal::classifyMeshVertices(
                    testMesh, plane, 1e-9);

            const auto result =
                plane_cut_internal::intersectFace(
                    testMesh, testMesh.faces[0], vertexData);

            check(result.has_value(),
                  "Test 7: tilted plane through A with B and C on opposite sides should produce a segment.");

            // The on-plane vertex A should be reused exactly.
            checkIntersectionPoint(
                *result,
                {0, 0},
                Vec3{0.0, 0.0, 0.0},
                "Test 7: intersection should reuse original vertex A.");

            // The plane crosses edge B-C at (-2/3, 2/3, 0).
            checkIntersectionPoint(
                *result,
                {1, 2},
                Vec3{-2.0 / 3.0, 2.0 / 3.0, 0.0},
                "Test 7: intersection point on edge B-C should be (-2/3, 2/3, 0).");
        }

        // Test 8: one exactly on-plane vertex,
        // with both other vertices on the same side.
        // Plane x + y = 0 contains vertex A;
        // B and C are both above.
        // Expected: no segment, because this is only a point touch.
        {
            const Mesh sameSideMesh = makeTriangle();

            const Plane plane{
                Vec3{0.0, 0.0, 0.0},
                Vec3{1.0, 1.0, 0.0}};

            const auto vertexData =
                plane_cut_internal::classifyMeshVertices(
                    sameSideMesh, plane, 1e-9);

            const auto result =
                plane_cut_internal::intersectFace(
                    sameSideMesh, sameSideMesh.faces[0], vertexData);

            check(!result.has_value(),
                  "Test 8: one on-plane vertex with both others on the same side should not produce a segment.");
        }

        // Test 9: one near-plane vertex within tolerance,
        // with the other two vertices on opposite sides.
        // Vertex A is slightly above the tilted plane, but within tolerance,
        // so it should be treated as OnPlane and reused exactly.
        {
            Mesh nearPlaneMesh = makeTriangle();

            // Move A slightly along the plane normal.
            nearPlaneMesh.vertices[0] =
                Vec3{1e-12, 1e-12, 0.0};

            // Move C below the tilted plane.
            nearPlaneMesh.vertices[2] =
                Vec3{-2.0, 1.0, 0.0};

            const Plane plane{
                Vec3{0.0, 0.0, 0.0},
                Vec3{1.0, 1.0, 0.0}};

            const auto vertexData =
                plane_cut_internal::classifyMeshVertices(
                    nearPlaneMesh, plane, 1e-9);

            const auto result =
                plane_cut_internal::intersectFace(
                    nearPlaneMesh, nearPlaneMesh.faces[0], vertexData);

            check(result.has_value(),
                  "Test 9: near-plane vertex with opposite-side vertices should produce a segment.");

            // The near-plane vertex should be reused with its original coordinate.
            checkIntersectionPoint(
                *result,
                {0, 0},
                Vec3{1e-12, 1e-12, 0.0},
                "Test 9: near-plane vertex A should be reused exactly.");

            // The plane crosses edge B-C at (-2/3, 2/3, 0).
            checkIntersectionPoint(
                *result,
                {1, 2},
                Vec3{-2.0 / 3.0, 2.0 / 3.0, 0.0},
                "Test 9: intersection point on edge B-C should be (-2/3, 2/3, 0).");
        }
    }

    // Tests 10, 11, and 12: unsupported and invalid inputs.
    void testInvalidInputs()
    {
        // Test 10: two vertices OnPlane.
        // Plane y = 0 contains vertices A and B.
        {
            const Mesh mesh = makeTriangle();

            const Plane plane{
                Vec3{0.0, 0.0, 0.0},
                Vec3{0.0, 1.0, 0.0}};

            const auto vertexData =
                plane_cut_internal::classifyMeshVertices(
                    mesh, plane, 1e-9);

            check(
                expectThrowsAs<std::invalid_argument>(
                    [&]()
                    {
                        plane_cut_internal::intersectFace(
                            mesh, mesh.faces[0], vertexData);
                    }),
                "Test 10: two OnPlane vertices should throw std::invalid_argument.");
        }

        // Test 11: three vertices OnPlane.
        // Plane z = 0 contains the entire triangle.
        {
            const Mesh mesh = makeTriangle();

            const Plane plane{
                Vec3{0.0, 0.0, 0.0},
                Vec3{0.0, 0.0, 1.0}};

            const auto vertexData =
                plane_cut_internal::classifyMeshVertices(
                    mesh, plane, 1e-9);

            check(
                expectThrowsAs<std::invalid_argument>(
                    [&]()
                    {
                        plane_cut_internal::intersectFace(
                            mesh, mesh.faces[0], vertexData);
                    }),
                "Test 11: three OnPlane vertices should throw std::invalid_argument.");
        }

        // Test 12: face contains an out-of-range vertex index.
        {
            Mesh mesh = makeTriangle();

            // Vertex index 3 does not exist:
            // the mesh has only vertices 0, 1, and 2.
            mesh.faces[0] = Face{0, 1, 3};

            const Plane plane{
                Vec3{1.0, 0.0, 0.0},
                Vec3{1.0, 0.0, 0.0}};

            const auto vertexData =
                plane_cut_internal::classifyMeshVertices(
                    mesh, plane, 1e-9);

            check(
                expectThrowsAs<std::out_of_range>(
                    [&]()
                    {
                        plane_cut_internal::intersectFace(
                            mesh, mesh.faces[0], vertexData);
                    }),
                "Test 12: out-of-range face vertex index should throw std::out_of_range.");
        }
    }
} // namespace

int main()
{

    try
    {
        testAllAboveOrBelow();
        testOrdinaryCrossings();
        testInterpolationAndWinding();
        testVertexOnPlane();
        testInvalidInputs();

        std::cout << "All face-intersection tests passed.\n";
        return 0;
    }
    catch (const std::exception &error)
    {
        std::cerr << "Test failed: " << error.what() << '\n';
        return 1;
    }

    return 0;
}

#include <array>
#include <cstddef>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>
#include <vector>

#include "TestHelpers.hpp"

#include "surface_mesh_cutter/PlaneCutInternal.hpp"
#include "surface_mesh_cutter/VectorMath.hpp"

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

    // Test 1: one vertex above, two vertices below.
    void testSplitFaceOneAboveTwoBelow()
    {
        const Mesh mesh = makeTriangle();

        const Plane plane{
            Vec3{1.0, 0.0, 0.0},
            Vec3{1.0, 0.0, 0.0}};

        const auto vertexData =
            plane_cut_internal::classifyMeshVertices(
                mesh, plane, 1e-9);

        const auto segment =
            plane_cut_internal::intersectFace(
                mesh, mesh.faces[0], vertexData);

        check(segment.has_value(),
              "One above/two below: plane should cross the triangle.");

        std::vector<Vec3> newVertices;
        std::size_t nextNewVertexIndex = mesh.vertices.size();

        std::map<std::array<std::size_t, 2>, std::size_t>
            intersectionVertexCache;

        const auto split =
            plane_cut_internal::splitFace(
                mesh,
                mesh.faces[0],
                vertexData,
                *segment,
                newVertices,
                nextNewVertexIndex,
                intersectionVertexCache);

        check(newVertices.size() == 2,
              "One above/two below: expected two new vertices.");

        check(pointsAreClose(
                  newVertices[0],
                  Vec3{1.0, 0.0, 0.0},
                  1e-9),
              "One above/two below: P should be (1, 0, 0).");

        check(pointsAreClose(
                  newVertices[1],
                  Vec3{1.0, 1.0, 0.0},
                  1e-9),
              "One above/two below: Q should be (1, 1, 0).");

        check(split.aboveFaces.size() == 1,
              "One above/two below: expected one above face.");

        check(split.belowFaces.size() == 2,
              "One above/two below: expected two below faces.");

        check(split.aboveFaces[0] == Face{3, 1, 4},
              "One above/two below: above face should be 3, 1, 4.");

        check(split.belowFaces[0] == Face{0, 3, 4},
              "One above/two below: first below face should be 0, 3, 4.");

        check(split.belowFaces[1] == Face{0, 4, 2},
              "One above/two below: second below face should be 0, 4, 2.");
    }

    // Test 2: one vertex below, two vertices above.
    //
    // Same plane geometry as Test 1: x = 1.
    // Reversing the normal swaps the above and below sides.
    //
    // Triangle:
    // Vertex 0 = (0, 0, 0), Above
    // Vertex 1 = (2, 0, 0), Below
    // Vertex 2 = (0, 2, 0), Above
    //
    // Plane crossings:
    // Edge 0-1 at vertex 3 = (1, 0, 0)
    // Edge 1-2 at vertex 4 = (1, 1, 0)
    void testSplitFaceOneBelowTwoAbove()
    {
        const Mesh mesh = makeTriangle();

        const Plane plane{
            Vec3{1.0, 0.0, 0.0},
            Vec3{-1.0, 0.0, 0.0}};

        const auto vertexData =
            plane_cut_internal::classifyMeshVertices(
                mesh, plane, 1e-9);

        const auto segment =
            plane_cut_internal::intersectFace(
                mesh, mesh.faces[0], vertexData);

        check(segment.has_value(),
              "Test 2: plane should cross the triangle.");

        std::vector<Vec3> newVertices;
        std::size_t nextNewVertexIndex = mesh.vertices.size();

        std::map<std::array<std::size_t, 2>, std::size_t>
            intersectionVertexCache;

        const auto split =
            plane_cut_internal::splitFace(
                mesh,
                mesh.faces[0],
                vertexData,
                *segment,
                newVertices,
                nextNewVertexIndex,
                intersectionVertexCache);

        check(newVertices.size() == 2,
              "Test 2: expected two new vertices.");

        check(pointsAreClose(
                  newVertices[0],
                  Vec3{1.0, 0.0, 0.0},
                  1e-9),
              "Test 2: new vertex 3 should be (1, 0, 0).");

        check(pointsAreClose(
                  newVertices[1],
                  Vec3{1.0, 1.0, 0.0},
                  1e-9),
              "Test 2: new vertex 4 should be (1, 1, 0).");

        check(split.belowFaces.size() == 1,
              "Test 2: expected one below face.");

        check(split.aboveFaces.size() == 2,
              "Test 2: expected two above faces.");

        check(split.belowFaces[0] == Face{3, 1, 4},
              "Test 2: below face should be (3, 1, 4).");

        check(split.aboveFaces[0] == Face{0, 3, 4},
              "Test 2: first above face should be (0, 3, 4).");

        check(split.aboveFaces[1] == Face{0, 4, 2},
              "Test 2: second above face should be (0, 4, 2).");
    }

}

int main()
{

    try
    {
        testSplitFaceOneAboveTwoBelow();
        testSplitFaceOneBelowTwoAbove();

        std::cout << "All split-face basic crossing tests passed.\n";
        return 0;
    }
    catch (const std::exception &error)
    {
        std::cerr << "Test failed: " << error.what() << '\n';
        return 1;
    }
}
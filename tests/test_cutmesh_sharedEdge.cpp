#include <array>
#include <cstddef>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

#include "TestHelpers.hpp"

#include "surface_mesh_cutter/Mesh.hpp"
#include "surface_mesh_cutter/PlaneCut.hpp"
#include "surface_mesh_cutter/Vec3.hpp"

namespace
{

    // Rectangle made from two triangles:
    //
    // Vertex 0: (0, 0, 0)
    // Vertex 1: (2, 0, 0)
    // Vertex 2: (2, 2, 0)
    // Vertex 3: (0, 2, 0)
    //
    // Triangle 1: vertices 0, 1, 2
    // Triangle 2: vertices 0, 2, 3
    //
    // The two triangles share diagonal edge 0-2.
    Mesh makeTwoTriangleRectangle()
    {
        Mesh mesh;

        mesh.vertices = {
            {0.0, 0.0, 0.0}, // Vertex 0
            {2.0, 0.0, 0.0}, // Vertex 1
            {2.0, 2.0, 0.0}, // Vertex 2
            {0.0, 2.0, 0.0}  // Vertex 3
        };

        mesh.faces.push_back(Face{0, 1, 2});
        mesh.faces.push_back(Face{0, 2, 3});

        return mesh;
    }

    void testCutMeshSharedEdge()
    {
        const Mesh mesh = makeTwoTriangleRectangle();

        // Plane x = 1 cuts the rectangle vertically.
        const Plane plane{
            Vec3{1.0, 0.0, 0.0},
            Vec3{1.0, 0.0, 0.0}};

        const PlaneCutResult result =
            cutMeshWithPlane(mesh, plane, 1e-9);

        // The plane crosses both triangles, so a cut must happen.
        check(result.cutPerformed,
              "Plane x = 1 should cut the two-triangle rectangle.");

        // Triangle 1 produces 2 above faces and 1 below face.
        // Triangle 2 produces 1 above face and 2 below faces.
        check(result.above.faces.size() == 3,
              "Above mesh should contain 3 triangles.");

        check(result.below.faces.size() == 3,
              "Below mesh should contain 3 triangles.");

        check(result.above.vertices.size() == 5,
              "Above mesh should contain 5 vertices.");

        check(result.below.vertices.size() == 5,
              "Below mesh should contain 5 vertices.");

        // Edge 0-1 crosses x = 1 at (1, 0, 0).
        // This becomes new vertex 4.
        bool foundBottomCrossing = false;

        for (const Vec3 &vertex : result.above.vertices)
        {
            if (pointsAreClose(
                    vertex,
                    Vec3{1.0, 0.0, 0.0},
                    1e-9))
            {
                foundBottomCrossing = true;
            }
        }

        check(foundBottomCrossing,
              "New vertex 4 should exist at (1, 0, 0).");

        // Shared diagonal edge 0-2 crosses x = 1 at (1, 1, 0).
        // This becomes new vertex 5.
        //
        // It is shared by both input triangles, so exactly one
        // vertex at this coordinate should exist in each output mesh.
        std::size_t countAtSharedDiagonal = 0;

        for (const Vec3 &vertex : result.above.vertices)
        {
            if (pointsAreClose(
                    vertex,
                    Vec3{1.0, 1.0, 0.0},
                    1e-9))
            {
                ++countAtSharedDiagonal;
            }
        }

        check(countAtSharedDiagonal == 1,
              "New vertex 5 should appear exactly once at (1, 1, 0) "
              "in the above mesh.");

        std::size_t belowCountAtSharedDiagonal = 0;

        for (const Vec3 &vertex : result.below.vertices)
        {
            if (pointsAreClose(
                    vertex,
                    Vec3{1.0, 1.0, 0.0},
                    1e-9))
            {
                ++belowCountAtSharedDiagonal;
            }
        }

        check(belowCountAtSharedDiagonal == 1,
              "New vertex 5 should appear exactly once at (1, 1, 0) "
              "in the below mesh.");

        // Edge 2-3 crosses x = 1 at (1, 2, 0).
        // This becomes new vertex 6.
        bool foundTopCrossing = false;

        for (const Vec3 &vertex : result.above.vertices)
        {
            if (pointsAreClose(
                    vertex,
                    Vec3{1.0, 2.0, 0.0},
                    1e-9))
            {
                foundTopCrossing = true;
            }
        }

        check(foundTopCrossing,
              "New vertex 6 should exist at (1, 2, 0).");

        check(result.above.faces[0] == Face{0, 1, 2},
              "Above face 0 should be (0, 1, 2).");

        check(result.above.faces[1] == Face{0, 2, 3},
              "Above face 1 should be (0, 2, 3).");

        check(result.above.faces[2] == Face{3, 2, 4},
              "Above face 2 should be (3, 2, 4).");

        check(result.below.faces[0] == Face{0, 1, 2},
              "Below face 0 should be (0, 1, 2).");

        check(result.below.faces[1] == Face{0, 2, 3},
              "Below face 1 should be (0, 2, 3).");

        check(result.below.faces[2] == Face{0, 3, 4},
              "Below face 2 should be (0, 3, 4).");
    }
} // namespace

int main()
{
    try
    {
        testCutMeshSharedEdge();
        std::cout << "testCutMeshSharedEdge passed.\n";

        return 0;
    }
    catch (const std::exception &error)
    {
        std::cerr << "Test failed: " << error.what() << '\n';
        return 1;
    }
}
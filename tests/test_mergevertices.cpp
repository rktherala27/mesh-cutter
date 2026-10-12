#include <cassert>
#include <iostream>

#include "surface_mesh_cutter/MergeVertices.hpp"

int main()
{
    // ---------- Test 1: Exact duplicate ----------
    {
        Mesh mesh;
        mesh.vertices = {{0, 0, 0}, {1, 0, 0}, {0, 0, 0}};
        mesh.faces = {{0, 1, 2}};

        std::size_t removed = mergeVertices(mesh, 0.001);

        assert(removed == 1);
        assert(mesh.vertices.size() == 2);
        assert((mesh.faces[0] == Face{0, 1, 0})); // face remapped
        std::cout << "Test 1 (exact duplicate) passed\n";
    }

    // ---------- Test 2: Near duplicate ----------
    {
        Mesh mesh;
        mesh.vertices = {{0, 0, 0}, {1, 0, 0}, {0.0005, 0, 0}};
        mesh.faces = {{0, 1, 2}};

        std::size_t removed = mergeVertices(mesh, 0.001);

        assert(removed == 1);
        assert(mesh.vertices.size() == 2);
        std::cout << "Test 2 (near duplicate) passed\n";
    }

    // ---------- Test 3: Distinct (no merge) ----------
    {
        Mesh mesh;
        mesh.vertices = {{0, 0, 0}, {1, 0, 0}, {0, 1, 0}};
        mesh.faces = {{0, 1, 2}};

        std::size_t removed = mergeVertices(mesh, 0.001);

        assert(removed == 0);
        assert(mesh.vertices.size() == 3);
        std::cout << "Test 3 (distinct) passed\n";
    }

    // ---------- Test 4: Mixed ----------
    {
        Mesh mesh;
        mesh.vertices = {{0, 0, 0}, {1, 0, 0}, {0, 0, 0}, {0, 1, 0}};
        mesh.faces = {{0, 1, 2}};

        std::size_t removed = mergeVertices(mesh, 0.001);

        assert(removed == 1);
        assert(mesh.vertices.size() == 3);
        std::cout << "Test 4 (mixed) passed\n";
    }

    // ---------- Test 5: Face remapping ----------
    {
        Mesh mesh;
        mesh.vertices = {{0, 0, 0}, {1, 0, 0}, {0, 1, 0}, {0, 0, 0}};
        mesh.faces = {{0, 1, 2}, {3, 1, 2}};

        std::size_t removed = mergeVertices(mesh, 0.001);

        assert(removed == 1);
        assert(mesh.vertices.size() == 3);
        assert((mesh.faces[1] == Face{0, 1, 2})); // face1 remapped
        std::cout << "Test 5 (face remap) passed\n";
    }

    std::cout << "ALL MERGE VERTICES TESTS PASSED\n";
    return 0;
}
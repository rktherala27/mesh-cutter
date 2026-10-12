#include <cassert>
#include <iostream>
#include <utility>

#include "surface_mesh_cutter/MeshTopology.hpp"

// ---------- Helper: build a tetrahedron (closed, consistently oriented) ----------
static Mesh makeTetrahedron()
{
    Mesh mesh;
    mesh.vertices = {
        {0.0, 0.0, 0.0}, // v0
        {1.0, 0.0, 0.0}, // v1
        {0.0, 1.0, 0.0}, // v2
        {0.0, 0.0, 1.0}, // v3
    };
    mesh.faces = {
        {0, 2, 1},
        {0, 1, 3},
        {1, 2, 3},
        {0, 3, 2},
    };
    return mesh;
}

// ---------- Test 1: buildEdgeFaceMap ----------
static void testBuildEdgeFaceMap()
{
    Mesh mesh = makeTetrahedron();
    auto map = buildEdgeFaceMap(mesh);

    // Tetrahedron has 6 unique edges, each shared by exactly 2 faces
    assert(map.size() == 6);

    for (const auto &entry : map)
    {
        assert(entry.second.size() == 2);
    }

    std::cout << "testBuildEdgeFaceMap passed\n";
}

// ---------- Test 2: checkClosed (valid tetrahedron) ----------
static void testCheckClosedValid()
{
    Mesh mesh = makeTetrahedron();
    auto report = checkClosed(mesh);

    assert(report.isClosed);

    std::cout << "testCheckClosedValid passed\n";
}

// ---------- Test 3: checkClosed (mesh with a hole) ----------
static void testCheckClosedHole()
{
    Mesh mesh = makeTetrahedron();
    mesh.faces.pop_back(); // remove one face - 3 boundary edges

    auto report = checkClosed(mesh);

    assert(!report.isClosed);

    std::cout << "testCheckClosedHole passed\n";
}

// ---------- Test 4: checkMeshConsistentlyOriented (valid tetrahedron) ----------
static void testOrientationValid()
{
    Mesh mesh = makeTetrahedron();
    auto report = checkMeshConsistentlyOriented(mesh);

    assert(report.isConsistentlyOriented);

    std::cout << "testOrientationValid passed\n";
}

// ---------- Test 5: checkMeshConsistentlyOriented (one flipped face) ----------
static void testOrientationFlipped()
{
    Mesh mesh = makeTetrahedron();

    // Flip one face: swap two vertices - reverses winding
    std::swap(mesh.faces[1][1], mesh.faces[1][2]);

    auto report = checkMeshConsistentlyOriented(mesh);

    assert(!report.isConsistentlyOriented);

    std::cout << "testOrientationFlipped passed\n";
}

// ---------- Test 6: makeMeshConsistent fixes a flipped face ----------
static void testMakeConsistent()
{
    Mesh mesh = makeTetrahedron();

    // Flip one face
    std::swap(mesh.faces[1][1], mesh.faces[1][2]);

    auto map = buildEdgeFaceMap(mesh);

    // Verify it's broken before the fix
    auto reportBefore = checkMeshConsistentlyOriented(mesh);
    assert(!reportBefore.isConsistentlyOriented);

    // Fix it
    makeMeshConsistent(mesh, map);

    // Verify it's fixed after
    auto reportAfter = checkMeshConsistentlyOriented(mesh);
    assert(reportAfter.isConsistentlyOriented);

    std::cout << "testMakeConsistent passed\n";
}

// ---------- Test 7: empty mesh ----------
static void testEmptyMesh()
{
    Mesh mesh;
    auto map = buildEdgeFaceMap(mesh);

    assert(map.empty());

    auto report = checkClosed(mesh);
    assert(report.isClosed);

    std::cout << "testEmptyMesh passed\n";
}

// ---------- Test 8: computeVolume ----------
static void testComputeVolume()
{
    Mesh mesh = makeTetrahedron();

    const double volume = computeVolume(mesh);

    // Tetrahedron with vertices (0,0,0), (1,0,0), (0,1,0), (0,0,1)
    // has volume 1/6.
    const double expected = 1.0 / 6.0;

    assert(std::abs(volume - expected) < 1e-12);

    std::cout << "testComputeVolume passed\n";
}

int main()
{
    testBuildEdgeFaceMap();
    testCheckClosedValid();
    testCheckClosedHole();
    testOrientationValid();
    testOrientationFlipped();
    testMakeConsistent();
    testEmptyMesh();
    testComputeVolume();

    std::cout << "All tests passed!\n";
    return 0;
}
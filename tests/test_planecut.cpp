#include <cassert>
#include <cmath>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <vector>

#include "surface_mesh_cutter/PlaneCutInternal.hpp"

// Compare floating-point values with an absolute tolerance.
static bool nearlyEqual(double actual, double expected, double tolerance = 1e-12)
{
    return std::abs(actual - expected) <= tolerance;
}

static Mesh makeVertexMesh()
{
    Mesh mesh;

    mesh.vertices = {
        {0.0, 0.0, -2.0},
        {0.0, 0.0, 0.0},
        {0.0, 0.0, 3.0}};

    // Faces are unnecessary for vertex distance/classification tests.
    return mesh;
}

static void testComputeSignedDistancesBasic()
{
    const Mesh mesh = makeVertexMesh();

    Plane plane;
    plane.point = {0.0, 0.0, 0.0};
    plane.normal = {0.0, 0.0, 1.0};

    const std::vector<double> distances =
        plane_cut_internal::computeSignedDistances(mesh, plane);

    assert(distances.size() == mesh.vertices.size());
    assert(nearlyEqual(distances[0], -2.0));
    assert(nearlyEqual(distances[1], 0.0));
    assert(nearlyEqual(distances[2], 3.0));

    std::cout << "testComputeSignedDistancesBasic passed\n";
}

static void testComputeSignedDistancesOffsetPlane()
{
    const Mesh mesh = makeVertexMesh();

    Plane plane;
    plane.point = {0.0, 0.0, 1.0};
    plane.normal = {0.0, 0.0, 1.0};

    const std::vector<double> distances =
        plane_cut_internal::computeSignedDistances(mesh, plane);

    assert(nearlyEqual(distances[0], -3.0));
    assert(nearlyEqual(distances[1], -1.0));
    assert(nearlyEqual(distances[2], 2.0));

    std::cout << "testComputeSignedDistancesOffsetPlane passed\n";
}

static void testComputeSignedDistancesNonUnitNormal()
{
    const Mesh mesh = makeVertexMesh();

    Plane plane;
    plane.point = {0.0, 0.0, 0.0};
    plane.normal = {0.0, 0.0, 5.0};

    const std::vector<double> distances =
        plane_cut_internal::computeSignedDistances(mesh, plane);

    assert(nearlyEqual(distances[0], -2.0));
    assert(nearlyEqual(distances[1], 0.0));
    assert(nearlyEqual(distances[2], 3.0));

    std::cout << "testComputeSignedDistancesNonUnitNormal passed\n";
}

static void testComputeSignedDistancesEmptyMesh()
{
    Mesh mesh;

    Plane plane;
    plane.point = {0.0, 0.0, 0.0};
    plane.normal = {0.0, 0.0, 1.0};

    const std::vector<double> distances =
        plane_cut_internal::computeSignedDistances(mesh, plane);

    assert(distances.empty());

    std::cout << "testComputeSignedDistancesEmptyMesh passed\n";
}

static void testClassifyMeshVertices()
{
    const Mesh mesh = makeVertexMesh();

    Plane plane;
    plane.point = {0.0, 0.0, 0.0};
    plane.normal = {0.0, 0.0, 1.0};

    const double tolerance = 1e-9;

    const plane_cut_internal::VertexPlaneData data =
        plane_cut_internal::classifyMeshVertices(mesh, plane, tolerance);

    assert(data.distances.size() == 3);
    assert(data.sides.size() == 3);

    assert(nearlyEqual(data.distances[0], -2.0));
    assert(nearlyEqual(data.distances[1], 0.0));
    assert(nearlyEqual(data.distances[2], 3.0));

    assert(data.sides[0] == plane_cut_internal::VertexSide::Below);
    assert(data.sides[1] == plane_cut_internal::VertexSide::OnPlane);
    assert(data.sides[2] == plane_cut_internal::VertexSide::Above);

    std::cout << "testClassifyMeshVertices passed\n";
}

static void testClassificationToleranceBoundaries()
{
    const double tolerance = 1e-3;

    Mesh mesh;
    mesh.vertices = {
        {0.0, 0.0, -2.0 * tolerance},
        {0.0, 0.0, -tolerance},
        {0.0, 0.0, 0.0},
        {0.0, 0.0, tolerance},
        {0.0, 0.0, 2.0 * tolerance}};

    Plane plane;
    plane.point = {0.0, 0.0, 0.0};
    plane.normal = {0.0, 0.0, 1.0};

    const plane_cut_internal::VertexPlaneData data =
        plane_cut_internal::classifyMeshVertices(mesh, plane, tolerance);

    assert(data.sides.size() == 5);
    assert(data.sides[0] == plane_cut_internal::VertexSide::Below);
    assert(data.sides[1] == plane_cut_internal::VertexSide::OnPlane);
    assert(data.sides[2] == plane_cut_internal::VertexSide::OnPlane);
    assert(data.sides[3] == plane_cut_internal::VertexSide::OnPlane);
    assert(data.sides[4] == plane_cut_internal::VertexSide::Above);

    std::cout << "testClassificationToleranceBoundaries passed\n";
}

static void testClassifyMeshVerticesEmptyMesh()
{
    Mesh mesh;

    Plane plane;
    plane.point = {0.0, 0.0, 0.0};
    plane.normal = {0.0, 0.0, 1.0};

    const plane_cut_internal::VertexPlaneData data =
        plane_cut_internal::classifyMeshVertices(mesh, plane, 1e-9);

    assert(data.distances.empty());
    assert(data.sides.empty());

    std::cout << "testClassifyMeshVerticesEmptyMesh passed\n";
}

static void testZeroNormalThrows()
{
    const Mesh mesh = makeVertexMesh();

    Plane plane;
    plane.point = {0.0, 0.0, 0.0};
    plane.normal = {0.0, 0.0, 0.0};

    bool threw = false;

    try
    {
        (void)plane_cut_internal::computeSignedDistances(mesh, plane);
    }
    catch (const std::invalid_argument &)
    {
        threw = true;
    }

    assert(threw);

    std::cout << "testZeroNormalThrows passed\n";
}

static void testInvalidToleranceThrows()
{
    const Mesh mesh = makeVertexMesh();

    Plane plane;
    plane.point = {0.0, 0.0, 0.0};
    plane.normal = {0.0, 0.0, 1.0};

    bool threw = false;

    try
    {
        (void)plane_cut_internal::classifyMeshVertices(mesh, plane, -1.0);
    }
    catch (const std::invalid_argument &)
    {
        threw = true;
    }

    assert(threw);

    std::cout << "testInvalidToleranceThrows passed\n";
}

static void testNonFiniteToleranceThrows()
{
    const Mesh mesh = makeVertexMesh();

    Plane plane;
    plane.point = {0.0, 0.0, 0.0};
    plane.normal = {0.0, 0.0, 1.0};

    const double invalidTolerance =
        std::numeric_limits<double>::infinity();

    bool threw = false;

    try
    {
        (void)plane_cut_internal::classifyMeshVertices(mesh, plane, invalidTolerance);
    }
    catch (const std::invalid_argument &)
    {
        threw = true;
    }

    assert(threw);

    std::cout << "testNonFiniteToleranceThrows passed\n";
}

int main()
{
    testComputeSignedDistancesBasic();
    testComputeSignedDistancesOffsetPlane();
    testComputeSignedDistancesNonUnitNormal();
    testComputeSignedDistancesEmptyMesh();

    testClassifyMeshVertices();
    testClassificationToleranceBoundaries();
    testClassifyMeshVerticesEmptyMesh();

    testZeroNormalThrows();
    testInvalidToleranceThrows();
    testNonFiniteToleranceThrows();

    std::cout << "All PlaneCut classification tests passed!\n";
    return 0;
}
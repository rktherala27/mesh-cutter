#pragma once

#include <string>
#include <vector>

#include "Mesh.hpp"
#include "Vec3.hpp"
#include "VectorMath.hpp"

// The plane is defined by a point and a normal.
// The normal's direction defines which output is "above".
struct Plane
{
    Vec3 point;
    Vec3 normal;
};

// Result of cutting one mesh into the two half-meshes.
struct PlaneCutResult
{
    Mesh above;
    Mesh below;

    // Intersection loops, each stored as ordered points.
    // A cut may produce more than one loop.
    std::vector<std::vector<Vec3>> intersectionLoops;

    // False if the plane misses or only touches the mesh,
    // or if cutting fails.
    bool cutPerformed = false;

    // Human-readable diagnostics or limitations encountered.
    std::vector<std::string> messages;
};

// Cut a closed triangle mesh with a plane.
//
// `tolerance` is used for classifying points near the plane and
// identifying coincident intersection points.
//
// Positive signed distance is the above side; negative is below.
PlaneCutResult cutMeshWithPlane(
    const Mesh &mesh,
    const Plane &plane,
    double tolerance);
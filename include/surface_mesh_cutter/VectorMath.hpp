#pragma once

#include <cmath>
#include "Vec3.hpp"

// --- Arithematic inline functions ------

inline Vec3 operator+(const Vec3 &a, const Vec3 &b)
{
    return {a.x + b.x, a.y + b.y, a.z + b.z};
}

inline Vec3 operator-(const Vec3 &a, const Vec3 &b)
{
    return {a.x - b.x, a.y - b.y, a.z - b.z};
}

inline Vec3 operator*(const Vec3 &a, const double s)
{
    return {a.x * s, a.y * s, a.z * s};
}

inline Vec3 operator*(const double s, const Vec3 &a)
{
    return a * s;
}

// --------- Vector Operations ----------

inline double dot(const Vec3 &a, const Vec3 &b)
{

    return a.x * b.x + a.y * b.y + a.z * b.z;
}

// Cross product. Returns the zero vector if a and b are parallel
// (or anti-parallel) or if either is the zero vector.
//
// Edge case: for nearly-parallel inputs, floating-point rounding may
// return a tiny nonzero vector instead of exact zero. Callers must
// compare lengthSq(result) against a tolerance, never against zero.

inline Vec3 cross(const Vec3 &a, const Vec3 &b)
{
    return {
        a.y * b.z - a.z * b.y,
        a.z * b.x - a.x * b.z,
        a.x * b.y - a.y * b.x};
}

// --- Length and normalization ---

// Squared length. Preferred for tolerance comparisons: avoids sqrt.
inline double lengthSq(const Vec3 &a)
{
    return dot(a, a);
}

inline double length(const Vec3 &a)
{
    return std::sqrt(lengthSq(a));
}

// Returns a unit-length vector in the same direction as 'a'.
//
// Edge case: if 'a' is the zero vector (or near-zero), division by
// zero would occur. Returns the zero vector safely instead.
// Callers working with geometric normals should check lengthSq(a)
// against a tolerance BEFORE calling this, since a zero-length normal
// is usually a sign of a degenerate triangle.
inline Vec3 normalized(const Vec3 &a)
{
    double len = length(a);
    return (len > 0.0) ? a * (1.0 / len) : Vec3{};
}
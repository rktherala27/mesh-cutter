# Surface Mesh Cutter

A C++17 command-line tool for reading an ASCII STL triangle mesh,
preparing it for cutting, and cutting it with a plane.

The program welds nearby duplicate STL vertices, removes faces that become
degenerate after welding, checks mesh closure and face orientation, computes
signed volume, and cuts the mesh with a user-defined plane.

## Features

| Feature | Status |
|---|---|
| Read ASCII STL files | Implemented |
| Write ASCII STL files | Implemented |
| Weld duplicate vertices using a user-supplied tolerance | Implemented |
| Remove topologically degenerate faces after welding | Implemented |
| Check whether the mesh is closed | Implemented |
| Check and repair face winding | Implemented |
| Compute signed volume | Implemented |
| Classify vertices relative to a cutting plane | Implemented |
| Cut a closed mesh with a plane and cap the resulting boundary | Implemented |
| Automatic merge-tolerance selection | Not implemented |
| Hole detection and hole filling | Not implemented |
| Cutting planes passing exactly/within tolerance, through mesh vertices | Not fully implemented |
| Multiple separate intersection loops | Not implemented |
| Unit tests | Implemented |

## Requirements

- CMake 3.16 or newer.
- A C++17 compiler.
- No external geometry library.

## Build and test

```bash
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
```

The executable is normally created at:

```text
build/cutter
```

## Command-line usage

```text
cutter input.stl px py pz nx ny nz [tol]
```

- `input.stl`: path to an ASCII STL file.
- `px py pz`: a point on the cutting plane.
- `nx ny nz`: plane normal. Its direction defines the positive side.
- `tol`: optional vertex-merging tolerance. If omitted, the program uses `1e-9`.

Example:

```bash
./build/cutter sphere.stl 55 55 25 0 0 0.5 0.0002
```

The plane point is `(55, 55, 25)`, the plane direction is `(0, 0, 0.5)`,
and the vertex-merging tolerance is `0.0002`.

## Processing pipeline

1. Read the ASCII STL file.
2. Merge duplicate vertices within the supplied tolerance.
3. Remove faces that become degenerate after merging.
4. Check that every edge is shared by exactly two faces.
5. Check and repair face winding.
6. Compute signed volume.
7. Classify vertices relative to the plane.
8. Cut the mesh and cap the cut boundary.
9. Write the resulting mesh as an ASCII STL file.

## Mesh repair

STL files store each triangle independently. The same geometric corner can
appear several times with slightly different coordinates because of export
precision or rounding.

The program merges nearby vertices using the supplied tolerance. After
merging, some very small triangles may have two identical vertex indices.
These faces no longer represent valid triangles and are removed.

Removing a degenerate face does not necessarily create a hole. In this
project, the removed faces were zero-area slivers whose edges had already
become shared by neighboring valid faces after welding. Therefore, the
remaining mesh remained closed.

The STL writer uses 17 significant digits so that vertex coordinates survive
an export-and-reload cycle without becoming near-duplicates again.

## Sphere example

The included `sphere.stl` contains facets with two nearly identical corner
vertices. These near-duplicates arise from limited STL coordinate precision.

For example, one facet contains both of these vertices:

```text
vertex  0.550000E+02  0.550000E+02  0.500000E+01
vertex  0.549999E+02  0.549999E+02  0.500000E+01
```

The two vertices differ by approximately `0.0001` in each coordinate. They are
two corners of the same facet, not merely duplicated vertices shared between
neighboring facets.

A merge tolerance of `0.0002` or larger is required to weld these two corners
into one vertex. Once welded, the affected facet has two identical vertex
indices and becomes degenerate. The program then removes that degenerate facet.

With a smaller tolerance, the near-duplicate vertices are not merged. The
sphere may therefore remain topologically invalid and cannot be cut
successfully.

Use this command for `sphere.stl`:

```bash
./build/cutter sphere.stl 55 55 25 0 0 0.5 0.0002
```

After welding, the sphere has 112 unique vertices and 220 valid faces. The
program removes the degenerate facets and can cut the mesh with the supplied
plane.

## Current limitations

- Only ASCII STL input is supported.
- Removing degenerate faces can create holes in other meshes. Hole detection
  and hole filling are not yet implemented.
- The merge tolerance must currently be chosen manually. Automatic tolerance
  selection is not implemented.
- For `sphere.stl`, a tolerance of at least `0.0002` is required because some
  facets contain two nearly identical corner vertices.
- A plane passing exactly through a mesh vertex is not fully supported by the
  cutting path.
- Multiple separate intersection loops are not yet handled correctly.
- Self-intersecting meshes are not detected or repaired.
- Consistent face winding does not prove that all normals point outward.

## Repository layout

```text
include/surface_mesh_cutter/  Public and internal headers
src/                          Library sources and CLI entry point
tests/                        Automated tests
tests/data/                   Test input files
examples/                     Example STL files
```

## How the mesh is stored

An STL file describes each triangle using three corner points. When triangles
share a corner, STL may write that point repeatedly.

This project stores each unique point once in a vertex list. Each triangle
then refers to its three corners by their positions in that list.

For example, the face `(0, 1, 2)` means “make a triangle from points 0, 1,
and 2.” When another triangle uses point `0`, it refers to that same vertex.
This allows the program to recognize how triangles connect.
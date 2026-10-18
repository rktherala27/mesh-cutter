# Surface Mesh Cutter

A C++17 command-line tool that reads an **ASCII STL** triangle mesh and
classifies its vertices relative to a plane. The current implementation
prepares and checks the mesh, computes signed volume and vertex-to-plane
distances, and reports whether vertex classifications occur on both sides
of the plane.

The internal plane-cutting pipeline now supports splitting an individual
triangle that is definitely crossed by a plane. For such a triangle, it
creates intersection vertices on the crossed edges and produces separate
above-side and below-side triangle sets.

> **Note:** full-mesh cutting through `cutMeshWithPlane()` is not yet
> integrated into the command-line program. The program still reports a
> vertex-sign judgement rather than producing two completed output meshes.
> Intersection-loop construction, capping, and writing two output STL files
> are not implemented yet.

## Requirements

- CMake 3.16 or newer.
- A C++17 compiler (GCC/Clang on Linux; MSVC or MinGW-w64 on Windows).
- No external geometry library is used.

## CMake options

| Option | Default | Description |
|---|---:|---|
| `BUILD_CUTTER_APP` | `ON` | Builds the `cutter` command-line application. |

To build only the library and tests, without the command-line application,
configure CMake with:

```bash
cmake -S . -B build -DBUILD_CUTTER_APP=OFF
```

To explicitly build the command-line application, use:

```bash
cmake -S . -B build -DBUILD_CUTTER_APP=ON
```

Because `BUILD_CUTTER_APP` is `ON` by default, the normal build commands also
build the `cutter` executable.

## Build and test on Linux

Run these commands from the repository root:

```bash
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
```

This builds the `surface_mesh_cutter` library, the `cutter` command-line
application, and the automated tests.

To build only the library and tests, use:

```bash
cmake -S . -B build -DBUILD_CUTTER_APP=OFF
cmake --build build
ctest --test-dir build --output-on-failure
```

## Build and test on Windows (untested)

I have not tested these instructions on Windows. They are provided as a guide
and should be checked on a Windows system with a C++ compiler installed.

For the MSVC compiler, follow Microsoft's instructions to install C++ support.
Select the **Desktop development with C++** workload, which includes the MSVC
compiler and Windows build tools:

[Install C and C++ support in Visual Studio](https://learn.microsoft.com/en-us/cpp/build/vscpp-step-1-installation?view=msvc-170)

After installation, open **Developer PowerShell for Visual Studio** or
**Developer Command Prompt for Visual Studio**, navigate to the project
directory, and run:

```powershell
cmake -S . -B build
cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure
```

This builds the `surface_mesh_cutter` library, the `cutter` command-line
application, and the automated tests.

To build only the library and tests, use:

```powershell
cmake -S . -B build -DBUILD_CUTTER_APP=OFF
cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure
```

These commands assume the Visual Studio CMake generator. The build and test
commands have not been verified on Windows.

With Visual Studio, the executable is commonly located at
`build\Release\cutter.exe`. With a single-configuration generator such as
Ninja or MinGW Makefiles, it is commonly at `build\cutter.exe`; the exact
path depends on the generator.

CTest runs the tests registered by the project. Passing tests only verifies
the behavior covered by those tests; it does not mean the mesh-cutting
functionality is complete.

## Command-line usage

```text
cutter input.stl px py pz nx ny nz [tol]
```

- `input.stl`: path to an ASCII STL file.
- `px py pz`: a point on the plane.
- `nx ny nz`: non-zero plane normal; its direction defines the positive/“above” side.
- `tol`: optional non-negative tolerance. If omitted, the current program uses `1e-9`.

The tolerance is used both for merging duplicate STL vertices and for
classifying signed distances near zero. Choose it in the coordinate scale
used by the STL. STL itself does not encode physical units, so the input
coordinates and tolerance must use a consistent unit convention.

## Current processing pipeline

1. Read the ASCII STL into a triangle-soup mesh.
2. Merge duplicate vertices within the supplied tolerance, producing an indexed mesh.
3. Check whether every mesh edge is shared by exactly two faces. Stop if the mesh is not closed.
4. Check face winding. If inconsistent, attempt to make it consistent, then check again.
5. Compute and print signed volume.
6. Compute each vertex’s signed distance to the plane and classify it as `Above`, `Below`, or `OnPlane`.
7. Print the classification counts and a judgement based on those counts.

Positive signed distance means the vertex is on the normal-facing side;
negative means the opposite side. A distance whose magnitude is at most `tol`
is classified `OnPlane`.

## Implemented plane-cutting internals

The following internal functions are implemented:

- `classifyMeshVertices()` classifies every vertex as `Above`, `Below`, or
  `OnPlane`.
- `intersectFace()` finds where a plane crosses the edges of one triangle.
- `splitFace()` splits one crossed triangle into above-side and below-side
  faces.
- `getOrCreateIntersectionVertex()` creates or reuses an intersection vertex
  for a crossed mesh edge.
- `triangulatePolygon()` converts an above- or below-side polygon into
  triangles using a fan triangulation.

For a triangle with one vertex on one side and two vertices on the other, the
plane crosses two edges. The implementation creates two intersection vertices,
produces one triangle on the single-vertex side, and produces two triangles on
the two-vertex side.

The intersection-vertex cache is designed so that adjacent triangles crossing
the same mesh edge can reuse the same intersection vertex.

## Current implementation status

| Area | Status |
|---|---|
| ASCII STL reading | Implemented |
| Duplicate-vertex merging | Implemented |
| Closed-mesh topology check | Implemented |
| Face-winding check and repair | Implemented |
| Signed-volume calculation | Implemented |
| Vertex-to-plane classification | Implemented |
| Single-triangle plane intersection | Implemented |
| Single-triangle splitting | Implemented for definite crossings |
| Intersection-vertex creation and reuse | Implemented for a single triangle |
| Full-mesh cutting | Not yet integrated |
| Intersection-loop extraction | Not implemented |
| Capping cut halves | Not implemented |
| Writing two output STL files | Not implemented |
| Point-inside-mesh testing | Not implemented |

### Tested `splitFace()` cases

The current tests cover two basic definite-crossing cases:

1. One vertex above and two vertices below.
2. One vertex below and two vertices above.

These tests check:

- The number of created intersection vertices.
- The coordinates of the intersection vertices.
- The vertex indices assigned to the intersection vertices.
- The number of above-side and below-side faces.
- The exact triangle topology returned by `splitFace()`.

## Important limitations

- Input is ASCII STL only; binary STL is not supported by the current reader.
- Consistent face winding does not by itself prove that all faces point outward (may be inward).
- Winding repair cannot fill holes or fix non-manifold topology.
- Full-mesh cutting is not yet connected to the command-line entry point.
- `cutMeshWithPlane()` is declared in the public API but is not yet fully implemented.
- The current `splitFace()` implementation supports ordinary definite edge crossings.
- Vertices exactly on the plane are not yet supported by the full-mesh cutting path.
- Edges lying entirely in the plane are not supported yet.
- Faces lying entirely in the plane are not supported yet.
- Intersection-loop construction is not implemented yet.
- Capping the two cut halves is not implemented yet.
- Writing two output STL files is not implemented yet.
- Point-inside-mesh testing is not implemented yet.

## Next integration step

The next step is to update `src/main.cpp` so that it calls
`cutMeshWithPlane()` after the mesh has been read, merged, and validated.

The command-line program should then:

1. Read and prepare the STL mesh.
2. Classify its vertices against the supplied plane.
3. Call `cutMeshWithPlane()`.
4. Report whether a cut was performed.
5. Report the number of vertices and faces in the above and below meshes.
6. Later, write the two resulting meshes as separate STL files.

Until that integration is complete, the program continues to report only the
vertex-sign classification and does not produce cut geometry.

## Repository layout

```text
include/surface_mesh_cutter/  Public and internal headers
src/                          Library sources and CLI entry point
tests/                        Automated tests
tests/data/                   Test input files
examples/                     User-facing example STL files
```

## How the mesh is stored

An STL file describes each triangle using three corner points. When triangles
share a corner, STL may write that point repeatedly. This project instead
stores each point once in a **vertex list**. Each triangle then refers to its
three corners by their positions in that list.

For example, if the vertex list contains points numbered `0`, `1`, and `2`,
then the face `(0, 1, 2)` means “make a triangle from points 0, 1, and 2.”
When another triangle uses point `0`, it refers to that same point. This helps
the program recognize how the triangles connect.
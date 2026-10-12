# Surface Mesh Cutter

A C++17 command-line tool that reads an **ASCII STL** triangle mesh and classifies its vertices relative to a plane. The current implementation prepares and checks the mesh, computes signed volume and vertex-to-plane distances, and reports whether vertex classifications occur on both sides of the plane.

> **Note:** this version does not yet split triangles, construct intersection loops, cap the two halves, write two output STLs, or test whether an arbitrary point is inside a mesh. It reports a vertex-sign judgement, not completed cut geometry.

## Requirements

- CMake 3.16 or newer.
- A C++17 compiler (GCC/Clang on Linux; MSVC or MinGW-w64 on Windows).
- No external geometry library is used.

## Build and test on Linux

Run these commands from the repository root:

```bash
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
```

## Build and test on Windows (untested)

I have not tested these instructions on Windows. They are provided as a guide and should be checked on a Windows system with a C++ compiler installed.

For the MSVC compiler, follow Microsoft's instructions to install C++ support. Select the **Desktop development with C++** workload, which includes the MSVC compiler and Windows build tools:

[Install C and C++ support in Visual Studio](https://learn.microsoft.com/en-us/cpp/build/vscpp-step-0-installation?view=msvc-170)

After installation, open **Developer PowerShell for Visual Studio** or **Developer Command Prompt for Visual Studio**, navigate to the project directory, and run:

```powershell
cmake -S . -B build
cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure
```

These commands assume the Visual Studio CMake generator. The build and test commands have not been verified on Windows.

With Visual Studio, the executable is commonly located at `build\Release\cutter.exe`. With a single-configuration generator such as Ninja or MinGW Makefiles, it is commonly at `build\cutter.exe`; the exact path depends on the generator.

CTest runs the tests registered by the project. Passing tests only verifies the behavior covered by those tests; it does not mean the mesh-cutting functionality is complete.

## Command-line usage

```text
cutter input.stl px py pz nx ny nz [tol]
```

- `input.stl`: path to an ASCII STL file.
- `px py pz`: a point on the plane.
- `nx ny nz`: non-zero plane normal; its direction defines the positive/“above” side.
- `tol`: optional non-negative tolerance. If omitted, the current program uses `1e-9`.

The tolerance is used both for merging duplicate STL vertices and for classifying signed distances near zero. Choose it in the coordinate scale used by the STL. STL itself does not encode physical units, so the input coordinates and tolerance must use a consistent unit convention. [web:946]

### Run the example

Assuming the example cube spans `z = 0` to `z = 1`, a plane through its middle is `z = 0.5`:

Linux:

```bash
./build/cutter examples/cube.stl 0 0 0.5 0 0 1 1e-9
```

Windows, Visual Studio generator:

```powershell
.\build\Release\cutter.exe examples\cube.stl 0 0 0.5 0 0 1 1e-9
```

Windows, single-configuration generator:

```powershell
.\build\cutter.exe examples\cube.stl 0 0 0.5 0 0 1 1e-9
```

Adjust the plane point to match the actual example mesh coordinates. The command assumes `examples/cube.stl` is present in the repository.

## Current processing pipeline

1. Read the ASCII STL into a triangle-soup mesh.
2. Merge duplicate vertices within the supplied tolerance, producing an indexed mesh.
3. Check whether every mesh edge is shared by exactly two faces. Stop if the mesh is not closed.
4. Check face winding. If inconsistent, attempt to make it consistent, then check again.
5. Compute and print signed volume.
6. Compute each vertex’s signed distance to the plane and classify it as `Above`, `Below`, or `OnPlane`.
7. Print the classification counts and a judgement based on those counts.

Positive signed distance means the vertex is on the normal-facing side; negative means the opposite side. A distance whose magnitude is at most `tol` is classified `OnPlane`.

## Important limitations

- Input is ASCII STL only; binary STL is not supported by the current reader.
- Consistent face winding does not by itself prove that all faces point outward (may be inward).
- Winding repair cannot fill holes or fix non-manifold topology.
- Having both `Above` and `Below` vertices is evidence of a crossing for the current classification stage; the program does not calculate the actual intersection points yet.
- Vertices lying on the plane are classified using the tolerance band, but this version does not construct a robust cut for those degenerate cases.
- Point-inside-mesh testing is not implemented yet.

## Repository layout

```text
include/surface_mesh_cutter/  Public and internal headers
src/                          Library sources and CLI entry point
tests/                        Automated tests
tests/data/                   Test input files
examples/                     User-facing example STL files
```

## How the mesh is stored

An STL file describes each triangle using three corner points. When triangles share a corner, STL may write that point repeatedly. This project instead stores each point once in a **vertex list**. Each triangle then refers to its three corners by their positions in that list.

For example, if the vertex list contains points numbered `0`, `1`, and `2`, then the face `(0, 1, 2)` means “make a triangle from points 0, 1, and 2.” When another triangle uses point `0`, it refers to that same point. This helps the program recognize how the triangles connect.

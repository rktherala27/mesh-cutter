#include <cassert>
#include <iostream>
#include <string>

#include "surface_mesh_cutter/StlIO.hpp"
#include "TestHelpers.hpp"

int main()
{
    const std::string dataDir = std::string(TEST_DATA_DIR) + "/";

    // ---------- Test 1: Read tetrahedron (4 facets) ----------
    {
        Mesh m = readAsciiStl(dataDir + "tetrahedron_ascii.stl");
        assert(m.faces.size() == 4);
        assert(m.vertices.size() == 12);
        std::cout << "tetrahedron read OK: "
                  << m.vertices.size() << " verts, "
                  << m.faces.size() << " faces\n";
    }

    // ---------- Test 2: Read flat (3 facets) ----------
    {
        Mesh m = readAsciiStl(dataDir + "flat.stl");
        assert(m.faces.size() == 3);
        assert(m.vertices.size() == 9);
        std::cout << "flat read OK: "
                  << m.vertices.size() << " verts, "
                  << m.faces.size() << " faces\n";
    }

    // ---------- Test 3: Read cube (12 facets) ----------
    {
        Mesh m = readAsciiStl(dataDir + "cube.stl");
        assert(m.faces.size() == 12);
        assert(m.vertices.size() == 36);
        std::cout << "cube read OK: "
                  << m.vertices.size() << " verts, "
                  << m.faces.size() << " faces\n";
    }

    // ---------- Test 4: Read sphere (228 facets) ----------
    {
        Mesh m = readAsciiStl(dataDir + "sphere.stl");
        assert(m.faces.size() == 228);
        assert(m.vertices.size() == 684);
        std::cout << "sphere read OK: "
                  << m.vertices.size() << " verts, "
                  << m.faces.size() << " faces\n";
    }

    // ---------- Test 5: Round-trip (read → write → re-read) ----------
    {
        Mesh in = readAsciiStl(dataDir + "cube.stl");
        writeAsciiStl(in, dataDir + "cube_roundtrip.stl");
        Mesh out = readAsciiStl(dataDir + "cube_roundtrip.stl");

        assert(out.vertices.size() == in.vertices.size());
        assert(out.faces.size() == in.faces.size());
        std::cout << "round-trip OK: counts match\n";
    }

    // ---------- Test 6: Error handling (malformed file should throw) ----------
    {
        bool threw = expectThrows([&]
                                  { readAsciiStl(dataDir + "fourVertices_ascii.stl"); });
        assert(threw);
        std::cout << "error-handling OK: malformed file threw as expected\n";
    }

    std::cout << "ALL STLIO TESTS PASSED\n";
    return 0;
}
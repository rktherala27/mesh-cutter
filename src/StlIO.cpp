#include "surface_mesh_cutter/StlIO.hpp"
#include "surface_mesh_cutter/VectorMath.hpp"

#include <fstream>
#include <sstream>
#include <stdexcept>

Mesh readAsciiStl(const std::string &path)
{
    std::ifstream in(path);
    if (!in)
        throw std::runtime_error("cannot open file at: " + path);

    Mesh mesh;
    std::vector<Vec3> currentVertex;
    std::string line;

    while (std::getline(in, line))
    {
        std::istringstream ls(line);
        std::string word;
        ls >> word;

        if (word == "vertex")
        {
            Vec3 v;
            if (!(ls >> v.x >> v.y >> v.z))
                throw std::runtime_error("bad vertex line: " + line);

            currentVertex.push_back(v);
        }
        else if (word == "endloop")
        {
            if (currentVertex.size() != 3)
                throw std::runtime_error("facet does not have exactly 3 vertices");

            std::size_t base = mesh.vertices.size();
            for (const Vec3 &v : currentVertex)
                mesh.vertices.push_back(v);
            mesh.faces.push_back({base, base + 1, base + 2});

            currentVertex.clear();
        }
    }
    // "solid", "facet", "outer loop", "endfacet",
    // "endsolid" and normals are ignored:
    // we recompute normals ourselves from vertex order.
    if (mesh.faces.empty())
        throw std::runtime_error("no facets found in the ASCII STL");

    return mesh;
}

void writeAsciiStl(const Mesh &mesh, const std::string &path)
{
    std::ofstream out(path);
    if (!out)
        throw std::runtime_error("cannot open file for writing: " + path);

    out << "solid mesh\n";

    for (const Face &f : mesh.faces)
    {
        const Vec3 &a = mesh.vertices[f[0]];
        const Vec3 &b = mesh.vertices[f[1]];
        const Vec3 &c = mesh.vertices[f[2]];

        // Recompute normal from vertex order (right-hand rule)
        Vec3 n = normalized(cross(b - a, c - a));

        out << "  facet normal "
            << n.x << " " << n.y << " " << n.z << "\n";

        out << "    outer loop\n";

        out << "      vertex "
            << a.x << " " << a.y << " " << a.z << "\n";

        out << "      vertex "
            << b.x << " " << b.y << " " << b.z << "\n";

        out << "      vertex "
            << c.x << " " << c.y << " " << c.z << "\n";

        out << "    endloop\n";
        out << "  endfacet\n";
    }

    out << "endsolid mesh\n";
}

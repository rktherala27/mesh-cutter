#include <cmath>
#include <exception>
#include <iomanip>
#include <iostream>
#include <stdexcept>
#include <string>

#include "surface_mesh_cutter/StlIO.hpp"
#include "surface_mesh_cutter/VectorMath.hpp"
#include "surface_mesh_cutter/MergeVertices.hpp"
#include "surface_mesh_cutter/MeshTopology.hpp"
#include "surface_mesh_cutter/PlaneCutInternal.hpp"
#include "surface_mesh_cutter/PlaneCut.hpp"

int main(int argc, char *argv[])
{
    // Required arguments:
    // input.stl px py pz nx ny nz
    //
    // Optional argument:
    // tol
    if (argc != 8 && argc != 9)
    {
        std::cerr
            << "Usage: cutter input.stl px py pz nx ny nz [tol]\n";
        return 1;
    }

    try
    {
        // Step 1: Read the STL file.
        std::cout << "Step 1: Reading STL file...\n";
        Mesh mesh = readAsciiStl(argv[1]);

        std::cout << "Finished reading STL.\n";
        std::cout << "  Triangle vertices: "
                  << mesh.vertices.size() << '\n';
        std::cout << "  Faces: "
                  << mesh.faces.size() << "\n\n";

        // Convert the numeric command-line arguments.
        const double px = std::stod(argv[2]);
        const double py = std::stod(argv[3]);
        const double pz = std::stod(argv[4]);

        const double nx = std::stod(argv[5]);
        const double ny = std::stod(argv[6]);
        const double nz = std::stod(argv[7]);

        const double tolerance =
            (argc == 9) ? std::stod(argv[8]) : 1e-9;

        if (!std::isfinite(px) || !std::isfinite(py) ||
            !std::isfinite(pz) || !std::isfinite(nx) ||
            !std::isfinite(ny) || !std::isfinite(nz) ||
            !std::isfinite(tolerance))
        {
            throw std::invalid_argument(
                "Plane values and tolerance must be finite numbers.");
        }

        if (tolerance < 0.0)
        {
            throw std::invalid_argument(
                "Tolerance must be non-negative.");
        }

        const Plane plane{
            {px, py, pz},
            {nx, ny, nz}};

        if (length(plane.normal) == 0.0)
        {
            throw std::invalid_argument(
                "Plane normal must be non-zero.");
        }

        // Step 2: Merge duplicate vertices.
        std::cout << "Step 2: Merging duplicate vertices...\n";
        const std::size_t removedVertices =
            mergeVertices(mesh, tolerance);

        std::cout << "Finished merging vertices.\n";
        std::cout << "  Vertices merged: " << removedVertices << '\n';
        std::cout << "  Unique vertices: " << mesh.vertices.size() << '\n';
        std::cout << "  Faces: " << mesh.faces.size() << "\n\n";

        // Step 3: Check that the mesh is closed.
        std::cout << "Step 3: Checking whether the mesh is closed...\n";
        const TopologyReport closedReport = checkClosed(mesh);

        if (!closedReport.isClosed)
        {
            std::cout << "Mesh is not closed. Stopping here.\n";

            for (const std::string &problem : closedReport.problems)
            {
                std::cout << "  Problem: " << problem << '\n';
            }

            return 2;
        }

        std::cout << "Mesh is closed.\n\n";

        // Step 4: Check face orientation.
        std::cout << "Step 4: Checking face orientation...\n";
        TopologyReport orientationReport =
            checkMeshConsistentlyOriented(mesh);

        if (orientationReport.isConsistentlyOriented)
        {
            std::cout << "Face orientation is already consistent.\n\n";
        }
        else
        {
            std::cout << "Face orientation is inconsistent.\n\n";

            // Step 5: Attempt to make the orientation consistent.
            std::cout << "Step 5: Making face orientation consistent...\n";
            const auto edgeFaceMap = buildEdgeFaceMap(mesh);
            makeMeshConsistent(mesh, edgeFaceMap);

            std::cout << "Finished orientation repair.\n\n";

            // Step 6: Check orientation again.
            std::cout << "Step 6: Checking orientation again...\n";
            orientationReport = checkMeshConsistentlyOriented(mesh);

            if (!orientationReport.isConsistentlyOriented)
            {
                std::cout << "Orientation is still inconsistent. Stopping.\n";

                for (const std::string &problem : orientationReport.problems)
                {
                    std::cout << "  Problem: " << problem << '\n';
                }

                return 3;
            }

            std::cout << "Face orientation is now consistent.\n\n";
        }

        std::cout
            << "Note: consistent orientation does not by itself prove "
               "that the faces point outward.\n\n";

        // Step 7: Compute volume.
        std::cout << "Step 7: Computing signed volume...\n";
        const double volume = computeVolume(mesh);
        std::cout << "Finished volume calculation.\n";
        std::cout << "  Signed volume: " << volume << "\n\n";

        // Step 8: Compute vertex distances and classify them.
        std::cout << "Step 8: Computing signed distances and classifying vertices...\n";
        const plane_cut_internal::VertexPlaneData data =
            plane_cut_internal::classifyMeshVertices(
                mesh, plane, tolerance);

        std::cout << "Finished vertex classification.\n";
        std::cout << "  Positive distance = above the plane.\n";
        std::cout << "  Negative distance = below the plane.\n";
        std::cout << "  Within tolerance = on the plane.\n\n";

        std::cout << "\nStep 9: Plane-position judgement\n";

        std::size_t aboveCount = 0;
        std::size_t belowCount = 0;
        std::size_t onPlaneCount = 0;

        for (plane_cut_internal::VertexSide side : data.sides)
        {
            if (side == plane_cut_internal::VertexSide::Above)
            {
                ++aboveCount;
            }
            else if (side == plane_cut_internal::VertexSide::Below)
            {
                ++belowCount;
            }
            else
            {
                ++onPlaneCount;
            }
        }

        std::cout << "  Vertices above:   " << aboveCount << '\n';
        std::cout << "  Vertices below:   " << belowCount << '\n';
        std::cout << "  Vertices on-plane: " << onPlaneCount << '\n';

        if (aboveCount > 0 && belowCount > 0)
        {
            std::cout
                << "  Some vertices are above and some are below.\n"
                << "  There is a definite intersection with the mesh surface.\n";
        }
        else if (aboveCount > 0 && belowCount == 0)
        {
            std::cout << "  No vertices are below the plane.\n";

            if (onPlaneCount > 0)
            {
                std::cout << "  Some vertices are within the plane tolerance.\n";
            }
            else
            {
                std::cout << "  No intersection is indicated by vertex signs.\n";
            }
        }
        else if (belowCount > 0 && aboveCount == 0)
        {
            std::cout << "  No vertices are above the plane.\n";

            if (onPlaneCount > 0)
            {
                std::cout << "  Some vertices are within the plane tolerance.\n";
            }
            else
            {
                std::cout << "  No intersection is indicated by vertex signs.\n";
            }
        }
        else if (onPlaneCount > 0)
        {
            std::cout << "  All vertices are within the plane tolerance.\n";
        }
        else
        {
            std::cout << "  The mesh has no vertices.\n";
        }

        std::cout
            << "\nThis version stops at vertex classification. "
               "It does not split faces or write two output meshes.\n";

        return 0;
    }
    catch (const std::exception &error)
    {
        std::cerr << "Error: " << error.what() << '\n';
        return 1;
    }
}
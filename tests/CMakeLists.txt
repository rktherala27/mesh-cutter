# Test: StlIO (read + write + round-up)
add_executable(test_stlio test_stlio.cpp)
target_link_libraries(test_stlio PRIVATE surface_mesh_cutter)
add_test(NAME test_stlio COMMAND test_stlio)

target_compile_definitions(test_stlio PRIVATE
    TEST_DATA_DIR="${CMAKE_CURRENT_SOURCE_DIR}/data"
)

# Test: MergeVertices (duplicate merging)
add_executable(test_mergevertices test_mergevertices.cpp)
target_link_libraries(test_mergevertices PRIVATE surface_mesh_cutter)
add_test(NAME test_mergevertices COMMAND test_mergevertices)

# Test: MeshTopology (edge map, closed check, orientation)
add_executable(test_meshtopology test_meshtopology.cpp)
target_link_libraries(test_meshtopology PRIVATE surface_mesh_cutter)
add_test(NAME test_meshtopology COMMAND test_meshtopology)

# Test: PlaneCut vertex distances and classification
add_executable(test_planecut test_planecut.cpp)
target_link_libraries(test_planecut PRIVATE surface_mesh_cutter)
add_test(NAME test_planecut COMMAND test_planecut)

# Test: Face intersection (above, below, and crossing)
add_executable(test_faceintersection test_faceintersection.cpp)
target_link_libraries(test_faceintersection PRIVATE surface_mesh_cutter)
add_test(NAME test_faceintersection COMMAND test_faceintersection)
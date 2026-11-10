add_library(vector STATIC vector.c)
target_include_directories(vector PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(vector PRIVATE project_warnings)

add_executable(test_vector tests/test_vector.c)
target_link_libraries(test_vector PRIVATE vector project_warnings)

if(BUILD_TESTING)
    add_test(NAME test_vector COMMAND test_vector)
endif()

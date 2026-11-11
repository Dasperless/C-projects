add_library(bst STATIC bst.c)
target_include_directories(bst PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(bst PRIVATE project_warnings)

add_executable(test_bst tests/test_bst.c)
target_link_libraries(test_bst PRIVATE bst project_warnings)

if(BUILD_TESTING)
    add_test(NAME test_bst COMMAND test_bst)
endif()

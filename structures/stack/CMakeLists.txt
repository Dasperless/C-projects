add_library(stack STATIC stack.c)
target_include_directories(stack PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(stack PRIVATE project_warnings)

add_executable(test_stack tests/test_stack.c)
target_link_libraries(test_stack PRIVATE stack project_warnings)

if(BUILD_TESTING)
    add_test(NAME test_stack COMMAND test_stack)
endif()

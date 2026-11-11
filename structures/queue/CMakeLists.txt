add_library(queue STATIC queue.c)
target_include_directories(queue PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(queue PRIVATE project_warnings)

add_executable(test_queue tests/test_queue.c)
target_link_libraries(test_queue PRIVATE queue project_warnings)

if(BUILD_TESTING)
    add_test(NAME test_queue COMMAND test_queue)
endif()

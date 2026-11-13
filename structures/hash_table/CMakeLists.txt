add_library(hash_table STATIC hash_table.c)
target_include_directories(hash_table PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(hash_table PRIVATE project_warnings)

add_executable(test_hash_table tests/test_hash_table.c)
target_link_libraries(test_hash_table PRIVATE hash_table project_warnings)

if(BUILD_TESTING)
    add_test(NAME test_hash_table COMMAND test_hash_table)
endif()

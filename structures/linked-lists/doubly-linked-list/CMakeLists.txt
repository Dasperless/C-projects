add_library(doubly_linked_list STATIC dll.c)
target_include_directories(doubly_linked_list PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(doubly_linked_list PRIVATE project_warnings)

add_executable(test_dll tests/test_dll.c)
target_link_libraries(test_dll PRIVATE doubly_linked_list project_warnings)

if(BUILD_TESTING)
    add_test(NAME test_dll COMMAND test_dll)
endif()

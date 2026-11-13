add_library(singly_linked_list STATIC linked_list.c list_container.c)
target_include_directories(singly_linked_list PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(singly_linked_list PRIVATE project_warnings)

add_executable(singly_linked_list_demo demo.c)
target_link_libraries(singly_linked_list_demo PRIVATE singly_linked_list project_warnings)

add_executable(test_singly_linked_list tests/test_linked_list.c)
target_link_libraries(test_singly_linked_list PRIVATE singly_linked_list project_warnings)

if(BUILD_TESTING)
    add_test(NAME test_singly_linked_list COMMAND test_singly_linked_list)
    add_test(NAME singly_linked_list_demo COMMAND singly_linked_list_demo)
endif()

add_executable(singly_linked_list linked_list.c)
target_include_directories(singly_linked_list PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(singly_linked_list PRIVATE project_warnings)

if(BUILD_TESTING)
    add_test(NAME singly_linked_list_test COMMAND singly_linked_list)
endif()

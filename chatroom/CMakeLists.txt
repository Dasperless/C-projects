add_library(chatroom_protocol STATIC protocol.c)
target_include_directories(chatroom_protocol PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/includes)
target_link_libraries(chatroom_protocol PRIVATE project_warnings)

add_executable(chatroom_server server.c)
target_link_libraries(chatroom_server PRIVATE chatroom_protocol project_warnings Threads::Threads)

add_executable(chatroom_client client.c)
target_link_libraries(chatroom_client PRIVATE chatroom_protocol project_warnings Threads::Threads)

add_executable(test_protocol tests/test_protocol.c)
target_link_libraries(test_protocol PRIVATE chatroom_protocol project_warnings Threads::Threads)

if(BUILD_TESTING)
    add_test(NAME test_protocol COMMAND test_protocol)
endif()

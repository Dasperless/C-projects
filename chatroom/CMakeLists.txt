add_executable(chatroom_server server.c)
target_include_directories(chatroom_server PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/includes)
target_link_libraries(chatroom_server PRIVATE project_warnings Threads::Threads)

add_executable(chatroom_client client.c)
target_include_directories(chatroom_client PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/includes)
target_link_libraries(chatroom_client PRIVATE project_warnings Threads::Threads)

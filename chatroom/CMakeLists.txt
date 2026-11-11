add_library(chatroom_logger STATIC logger.c)
target_include_directories(chatroom_logger PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/includes)
target_link_libraries(chatroom_logger PRIVATE project_warnings Threads::Threads)

add_library(chatroom_fileshare STATIC fileshare.c)
target_include_directories(chatroom_fileshare PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/includes)
target_link_libraries(chatroom_fileshare PRIVATE project_warnings)

add_library(chatroom_protocol STATIC protocol.c)
target_include_directories(chatroom_protocol PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/includes)
target_link_libraries(chatroom_protocol PRIVATE project_warnings)

add_executable(chatroom_server server.c)
target_link_libraries(chatroom_server PRIVATE chatroom_protocol chatroom_logger project_warnings Threads::Threads)

add_executable(chatroom_client client.c)
target_link_libraries(chatroom_client PRIVATE chatroom_protocol project_warnings Threads::Threads)

add_executable(test_logger tests/test_logger.c)
target_link_libraries(test_logger PRIVATE chatroom_logger project_warnings Threads::Threads)

add_executable(test_fileshare tests/test_fileshare.c)
target_link_libraries(test_fileshare PRIVATE chatroom_fileshare project_warnings)

add_executable(test_protocol tests/test_protocol.c)
target_link_libraries(test_protocol PRIVATE chatroom_protocol project_warnings Threads::Threads)

add_executable(test_chat_features tests/test_chat_features.c)
target_link_libraries(test_chat_features PRIVATE chatroom_protocol project_warnings Threads::Threads)

add_executable(test_concurrency tests/test_concurrency.c)
target_link_libraries(test_concurrency PRIVATE chatroom_protocol project_warnings Threads::Threads)

if(BUILD_TESTING)
    add_test(NAME test_logger COMMAND test_logger)
    add_test(NAME test_fileshare COMMAND test_fileshare)
    add_test(NAME test_protocol COMMAND test_protocol)
    add_test(NAME test_chat_features COMMAND test_chat_features $<TARGET_FILE:chatroom_server>)
    add_test(NAME test_concurrency COMMAND test_concurrency $<TARGET_FILE:chatroom_server>)
endif()

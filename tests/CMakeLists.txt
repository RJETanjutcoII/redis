add_executable(test_resp     test_resp.cpp)
add_executable(test_store    test_store.cpp)
add_executable(test_commands test_commands.cpp)
add_executable(test_aof      test_aof.cpp)

foreach(t test_resp test_store test_commands test_aof)
    target_link_libraries(${t} PRIVATE mini_redis_lib GTest::gtest_main)
    add_test(NAME ${t} COMMAND ${t})
endforeach()

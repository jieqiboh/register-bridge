include(FetchContent)
FetchContent_Declare(
    googletest
    URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip
)
FetchContent_MakeAvailable(googletest)

add_executable(register_bridge_tests
    test_protocol.cpp
    test_register_bank.cpp
    test_thread_pool.cpp
)
target_link_libraries(register_bridge_tests
    PRIVATE register_bridge_lib GTest::gtest_main
)
include(GoogleTest)
gtest_discover_tests(register_bridge_tests)

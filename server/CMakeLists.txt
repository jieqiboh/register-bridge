add_library(register_bridge_lib
    register_bank.cpp
    thread_pool.cpp
    server.cpp
)
target_include_directories(register_bridge_lib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

add_executable(register_bridge main.cpp)
target_link_libraries(register_bridge PRIVATE register_bridge_lib)

add_library(fpga_bridge_lib
    register_bank.cpp
    thread_pool.cpp
    server.cpp
)
target_include_directories(fpga_bridge_lib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

add_executable(fpga_bridge main.cpp)
target_link_libraries(fpga_bridge PRIVATE fpga_bridge_lib)

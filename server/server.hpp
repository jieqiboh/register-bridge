#pragma once
#include "register_bank.hpp"
#include "thread_pool.hpp"

class Server {
public:
    explicit Server(int port, size_t num_threads = 4);

    void run();
    void stop();

private:
    void handle_client(int client_fd);

    int          port_;
    int          server_fd_ = -1;
    bool         running_   = false;
    ThreadPool   pool_;
    RegisterBank bank_;
};

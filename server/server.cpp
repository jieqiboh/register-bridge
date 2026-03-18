#include "server.hpp"
#include "protocol.hpp"
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#include <array>
#include <iostream>

Server::Server(int port, size_t num_threads)
    : port_(port), pool_(num_threads) {}

void Server::run() {
    server_fd_ = socket(AF_INET, SOCK_STREAM, 0);

    int opt = 1;
    setsockopt(server_fd_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    sockaddr_in addr{};
    addr.sin_family      = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port        = htons(static_cast<uint16_t>(port_));

    bind(server_fd_, reinterpret_cast<sockaddr*>(&addr), sizeof(addr));
    listen(server_fd_, 10);

    running_ = true;
    std::cout << "Listening on port " << port_ << "\n";

    while (running_) {
        int client_fd = accept(server_fd_, nullptr, nullptr);
        if (client_fd < 0) break;
        pool_.enqueue([this, client_fd]() { handle_client(client_fd); });
    }
}

void Server::stop() {
    running_ = false;
    if (server_fd_ >= 0) close(server_fd_);
}

void Server::handle_client(int client_fd) {
    std::array<uint8_t, PACKET_SIZE> buf{};

    while (true) {
        ssize_t n = recv(client_fd, buf.data(), PACKET_SIZE, MSG_WAITALL);
        if (n != static_cast<ssize_t>(PACKET_SIZE)) break;

        Packet req  = unpack(buf);
        Packet resp{};
        resp.address = req.address;

        if (req.opcode == Opcode::READ) {
            resp.opcode = Opcode::ACK;
            resp.value  = bank_.read(req.address);
        } else if (req.opcode == Opcode::WRITE) {
            bank_.write(req.address, req.value);
            resp.opcode = Opcode::ACK;
            resp.value  = 0;
        } else {
            resp.opcode = Opcode::ERR;
            resp.value  = 0;
        }

        auto out = pack(resp);
        send(client_fd, out.data(), PACKET_SIZE, 0);
    }

    close(client_fd);
}

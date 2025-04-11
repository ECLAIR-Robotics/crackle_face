#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include <chrono>
#include <thread>
#include <fcntl.h>
#include <errno.h>
#include <vector>
#include <algorithm>

int main() {
    int sock = 0;
    struct sockaddr_in serv_addr;
    const char* server_ip = "127.0.0.1";
    const int port = 8080;

    std::vector<std::string> valid_emotions = {
        "happy", "sad", "angry", "bored", "not_impressed",
        "evil_idea", "flirty", "aww", "wtf"
    };

    std::cout << "Emotion Tester Client\n";
    std::cout << "Connecting to " << server_ip << ":" << port << "\n";
    std::cout << "Available commands: ";
    for (const auto& e : valid_emotions) std::cout << e << " ";
    std::cout << "exit\n\n";

    while (true) {
        // Create socket
        if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
            std::cerr << "Socket creation error: " << strerror(errno) << std::endl;
            std::this_thread::sleep_for(std::chrono::seconds(1));
            continue;
        }

        serv_addr.sin_family = AF_INET;
        serv_addr.sin_port = htons(port);

        if (inet_pton(AF_INET, server_ip, &serv_addr.sin_addr) <= 0) {
            std::cerr << "Invalid address: " << strerror(errno) << std::endl;
            close(sock);
            return -1;
        }

        std::cout << "Attempting to connect...\n";

        int flags = fcntl(sock, F_GETFL, 0);
        fcntl(sock, F_SETFL, flags | O_NONBLOCK);

        connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr));

        fd_set set;
        FD_ZERO(&set);
        FD_SET(sock, &set);

        struct timeval timeout;
        timeout.tv_sec = 2;
        timeout.tv_usec = 0;

        int connect_result = select(sock + 1, NULL, &set, NULL, &timeout);

        if (connect_result <= 0) {
            std::cerr << "Connection failed: " << (connect_result == 0 ? "Timeout" : strerror(errno)) << std::endl;
            close(sock);
            std::this_thread::sleep_for(std::chrono::seconds(1));
            continue;
        }

        fcntl(sock, F_SETFL, flags);

        std::string command;
        std::cout << "\nEnter emotion (";
        for (const auto& e : valid_emotions) std::cout << e << "/";
        std::cout << "exit): ";
        std::getline(std::cin, command);

        std::transform(command.begin(), command.end(), command.begin(), ::tolower);

        if (command == "exit") {
            close(sock);
            break;
        }

        if (std::find(valid_emotions.begin(), valid_emotions.end(), command) == valid_emotions.end()) {
            std::cout << "Invalid command. Try again.\n";
            close(sock);
            continue;
        }
        int bytes_sent = send(sock, command.c_str(), command.length(), 0);
        if (bytes_sent < 0) {
            std::cerr << "Send failed: " << strerror(errno) << std::endl;
        } else {
            std::cout << "Sent: " << command << std::endl;
        }

        close(sock);
    }

    std::cout << "Exiting tester program.\n";
    return 0;
}

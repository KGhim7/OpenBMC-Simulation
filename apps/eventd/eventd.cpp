#include <iostream>
#include <string>
#include <cstring>
#include <cstdio>

#include <thread>
#include <chrono>
#include <atomic>
#include <csignal>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

constexpr int PORT = 5555;
constexpr int BUFFER_SIZE = 1024;

static std::atomic<bool> g_stop{false};

static void on_signal(int) {
    g_stop = true;
}

// send entire string (handles partial sends)
static bool send_all(int fd, const std::string& s) {
    const char* p = s.c_str();
    size_t left = s.size();

    while (left > 0) {
        ssize_t n = ::send(fd, p, left, 0);
        if (n <= 0) return false;
        p += n;
        left -= static_cast<size_t>(n);
    }
    return true;
}

static void handle_line(const std::string& line) {
    const std::string prefix = "SENSORS|";
    if (line.rfind(prefix, 0) != 0) {
        return;
    }

    static bool temp_high = false;
    static bool vcore_high = false;
    static bool fan_low = false;

    std::string payload = line.substr(prefix.size());

    constexpr double TEMP_HIGH = 80.0;
    constexpr double VCORE_HIGH = 4.8;
    constexpr double FAN_LOW = 600.0;

    size_t start = 0;
    while (start < payload.size()) {
        size_t end = payload.find(';', start);
        if (end == std::string::npos) end = payload.size();

        std::string field = payload.substr(start, end - start);
        size_t eq = field.find('=');

        if (eq != std::string::npos) {
            std::string key = field.substr(0, eq);
            std::string valStr = field.substr(eq + 1);

            char* parse_end = nullptr;
            double val = std::strtod(valStr.c_str(), &parse_end);

            if (parse_end != valStr.c_str()) {
                if (key == "temp_cpu") {
                    bool now = (val > TEMP_HIGH);
                    if (now && !temp_high) {
                        std::cout << "[eventd] ALERT START " << key << "=" << val
                                  << " (>" << TEMP_HIGH << ")\n";
                    } else if (!now && temp_high) {
                        std::cout << "[eventd] ALERT CLEARED " << key << "=" << val
                                  << " (<= " << TEMP_HIGH << ")\n";
                    }
                    temp_high = now;

                } else if (key == "vcore_cpu") {
                    bool now = (val > VCORE_HIGH);
                    if (now && !vcore_high) {
                        std::cout << "[eventd] WARN  START " << key << "=" << val
                                  << " (>" << VCORE_HIGH << ")\n";
                    } else if (!now && vcore_high) {
                        std::cout << "[eventd] WARN  CLEARED " << key << "=" << val
                                  << " (<= " << VCORE_HIGH << ")\n";
                    }
                    vcore_high = now;

                } else if (key == "fan_rpm") {
                    bool now = (val < FAN_LOW);
                    if (now && !fan_low) {
                        std::cout << "[eventd] WARN  START " << key << "=" << val
                                  << " (<" << FAN_LOW << ")\n";
                    } else if (!now && fan_low) {
                        std::cout << "[eventd] WARN  CLEARED " << key << "=" << val
                                  << " (>= " << FAN_LOW << ")\n";
                    }
                    fan_low = now;
                }
            } else {
                std::cout << "[eventd] bad number: " << field << "\n";
            }
        }

        start = end + 1;
    }
}

int connect_to_busd() {
    int sockfd = ::socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("socket failed");
        return -1;
    }

    sockaddr_in server_addr;
    std::memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);

    if (::inet_pton(AF_INET, "127.0.0.1", &server_addr.sin_addr) <= 0) {
        perror("inet_pton failed");
        ::close(sockfd);
        return -1;
    }

    if (::connect(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("connect failed");
        ::close(sockfd);
        return -1;
    }

    return sockfd;
}

static void read_and_print_loop(int fd) {
    char buffer[BUFFER_SIZE];
    std::string leftover;

    while (!g_stop) {
        ssize_t n = ::read(fd, buffer, BUFFER_SIZE);

        if (n > 0) {
            leftover.append(buffer, static_cast<size_t>(n));

            size_t pos;
            while ((pos = leftover.find('\n')) != std::string::npos) {
                std::string line = leftover.substr(0, pos);
                leftover.erase(0, pos + 1);
                handle_line(line);
            }
        } else if (n == 0) {
            std::cout << "[eventd] busd disconnected\n";
            break;
        } else {
            perror("read failed");
            break;
        }
    }
}

int run_eventd() {
    std::signal(SIGINT, on_signal);
    std::signal(SIGTERM, on_signal);

    int bus_fd = connect_to_busd();
    if (bus_fd < 0) {
        std::cerr << "[eventd] Failed to connect to busd\n";
        return 1;
    }

    if (!send_all(bus_fd, "HELLO|EVENTD\n")) {
        std::cerr << "[eventd] Failed to register with busd\n";
        ::close(bus_fd);
        return 1;
    }

    std::cout << "[eventd] Connected to busd on port " << PORT << "\n";

    read_and_print_loop(bus_fd);

    if (g_stop) {
        std::cout << "[eventd] EVENTD disconnected\n";
    }

    ::close(bus_fd);
    return 0;
}
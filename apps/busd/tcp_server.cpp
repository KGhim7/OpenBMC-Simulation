#include <algorithm>
#include <arpa/inet.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <map>
#include <mutex>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <thread>
#include <unordered_map>
#include <unistd.h>
#include <vector>

constexpr int PORT = 5555;
constexpr int BACKLOG = 8;
constexpr int BUFFER_SIZE = 1024;

std::map<std::string, double> latest_value;
std::mutex latest_mutex;

std::unordered_map<int, std::string> client_names;
std::mutex client_names_mutex;

std::vector<int> clients;
std::mutex clients_mutex;

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

static void handle_line(const std::string& line, int client_fd) {
    if (line.rfind("HELLO|", 0) == 0) {
        std::string name = line.substr(6);

        {
            std::lock_guard<std::mutex> lock(client_names_mutex);
            client_names[client_fd] = name;
        }

        std::cout << "[busd] registered client: " << name << "\n";
        return;
    }

    if (line == "GET|ALL") {
        std::string out = "SENSORS|";

        {
            std::lock_guard<std::mutex> lock(latest_mutex);
            for (const auto& kv : latest_value) {
                out += kv.first;
                out += "=";
                out += std::to_string(kv.second);
                out += ";";
            }
        }

        out += "\n";
        send_all(client_fd, out);
        return;
    }

    const std::string prefix = "SENSORS|";
    if (line.rfind(prefix, 0) != 0) return;

    std::string payload = line.substr(prefix.size());

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
                std::lock_guard<std::mutex> lock(latest_mutex);
                latest_value[key] = val;
            }
        }

        start = end + 1;
    }

    std::string msg = line + "\n";

    std::vector<int> snapshot;
    {
        std::lock_guard<std::mutex> lock(clients_mutex);
        snapshot = clients;
    }

    for (int fd : snapshot) {
        if (fd == client_fd) continue;

        if (!send_all(fd, msg)) {
            std::cerr << "[busd] failed to send to client " << fd << "\n";
        }
    }
}

static void read_and_print_loop(int client_fd) {
    char buffer[BUFFER_SIZE];
    std::string leftover;

    while (true) {
        ssize_t n = read(client_fd, buffer, BUFFER_SIZE);

        if (n > 0) {
            leftover.append(buffer, static_cast<size_t>(n));

            size_t pos;
            while ((pos = leftover.find('\n')) != std::string::npos) {
                std::string line = leftover.substr(0, pos);
                leftover.erase(0, pos + 1);
                handle_line(line, client_fd);
            }
        } else if (n == 0) {
            break;
        } else {
            perror("read failed");
            break;
        }
    }

    {
        std::lock_guard<std::mutex> lock(clients_mutex);
        clients.erase(std::remove(clients.begin(), clients.end(), client_fd), clients.end());
    }

    close(client_fd);

    std::string name = "UNKNOWN";
    {
        std::lock_guard<std::mutex> lock(client_names_mutex);
        auto it = client_names.find(client_fd);
        if (it != client_names.end()) {
            name = it->second;
            client_names.erase(it);
        }
    }

    std::cout << "[busd] " << name << " disconnected\n";
}

int run_server() {
    std::cout.setf(std::ios::unitbuf);

    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        perror("socket failed");
        return EXIT_FAILURE;
    }

    sockaddr_in address;
    std::memset(&address, 0, sizeof(address));
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    if (bind(server_fd, (struct sockaddr*)&address, sizeof(address)) < 0) {
        perror("bind failed");
        close(server_fd);
        return EXIT_FAILURE;
    }

    if (listen(server_fd, BACKLOG) < 0) {
        perror("listen failed");
        close(server_fd);
        return EXIT_FAILURE;
    }

    std::cout << "busd listening on port " << PORT << "\n";

    while (true) {
        int client_fd = accept(server_fd, nullptr, nullptr);
        if (client_fd < 0) {
            perror("accept failed");
            continue;
        }

        std::cout << "Connection accepted\n";

        {
            std::lock_guard<std::mutex> lock(clients_mutex);
            clients.push_back(client_fd);
        }

        std::thread([client_fd]() { read_and_print_loop(client_fd); }).detach();
    }

    return 0;
}
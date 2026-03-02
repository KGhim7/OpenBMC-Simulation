#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cstring>
#include <iostream>
#include <sstream>
#include <string>
#include <unordered_map>

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

static std::string read_line(int fd) {
    std::string out;
    char c;
    while (true) {
        ssize_t n = ::recv(fd, &c, 1, 0);
        if (n <= 0) break;
        if (c == '\r') continue;
        if (c == '\n') break;
        out.push_back(c);
    }
    return out;
}

static int busd_connect_once(const std::string& host, int port) {
    int fd = ::socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) return -1;

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(static_cast<uint16_t>(port));
    if (::inet_pton(AF_INET, host.c_str(), &addr.sin_addr) != 1) {
        ::close(fd);
        return -1;
    }

    if (::connect(fd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0) {
        ::close(fd);
        return -1;
    }

    if (!send_all(fd, "HELLO|REDFISHD\n")) {
        ::close(fd);
        return -1;
    }

    return fd;
}

static std::unordered_map<std::string, double> query_busd_get_all(const std::string& host, int port) {
    std::unordered_map<std::string, double> m;

    static int bus_fd = -1;

    auto ensure_connected = [&]() -> bool {
        if (bus_fd >= 0) return true;
        bus_fd = busd_connect_once(host, port);
        return bus_fd >= 0;
    };

    if (!ensure_connected()) return m;

    if (!send_all(bus_fd, "GET|ALL\n")) {
        ::close(bus_fd);
        bus_fd = -1;
        if (!ensure_connected()) return m;
        if (!send_all(bus_fd, "GET|ALL\n")) return m;
    }

    std::string line = read_line(bus_fd);
    if (line.empty()) {
        ::close(bus_fd);
        bus_fd = -1;
        if (!ensure_connected()) return m;
        if (!send_all(bus_fd, "GET|ALL\n")) return m;
        line = read_line(bus_fd);
    }

    const std::string prefix = "SENSORS|";
    if (line.rfind(prefix, 0) != 0) return m;

    std::string payload = line.substr(prefix.size());
    size_t start = 0;
    while (start < payload.size()) {
        size_t semi = payload.find(';', start);
        if (semi == std::string::npos) break;
        std::string token = payload.substr(start, semi - start);
        start = semi + 1;

        if (token.empty()) continue;
        size_t eq = token.find('=');
        if (eq == std::string::npos) continue;

        std::string key = token.substr(0, eq);
        std::string val = token.substr(eq + 1);
        try {
            m[key] = std::stod(val);
        } catch (...) {
        }
    }

    return m;
}

static void parse_http_request_line(const std::string& req, std::string& method, std::string& path) {
    std::istringstream iss(req);
    iss >> method >> path;
}

static std::string http_response_json(int code, const std::string& json_body) {
    std::string status = (code == 200) ? "OK" : (code == 404 ? "Not Found" : "Error");
    std::ostringstream oss;
    oss << "HTTP/1.1 " << code << " " << status << "\r\n";
    oss << "Content-Type: application/json\r\n";
    oss << "Content-Length: " << json_body.size() << "\r\n";
    oss << "Connection: close\r\n";
    oss << "\r\n";
    oss << json_body;
    return oss.str();
}

static std::string json_escape(const std::string& s) {
    std::string out;
    out.reserve(s.size());
    for (char c : s) {
        if (c == '\"') out += "\\\"";
        else if (c == '\\') out += "\\\\";
        else if (c == '\n') out += "\\n";
        else out += c;
    }
    return out;
}

static std::string redfish_service_root() {
    return R"({
  "@odata.id": "/redfish/v1",
  "Name": "OpenBMC-Sim Redfish Service",
  "Systems": { "@odata.id": "/redfish/v1/Systems" },
  "Chassis": { "@odata.id": "/redfish/v1/Chassis" }
})";
}

static std::string redfish_systems_collection() {
    return R"({
  "@odata.id": "/redfish/v1/Systems",
  "Members@odata.count": 1,
  "Members": [ { "@odata.id": "/redfish/v1/Systems/1" } ]
})";
}

static std::string redfish_system_1(const std::unordered_map<std::string, double>& state) {
    (void)state;
    return R"({
  "@odata.id": "/redfish/v1/Systems/1",
  "Id": "1",
  "Name": "Simulated System",
  "SystemType": "Physical",
  "Status": { "State": "Enabled", "Health": "OK" }
})";
}

static std::string redfish_chassis_collection() {
    return R"({
  "@odata.id": "/redfish/v1/Chassis",
  "Members@odata.count": 1,
  "Members": [ { "@odata.id": "/redfish/v1/Chassis/1" } ]
})";
}

static std::string redfish_chassis_1() {
    return R"({
  "@odata.id": "/redfish/v1/Chassis/1",
  "Id": "1",
  "Name": "Simulated Chassis",
  "Thermal": { "@odata.id": "/redfish/v1/Chassis/1/Thermal" }
})";
}

static std::string redfish_thermal_1(const std::unordered_map<std::string, double>& state) {
    auto get = [&](const std::string& k) -> double {
        auto it = state.find(k);
        return (it == state.end()) ? 0.0 : it->second;
    };

    double cpu  = get("temp_cpu");
    double fan  = get("fan_rpm");
    double vcore = get("vcore_cpu");  // <-- add this

    std::ostringstream oss;
    oss << "{\n"
        << "  \"@odata.id\": \"/redfish/v1/Chassis/1/Thermal\",\n"
        << "  \"Name\": \"Thermal\",\n"
        << "  \"Temperatures\": [\n"
        << "    { \"Name\": \"CPU\", \"ReadingCelsius\": " << cpu << " }\n"
        << "  ],\n"
        << "  \"Fans\": [\n"
        << "    { \"Name\": \"Fan1\", \"ReadingRPM\": " << fan << " }\n"
        << "  ],\n"
        << "  \"Voltages\": [\n"
        << "    { \"Name\": \"Vcore\", \"ReadingVolts\": " << vcore << " }\n"
        << "  ]\n"
        << "}";
    return oss.str();
}

int main() {
    const int HTTP_PORT = 8000;
    const std::string BUSD_HOST = "127.0.0.1";
    const int BUSD_PORT = 5555;

    int server_fd = ::socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        perror("redfishd socket");
        return 1;
    }

    int opt = 1;
    ::setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(static_cast<uint16_t>(HTTP_PORT));

    if (::bind(server_fd, reinterpret_cast<sockaddr*>(&address), sizeof(address)) < 0) {
        perror("redfishd bind");
        ::close(server_fd);
        return 1;
    }

    if (::listen(server_fd, 10) < 0) {
        perror("redfishd listen");
        ::close(server_fd);
        return 1;
    }

    std::cout << "redfishd listening on port " << HTTP_PORT << "\n";

    while (true) {
        sockaddr_in client_addr{};
        socklen_t client_len = sizeof(client_addr);
        int client_fd = ::accept(server_fd, reinterpret_cast<sockaddr*>(&client_addr), &client_len);
        if (client_fd < 0) {
            perror("redfishd accept");
            continue;
        }

        char buf[4096];
        ssize_t n = ::recv(client_fd, buf, sizeof(buf) - 1, 0);
        if (n <= 0) {
            ::close(client_fd);
            continue;
        }
        buf[n] = '\0';
        std::string req(buf);

        size_t eol = req.find("\r\n");
        std::string first_line = (eol == std::string::npos) ? req : req.substr(0, eol);

        std::string method, path;
        parse_http_request_line(first_line, method, path);

        auto state = query_busd_get_all(BUSD_HOST, BUSD_PORT);

        std::string body;
        int code = 200;

        if (method != "GET") {
            code = 404;
            body = R"({"error":"Only GET supported"})";
        } else if (path == "/redfish/v1" || path == "/redfish/v1/") {
            body = redfish_service_root();
        } else if (path == "/redfish/v1/Systems") {
            body = redfish_systems_collection();
        } else if (path == "/redfish/v1/Systems/1") {
            body = redfish_system_1(state);
        } else if (path == "/redfish/v1/Chassis") {
            body = redfish_chassis_collection();
        } else if (path == "/redfish/v1/Chassis/1") {
            body = redfish_chassis_1();
        } else if (path == "/redfish/v1/Chassis/1/Thermal") {
            body = redfish_thermal_1(state);
        } else {
            code = 404;
            body = R"({"error":"Not found"})";
        }

        std::string resp = http_response_json(code, body);
        send_all(client_fd, resp);
        ::close(client_fd);
    }
}
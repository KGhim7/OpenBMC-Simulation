#include "sensor.hpp"
#include <string>
#include <chrono>
#include <thread>
#include <iostream>
#include <iomanip>
#include <unistd.h>


using namespace std;
int connect_to_busd();
bool send_line(int sockfd, const std::string& line);


int main(){
    int bus_fd = connect_to_busd();
    if (bus_fd < 0) {
        std::cerr << "Failed to connect to busd\n";
        return 1;
    }

    send_line(bus_fd, "HELLO|SENSORD\n");

    std::srand(std::time(nullptr));

    Sensor sensor1("temp_cpu", 25.0, -10.0, 100.0);
    Sensor sensor2("fan_rpm", 1500.0, 0.0, 3000.0);
    Sensor sensor3("vcore_cpu", 3.3, 0.0, 5.0);    

    cout << "Sensor Daemon Started" << endl;
    cout << std::fixed << std::setprecision(2);

    while(true){
        sensor1.update();
        sensor2.update();
        sensor3.update();

        cout << "---- Sensor Readings ----" << endl;
        cout << sensor1.getName() << "=" << sensor1.getValue() << " C" << endl;
        cout << sensor2.getName() << "=" << sensor2.getValue() << " RPM" << endl;
        cout << sensor3.getName() << "=" << sensor3.getValue() << " V" << endl;
        cout << "-------------------------" << endl;

        std::ostringstream oss;
        oss << std::fixed << std::setprecision(2);
        oss << "SENSORS|"
            << "temp_cpu="  << sensor1.getValue() << ";"
            << "fan_rpm="   << sensor2.getValue() << ";"
            << "vcore_cpu=" << sensor3.getValue()
            << "\n";

        std::string msg = oss.str();

        if (!send_line(bus_fd, msg)) {
            std::cerr << "send failed\n";
            break;
        }

        this_thread::sleep_for(chrono::seconds(1));
    }
    close(bus_fd);
    return 0;
}
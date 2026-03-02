#include "sensor.hpp"
#include <cstdlib>   
#include <ctime>
#include <sstream>

Sensor::Sensor() : sensor_name(""), sensor_value(0),min_value(0), max_value(0) {}

Sensor::Sensor(const std::string& name, double value, double minValue, double maxValue)
    : sensor_name(name), sensor_value(value), min_value(minValue), max_value(maxValue) {}

void Sensor::update() {
    double delta = 0.0;

    if (sensor_name == "temp_cpu") {
        if (!in_spike) {
            if ((std::rand() % 35) == 0) {
                in_spike = true;
                spike_ticks_left = 6;
            }
        }

        if (in_spike) {
            delta = 8.0;
            spike_ticks_left--;
            if (spike_ticks_left <= 0) in_spike = false;
        } else {
            delta = (std::rand() % 31 - 15) / 100.0;
            delta += (25.0 - sensor_value) * 0.06;
        }
    } else if (sensor_name == "fan_rpm") {
        if (!in_spike) {
            if ((std::rand() % 60) == 0) {
                in_spike = true;
                spike_ticks_left = 5;
            }
        }

        if (in_spike) {
            delta = -350.0;
            spike_ticks_left--;
            if (spike_ticks_left <= 0) in_spike = false;
        } else {
            delta = (std::rand() % 101 - 50);
            delta += (1500.0 - sensor_value) * 0.02;
        }
    } else if (sensor_name == "vcore_cpu") {
        if (!in_spike) {
            if ((std::rand() % 80) == 0) {
                in_spike = true;
                spike_ticks_left = 4;
            }
        }

        if (in_spike) {
            delta = 0.6;
            spike_ticks_left--;
            if (spike_ticks_left <= 0) in_spike = false;
        } else {
            delta = (std::rand() % 21 - 10) / 100.0;
            delta += (3.3 - sensor_value) * 0.04;
        }
    } else {
        delta = (std::rand() % 101 - 50) / 100.0;
    }

    sensor_value += delta;

    if (sensor_value < min_value) sensor_value = min_value;
    if (sensor_value > max_value) sensor_value = max_value;
}
void Sensor::setValue(double value) {
    sensor_value = value;
}

std::string Sensor::getName() const {
    return sensor_name;
}

double Sensor::getValue() const {
    return sensor_value;
}
#pragma once
#include <string>

class Sensor {
public:
    Sensor();
    Sensor(const std::string& name, double value,
           double minValue, double maxValue);

    void update();               
    void setValue(double value);

    std::string getName() const;
    double getValue() const;

private:
    std::string sensor_name;
    double sensor_value;
    double min_value;
    double max_value;
    bool in_spike = false;
    int spike_ticks_left = 0;
};
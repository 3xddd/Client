#pragma once
#include <string>
#include "SensorData.h"

class Patient
{
public:
    Patient(const std::string& name);

    void Update();
    std::string BuildSendString() const;
    std::string GetName() const;

private:
    std::string m_name;
    SensorData m_sensor;
};
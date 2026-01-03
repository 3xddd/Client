#include "pch.h"
#include "Patient.h"

Patient::Patient(const std::string& name)
    : m_name(name)
{
}

void Patient::Update()
{
    m_sensor.Generate();
}

std::string Patient::BuildSendString() const
{
    return m_name + "," + m_sensor.ToString() + "\n";
}

std::string Patient::GetName() const
{
    return m_name;
}
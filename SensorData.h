#pragma once
#include <string>

class SensorData
{
public:
    SensorData();

    void Generate();
    std::string ToString() const;

private:
    double topPressure; //顶部压力
    double waistPressure; //腰椎压力
    double angle; //角度
    double theta0; //初始角度
	int timeIndex;
};
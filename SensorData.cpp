#include "pch.h"
#include "SensorData.h"
#include <cstdlib>
#include <cstdio>
#include <cmath>
#include <ctime>
#include <Windows.h>

// 顶椎压力噪声系数：0.1~0.5
static const double kNoiseCoeff = 0.30;

// 异常数据概率（每次 Generate 一帧）：例如 0.02 = 2% 概率出现异常
static const double kAnomalyProb = 0.02;

// 严重异常概率（在异常里再分级）：例如 0.25 表示 25% 的异常是“严重异常”
static const double kSevereRate = 0.05;

static const double kWaistSmooth = 0.35;

static double Clamp(double x, double lo, double hi) //限制大小函数
{
    if (x < lo) return lo;
    if (x > hi) return hi;
    return x;
}

static double Rand01()
{
    return (rand() % 10000) / 10000.0;
}

// 高斯随机（Box-Muller）
static double RandN01()
{
    double u1 = Rand01();
    double u2 = Rand01();
    return sqrt(-2.0 * log(u1)) * cos(2.0 * 3.1415926535 * u2);
}

SensorData::SensorData()
    : topPressure(45.0), waistPressure(32.5), angle(0), timeIndex(0)
{
    srand((unsigned)time(nullptr));
    theta0 = rand() % 31; // 初始角度0-30度随机
}

void SensorData::Generate()
{
    // 时间索引递增 用于让角度随时间做轻微变化
    timeIndex++;

    // 生成高斯噪声 用于模拟正常顶椎压力随机波动
    double z = RandN01();

    // 顶椎压力噪声 噪声系数越大 波动越明显
    double noiseTop = z * kNoiseCoeff * 5.0;

    // 顶椎压力基准 45N 加上噪声
    double topNormal = 45.0 + noiseTop;

    // 限制顶椎压力在正常范围 30 到 60
    topNormal = Clamp(topNormal, 30.0, 60.0);
    topPressure = topNormal;

    // 腰椎压力变化 使用随机增量制造缓慢变化
    double variation = ((rand() % 200) - 100) / 100.0;
    double waistTarget = waistPressure + variation;

    // 限制腰椎目标值在正常范围 20 到 45
    waistTarget = Clamp(waistTarget, 20.0, 45.0);

    // 使用平滑系数让腰椎压力逐步逼近目标值
    waistPressure = waistPressure + (waistTarget - waistPressure) * kWaistSmooth;

    // 再次限制腰椎压力范围 防止计算后越界
    waistPressure = Clamp(waistPressure, 20.0, 45.0);

    // 角度随时间做小幅正弦变化 围绕初始角度 theta0 摆动
    double ang = theta0 + 0.5 * sin(0.01 * timeIndex);
    angle = Clamp(ang, 0.0, 30.0);

    // 按概率插入异常数据
    if (Rand01() < kAnomalyProb)
    {
        // 判定是否为严重异常
        bool severe = (Rand01() < kSevereRate);

        // 异常类型 0到2
        int type = rand() % 3;

        if (type == 0)
        {
            // 压力尖峰或超范围
            // 严重 75到110
            // 轻微 62到75
            if (severe)
                topPressure = 75.0 + Rand01() * 35.0;
            else
                topPressure = 62.0 + Rand01() * 13.0;
        }
        else if (type == 1)
        {
            // 压力掉落或异常偏低
            if (severe)
            {
                // 严重时接近0 并且腰椎也可能接近0
                topPressure = 0.0 + Rand01() * 10.0;
                waistPressure = 0.0 + Rand01() * 8.0;
            }
            else
            {
                // 轻微时略低于正常下限
                topPressure = 20.0 + Rand01() * 8.0;
            }
        }
        else
        {
            // 角度突变
            // 严重 直接跳到接近0或接近30
            // 轻微 跳 4到10 度
            if (severe)
            {
                angle = (Rand01() < 0.5) ? (0.0 + Rand01() * 2.0) : (28.0 + Rand01() * 2.0);
            }
            else
            {
                double jump = 4.0 + Rand01() * 6.0;
                angle = Clamp(angle + (Rand01() < 0.5 ? -jump : jump), 0.0, 30.0);
            }
        }

        // 让异常更明显 严重尖峰时腰椎也可能超范围
        if (severe && type == 0)
        {
            waistPressure = 48.0 + Rand01() * 20.0;
        }
    }
}


std::string SensorData::ToString() const
{
    char buf[128];
    sprintf_s(buf, "%.3f,%.3f,%.3f", topPressure, angle, waistPressure);
    return std::string(buf);
}

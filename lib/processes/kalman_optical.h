#pragma once
#include "../lib/post_process.h"
#include "../lib/sensors/optical_sensor.h"
#include <math.h>

class KalmanOptical : public PostProcess {
public:
    KalmanOptical(OpticalSensor* left, OpticalSensor* right) : 
        PostProcess("KalmanOptical"),
        _left(left),
        _right(right)
    {}

    void setup() override {
        startTask(10, 1);
    }

protected:
    void runOnce() override {
        // Access live data 
        float lx = _left->pos.x;
        float ly = _left->pos.y;
        float lh = _left->pos.h;

        float rx = _right->pos.x;
        float ry = _right->pos.y;
        float rh = _right->pos.h;
    }

private:
    OpticalSensor* _left;
    OpticalSensor* _right;
};
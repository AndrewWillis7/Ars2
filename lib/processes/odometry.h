#pragma once
#include "../lib/post_process.h"
#include "../lib/sensors/encoder_sensor.h"
#include <hw_config.h>

#include <cmath>

class Odometry : public PostProcess {
public:
    Odometry(EncoderSensor* left, EncoderSensor* right, EncoderSensor* middle) :
        PostProcess("Odometry"),
        _left(left),
        _right(right),
        _middle(middle)
    {}

    void setup() override {
        x = 0;
        y = 0;
        h = 0;
        previousTime = millis();
        startTask(10, 1);   // run at 100Hz
    }

    float x = 0;    // global X
    float y = 0;    // global Y
    float h = 0;    // heading

protected:
    struct OdomResult {
        float x;
        float y;
        float theta;
        float vx;
        float vy;
        float omega;
    };

    void runOnce() override {
        OdomResult result = calculate_odometry(x, y, h);
        x = result.x;
        y = result.y;
        h = result.theta;
    }

private:
    EncoderSensor* _left;
    EncoderSensor* _right;
    EncoderSensor* _middle;

    float previousPosition[3] = {0.0f, 0.0f, 0.0f};
    uint32_t previousTime = 0;

    OdomResult calculate_odometry(float x, float y, float theta) {

        float encoderPosition[3] = {
            static_cast<float>(_left->value),
            static_cast<float>(_right->value),
            static_cast<float>(_middle->value)
        };

        // Timestamp
        uint32_t currentTime = millis();
        float dt = (currentTime - previousTime) / 1000.0f;
        if (dt <= 0) dt = 0.001f;     // avoid divide-by-zero
        previousTime = currentTime;

        // Encoder deltas
        float deltaLeft  = encoderPosition[0] - previousPosition[0];
        float deltaRight = encoderPosition[1] - previousPosition[1];
        float deltaMid   = encoderPosition[2] - previousPosition[2];

        // Store them for next cycle
        previousPosition[0] = encoderPosition[0];
        previousPosition[1] = encoderPosition[1];
        previousPosition[2] = encoderPosition[2];

        // Heading change
        float deltaTheta = (deltaRight - deltaLeft) / PHY_WH_DIST_CEN;
        theta += deltaTheta;

        float xChange = 0;
        float yChange = 0;

        if (fabs(deltaTheta) < 1e-6f) {
            // Straight-line motion
            xChange = deltaMid;
            yChange = (deltaLeft + deltaRight) * 0.5f;
        } else {
            // Arc motion
            float turnRadius = PHY_WH_DIST_CEN * (deltaLeft + deltaRight) / (deltaRight - deltaLeft);
            float strafeRadius = deltaMid / deltaTheta;

            xChange = turnRadius * (cos(deltaTheta) - 1.0f)
                    + strafeRadius * sin(deltaTheta);

            yChange = turnRadius * sin(deltaTheta)
                    + strafeRadius * (cos(deltaTheta) - 1.0f);
        }

        // Rotate body→global
        float sinT = sin(theta);
        float cosT = cos(theta);

        float deltaX =  yChange * sinT - xChange * cosT;
        float deltaY = -yChange * cosT + xChange * sinT;

        // Apply scaling
        x += deltaX / PHY_TICK_P_IN;
        y += deltaY / PHY_TICK_P_IN;

        // Velocities
        float vx = deltaX / dt;
        float vy = deltaY / dt;
        float omega = deltaTheta / dt;

        return { x, y, theta, vx, vy, omega };
    }
};
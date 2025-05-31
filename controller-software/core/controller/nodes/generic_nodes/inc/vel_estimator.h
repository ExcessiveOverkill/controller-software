#include "node_factory.h"

#pragma once

class vel_estimator: public base_node {
    private:

        float output_val = 0.0f; // Current velocity estimate
        float max_vel = 0.0f; // Optional maximum velocity for clamping (0.0 = no clamp)

        input* position = nullptr; // Input for position (angle) measurements


        /**
         * Simple exponentially-weighted velocity estimator.
         * v_est[k] = α * raw_diff + (1-α) * v_est[k-1]
         */
        class VelocityEstimatorIIR {
            public:
                /**
                 * Call on each new encoder reading.
                 * @param theta  New angle measurement.
                 * @return       Smoothed velocity estimate.
                 */
                float update(double turns) {
                    if (!initialized & turns != 0.0) {
                        prev_turns = turns;
                        initialized = true;
                        return v_est;
                    }
                    // Compute raw difference (wrapped)
                    double d = turns - prev_turns;
                    float raw_v = float(d) / dt;
                    // Exponential smoothing
                    v_est = alpha * raw_v + (1.0f - alpha) * v_est;
                    prev_turns = turns;
                    return v_est;
                }

                uint32_t setAlpha(float new_alpha) {
                    if (new_alpha > 0.0f && new_alpha <= 1.0f) {
                        alpha = new_alpha;
                    } else {
                        return 1; // Invalid alpha
                    }
                    return 0; // Success
                }

                void setDt(float new_dt) {
                    if (new_dt > 0.0f) {
                        dt = new_dt;
                    } else {
                        std::cerr << "Error: dt must be positive" << std::endl;
                    }
                }


            private:
                float alpha = .2f;
                float dt = 0.001f; // Time step in seconds)
                double prev_turns = 0.0f;
                float v_est;
                bool initialized = false;
        };

        VelocityEstimatorIIR estimator;

    public:
        vel_estimator() {
            inputs.emplace("position", input(io_type::DOUBLE, nullptr));
            outputs.emplace("velocity", output(io_type::FLOAT, &output_val, &execution_number));
            position = &inputs["position"];
        }

        uint32_t configure_settings(json* data) override {
            
            /*
            parse settings from json
            
            {
                "config":
                {
                    "alpha": 0.2,   // smoothing factor (0.0 = no smoothing, 1.0 = no change)
                    "max_vel": 0.0, // clamping (0.0 = no clamp)
                    "dt": 0.001 // time step in seconds (default: 0.001)
                }

            }
            
            */

            if(data->find("config") == data->end()){
                std::cerr << "Error: config not found" << std::endl;
                return 1;
            }

            auto config = data->at("config");

            if(config.find("alpha") != config.end()){
                estimator.setAlpha(config["alpha"]);
            }
            if(config.find("max_vel") != config.end()){
                max_vel = config["max_vel"];
            }
            if(config.find("dt") != config.end()){
                float dt = config["dt"];
                if(dt > 0.0f) {
                    estimator.setDt(dt); // Update time step
                } else {
                    std::cerr << "Error: dt must be positive" << std::endl;
                    return 1;
                }
            }
            return 0;
        }

        uint32_t run() override {
            float vel = estimator.update(*reinterpret_cast<double*>(position->data_pointer));
            if(max_vel > 0.0f) {
                // Clamp velocity if max_vel is set
                vel = std::clamp(vel, -max_vel, max_vel);
            }
            output_val = vel;
            return 0;
        }
};


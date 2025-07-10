#include "node_factory.h"

#pragma once

class vel_estimator: public base_node {
    private:

        float output_val = 0.0f; // Current velocity estimate

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
                float dt = 0.001f; // Time step in seconds TODO: get this from the containing network
                double prev_turns = 0.0f;
                float v_est;
                bool initialized = false;
        };

        VelocityEstimatorIIR estimator;

    public:
        vel_estimator() {
            inputs.emplace("input", input(io_type::DOUBLE, nullptr));
            outputs.emplace("output", output(io_type::FLOAT, &output_val, &execution_number));
            position = &inputs["input"];
        }

        uint32_t configure_settings(json* data) override {
            
            /*
            parse settings from json
            
            {
                "config":
                {
                    "alpha": 0.2,   // smoothing factor (0.0 = no smoothing, 1.0 = no change)
                }
            }
            
            */

            if(data->find("alpha") != data->end()){
                estimator.setAlpha(data->at("alpha"));
            }
            else {
                std::cerr << "Error: 'alpha' not found in configuration" << std::endl;
                return 1; // Error code for missing alpha
            }
            return 0;
        }

        uint32_t run() override {
            float vel = estimator.update(*reinterpret_cast<double*>(position->data_pointer));
            output_val = vel;
            return 0;
        }
};


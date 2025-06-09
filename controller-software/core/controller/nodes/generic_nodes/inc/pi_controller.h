#include "node_factory.h"

#pragma once

class pi_controller: public base_node {
    private:

        input* error = nullptr;

        class controller{
            public:

                float input_error = 0.0;
                float output = 0.0;

                float Kp = 0.0;
                float Ki = 0.0;
                float i_limit = 0.0;
                float output_min = 0.0;
                float output_max = 0.0;

                void reset(){
                    integral = 0.0;
                }

                void update(){
                    // calculate integral
                    integral += input_error * Ki;

                    // clamp integral to limits
                    if(integral > i_limit){
                        integral = i_limit;
                    }
                    if(integral < -i_limit){
                        integral = -i_limit;
                    }

                    // calculate output
                    output = Kp * input_error + integral;
                    output = -output;   // invert output so negative error results in positive output

                    // clamp output to limits
                    if(output > output_max){
                        output = output_max;
                    }
                    if(output < output_min){
                        output = output_min;
                    }
                }

            private:
                float integral = 0.0;
        };

        controller ctrl;


    public:

        pi_controller(){

            inputs.emplace("input", input(io_type::FLOAT, nullptr));
            outputs.emplace("output", output(io_type::FLOAT, &(ctrl.output), &execution_number));

            error = &inputs["input"];
        }

        uint32_t run() override {
            // get input values
            ctrl.input_error = *reinterpret_cast<float*>(error->data_pointer);

            // update controller
            ctrl.update();

            return 0;
        }

        uint32_t configure_settings(json* data) override {
            
            /*
            parse settings from json
            
            {
                "config":
                {
                    "Kp": 1.0,
                    "Ki": 0.1,
                    "i_limit": 10.0,
                    "output_min": -100.0,
                    "output_max": 100.0
                }
            }
            
            */

            if(data->find("config") == data->end()){
                std::cerr << "Error: config not found" << std::endl;
                return 1;
            }

            auto config = data->at("config");

            if(config.find("Kp") != config.end()){
                ctrl.Kp = config["Kp"];
            }
            if(config.find("Ki") != config.end()){
                ctrl.Ki = config["Ki"];
            }
            if(config.find("i_limit") != config.end()){
                ctrl.i_limit = config["i_limit"];
            }
            if(config.find("output_min") != config.end()){
                ctrl.output_min = config["output_min"];
            }
            if(config.find("output_max") != config.end()){
                ctrl.output_max = config["output_max"];
            }
            return 0;
        }
};


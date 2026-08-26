#include "node_factory.h"

#pragma once

class pi_controller: public base_node {
    private:

        input* error = nullptr;
        input* reset = nullptr;

        class controller{
            public:

                float input_error = 0.0;
                float output = 0.0;
                bool out_sat = false;

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
                        out_sat = true;
                    }
                    else if(output < output_min){
                        output = output_min;
                        out_sat = true;
                    }
                    else{
                        out_sat = false;
                    }
                }

            private:
                float integral = 0.0;
        };

        controller ctrl;


    public:

        pi_controller(){

            inputs.emplace("input", input(io_type::FLOAT, nullptr));
            inputs.emplace("reset", input(io_type::BOOL, nullptr));
            outputs.emplace("output", output(io_type::FLOAT, &(ctrl.output), &execution_number));
            outputs.emplace("saturation", output(io_type::BOOL, &(ctrl.out_sat), &execution_number));

            error = &inputs["input"];
            reset = &inputs["reset"];
        }

        uint32_t run() override {
            // get input values
            ctrl.input_error = *reinterpret_cast<float*>(error->data_pointer);
            bool reset_value = *reinterpret_cast<bool*>(reset->data_pointer);

            // reset controller if reset input is true
            if(reset_value){
                ctrl.reset();
            }

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
                    "i_limit": 10.0,`
                    "output_min": -100.0,
                    "output_max": 100.0
                }
            }
            
            */

            if(data->find("Kp") != data->end()){
                ctrl.Kp = (*data)["Kp"];
            }
            if(data->find("Ki") != data->end()){
                ctrl.Ki = (*data)["Ki"];
            }
            if(data->find("i_limit") != data->end()){
                ctrl.i_limit = (*data)["i_limit"];
            }
            if(data->find("output_min") != data->end()){
                ctrl.output_min = (*data)["output_min"];
            }
            if(data->find("output_max") != data->end()){
                ctrl.output_max = (*data)["output_max"];
            }
            return 0;
        }
};


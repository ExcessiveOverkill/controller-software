#include "node_factory.h"
#include <chrono>

/*

Allows API commands to be sent to the controller to set float values
values are clampped to the min/max settings (-1 -> 1 by default)
if a timeout value is set, the value will be reset to the configured default value (0 by default) after the timeout period

*/

#pragma once

class api_cmd_float: public base_node {
    private:
        struct output_setting{
            std::string name;
            float min = -1.0;
            float max = 1.0;
            float default_value = 0.0;
            double timeout = 0.0;    // timeout in seconds
            double last_set_time = 0.0;
        };

        float output_values[32];

        uint32_t validate_settings(output_setting* setting){
            if(setting->min > setting->max){
                return 1;   // min is greater than max
            }

            if(setting->timeout < 0){
                return 2;   // timeout is negative
            }

            if(setting->default_value < setting->min || setting->default_value > setting->max){
                return 3;   // default value is outside of min/max range
            }
            return 0;
        }

        std::vector<output_setting> output_settings;
    public:

        api_cmd_float(){
            execution_number = -1;
        }

        unsigned int run() override {

            double seconds = std::chrono::duration_cast<std::chrono::duration<double>>(std::chrono::steady_clock::now().time_since_epoch()).count();

            // reset values if timeout has been reached
            for(auto& setting : output_settings){
                if(setting.timeout > 0.0 && setting.last_set_time + setting.timeout < seconds){
                    output_values[&setting - &output_settings[0]] = setting.default_value;
                }
            }

            return 0;
        }

        unsigned int configure_settings(json* json) override{
            
            /*
            parse settings from json
            {
                "config": [
                    {
                        "name": "output_0",
                        "min": -1.0,
                        "max": 1.0,
                        "default_value": 0.0,
                        "timeout": 0.0
                    }
                ],
                "set": [
                    {
                        "name": "output_0",
                        "value": 0.5
                    }
                ]
            }
            */

            

            if(json->find("config") != json->end()){    // config mode

                try{
                    for (auto& o : (*json)["config"]) {

                        bool output_exists = false;

                        output_setting new_settings;
                        new_settings.name = o["name"];
                        new_settings.min = o["min"];
                        new_settings.max = o["max"];
                        new_settings.default_value = o["default_value"];
                        new_settings.timeout = o["timeout"];

                        uint32_t validation_result = validate_settings(&new_settings);
                        if(validation_result != 0){
                            return validation_result;   // error in settings
                        }

                        for(auto& setting : output_settings){   // adjust existing setings if they exist
                            if(setting.name == new_settings.name){
                                setting = new_settings;
                                output_exists = true;
                                break;
                            }
                        }

                        if(!output_exists){ // add new setting if it doesn't exist
                            if(output_settings.size() >= 32){
                                return 4;   // too many outputs
                            }
                            output_settings.push_back(new_settings);
                            output_values[output_settings.size()-1] = new_settings.default_value;
                            outputs.emplace(new_settings.name, output(io_type::FLOAT, &output_values[output_settings.size()-1], &execution_number));
                        }                        
                    }
                }
                catch(json::exception& e){
                    return 2;   // error parsing json
                }
            }


            if(json->find("set") != json->end()){    // set mode
                try{
                    for (auto& o : (*json)["set"]) {
                        std::string name = o["name"];
                        float value = o["value"];

                        for(auto& setting : output_settings){
                            if(setting.name == name){
                                if(value < setting.min){
                                    value = setting.min;
                                }
                                if(value > setting.max){
                                    value = setting.max;
                                }
                                output_values[&setting - &output_settings[0]] = value;
                                break;
                            }
                        }
                    }
                }
                catch(json::exception& e){
                    return 2;   // error parsing json
                }
            }
            
            
            return 0;
        }

};
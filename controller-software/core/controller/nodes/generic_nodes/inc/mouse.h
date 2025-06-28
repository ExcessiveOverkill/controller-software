#include "node_factory.h"
#include "kins_6_axis.h"

#pragma once

class mouse: public base_node {
    private:
        bool configured = false;

        struct input_signal_ptrs{
            // actual mouse joints
            input* j1_fbk_pos = nullptr;
            input* j2_fbk_pos = nullptr;
            input* j3_fbk_pos = nullptr;
            input* j4_fbk_pos = nullptr;
            input* j5_fbk_pos = nullptr;
            input* j6_fbk_pos = nullptr;

        } in_sig_ptrs;

        struct joint_positions{
            float j1 = 0.0f;
            float j2 = 0.0f;
            float j3 = 0.0f;
            float j4 = 0.0f;
            float j5 = 0.0f;
            float j6 = 0.0f;
        };

        struct cartesian_positions{
            float x = 0.0f;
            float y = 0.0f;
            float z = 0.0f;
            float xangle = 0.0f;
            float yangle = 0.0f;
            float zangle = 0.0f;
        };

        struct joint_params{
            int32_t count_per_rev;
            int32_t home_pos;
        };

        std::array<joint_params,6> joint_parameters;

        struct input_signals{
            joint_positions fbk_joint_positions;
            bool button = false;

        } in_sigs;

        struct output_signals{
            cartesian_positions fbk_cartesian_positions;
        } out_sigs;

        InverseKinematics ik_solver;

        void update_inputs();
        void create_inputs();
        void create_outputs();

        void update_forward_kinematics_outputs();

        void rot_to_euler(const std::array<float,9>* rot, std::array<float,3>* euler);


        
    public:

        mouse();

        uint32_t run() override;

        unsigned int configure_settings(json* json) override{
            
            /*
            parse settings from json
            {
                "dh_params":[   // robot dh params
                    {
                        "alpha": 0,
                        "a": 0,
                        "d": 0,
                        "theta": 0
                    }
                    // other joints...
                ],

                "joint_params":[
                    {
                        "count_per_rev": 100,
                        "home_pos": 0
                    }
                    // other joints...
                ]
            }
            */

            bool dh_set = false;
            bool joint_params_set = false;

            if(json->find("dh_params") != json->end()){
                auto& dh_params = (*json)["dh_params"];
                if(dh_params.is_array() && dh_params.size() == 6){
                    std::array<InverseKinematics::DHParam,6> dh;
                    for(size_t i = 0; i < 6; i++){
                        dh[i].alpha = dh_params[i]["alpha"].get<float>() * M_PI / 180.0f; // convert degrees to radians
                        dh[i].a = dh_params[i]["a"].get<float>();
                        dh[i].d = dh_params[i]["d"].get<float>();
                        dh[i].thetaOffset = dh_params[i]["theta"].get<float>() * M_PI / 180.0f; // convert degrees to radians
                    }
                    ik_solver.setDHParameters(dh);
                }
                else {
                    std::cerr << "Invalid DH parameters format, expected an array of 6 objects." << std::endl;
                    return 1; // error code for configuration failure
                }
                dh_set = true;
            }

            if(json->find("joint_params") != json->end()){
                auto& joint_params = (*json)["joint_params"];
                if(joint_params.is_array() && joint_params.size() == 6){
                    for(size_t i = 0; i < 6; i++){
                        joint_parameters[i].count_per_rev = joint_params[i]["count_per_rev"].get<int32_t>();
                        joint_parameters[i].home_pos = joint_params[i]["home_pos"].get<int32_t>();
                    }
                }
                else {
                    std::cerr << "Invalid joint parameters format, expected an array of 6 objects." << std::endl;
                    return 1; // error code for configuration failure
                }
                joint_params_set = true;
            }

            configured |= dh_set && joint_params_set;

            if(!configured){
                std::cerr << "Kins node initial configuration failed, missing required parameters." << std::endl;
                return 1; // error code for configuration failure
            }
            
            return 0;
        }

};
#include "node_factory.h"
#include "kins_6_axis.h"

#pragma once

class kins: public base_node {
    private:
        bool configured = false;

        float time_step = 0.001f; // time step for running the node

        struct input_signal_ptrs{
            // actual robot joints
            input* j1_fbk_pos = nullptr;
            input* j2_fbk_pos = nullptr;
            input* j3_fbk_pos = nullptr;
            input* j4_fbk_pos = nullptr;
            input* j5_fbk_pos = nullptr;
            input* j6_fbk_pos = nullptr;

            // controls
            input* jog_axis_select = nullptr;
            input* jog_vel = nullptr;
            input* jog_mode = nullptr;
            input* control_mode = nullptr;
            input* speed_override = nullptr;

            // commanded positions
            input* j1_cmd_pos = nullptr;
            input* j2_cmd_pos = nullptr;
            input* j3_cmd_pos = nullptr;
            input* j4_cmd_pos = nullptr;
            input* j5_cmd_pos = nullptr;
            input* j6_cmd_pos = nullptr;
            
            input* x_cmd_pos = nullptr;
            input* y_cmd_pos = nullptr;
            input* z_cmd_pos = nullptr;
            input* xangle_cmd_pos = nullptr;
            input* yangle_cmd_pos = nullptr;
            input* zangle_cmd_pos = nullptr;

            input* reset = nullptr;

        } in_sig_ptrs;

        struct joint_limit{
            float min_pos = 0;
            float max_pos = 0;
            float max_vel = 0;
            float max_acc = 0;
        };

        std::array<joint_limit,6> joint_limits;

        struct cartesian_limit{
            float min_pos = 0;
            float max_pos = 0;
            float max_vel = 0;
            float max_acc = 0;
        };

        struct joint_positions{
            double j1 = 0.0f;
            double j2 = 0.0f;
            double j3 = 0.0f;
            double j4 = 0.0f;
            double j5 = 0.0f;
            double j6 = 0.0f;
        };

        struct cartesian_positions{
            float x = 0.0f;
            float y = 0.0f;
            float z = 0.0f;
            float xangle = 0.0f;
            float yangle = 0.0f;
            float zangle = 0.0f;
        };

        std::array<cartesian_limit,6> cartesian_limits;

        enum class jog_modes{
            JOINT = 1,
            CARTESIAN = 2,
            UNDEFINED = 0
        };
        enum class control_modes{
            JOINT = 1,
            CARTESIAN = 2,
            JOG = 3,
            UNDEFINED = 0
        };

        struct input_signals{
            joint_positions fbk_joint_positions;
            float jog_vel = 0.0f;
            uint8_t jog_axis_select = 0;
            jog_modes jog_mode = jog_modes::UNDEFINED;
            control_modes control_mode = control_modes::UNDEFINED;
            float speed_override = 1.0f;
            cartesian_positions cmd_cartesian_positions;
            joint_positions cmd_joint_positions;
            bool reset = false;

        } in_sigs;

        struct output_signals{
            joint_positions cmd_joint_positions;
            cartesian_positions fbk_cartesian_positions;
        } out_sigs;

        InverseKinematics ik_solver;

        void update_inputs();
        void create_inputs();
        void create_outputs();

        void update_forward_kinematics_outputs();

        struct joint_lim_distance{
            double to_min = 0.0f;   // distance to min limit, positive if within limits, negative if outside
            double to_max = 0.0f;   // distance to max limit
            float allowed_positive_vel = 0.0f; // how fast we can move in the positive direction
            float allowed_negative_vel = 0.0f;

            bool infinite = false; // true if joint is continuous
        };
        std::array<joint_lim_distance,6> joint_distances_to_limit;
        void update_joint_distances_to_limit();

        float last_jog_vel = 0.0f; // last jog vel used, to limit acceleration
        float max_jog_speed = 0.1f; // max jog speed as a portion of the joint max_vel
        void jog_joint(uint8_t jog_axis, float jog_vel, float speed_override);


        void rot_to_euler(const std::array<float,9>* rot, std::array<float,3>* euler);


        
    public:

        kins();

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

                "limits":{
                    0:{
                        "min_pos": -160,    // degrees, both set to zero for continuous joints
                        "max_pos": 160,
                        "max_vel": 50,      // degrees per second
                        "max_acc": 100      // degrees per second squared
                    }
                    // other joints...
                },

                "max_jog_speed": .1,    // max speed for jogging, as a portion of the joint max_vel
                "cartesian_limits":{   // cartesian limits for the end effector
                    "x":{
                        "min_pos": -.2,
                        "max_pos": .2,
                        "max_vel": .1,
                        "max_acc": .2
                    }
                    // other axes...
                    "xangle":{
                        "min_pos": -180,
                        "max_pos": 180,
                        "max_vel": 50,
                        "max_acc": 100
                    }
                    // other angles...
                }
            }
            */
            bool dh_set = false;
            bool limits_set = false;
            bool max_jog_speed_set = false;
            bool cartesian_limits_set = false;


            if(json->find("dh_params") != json->end()){
                auto& dh_params = (*json)["dh_params"];
                if(dh_params.is_array() && dh_params.size() == 6){
                    std::array<InverseKinematics::DHParam,6> dh;
                    for(size_t i = 0; i < 6; i++){
                        dh[i].alpha = dh_params[i]["alpha"].get<float>();
                        dh[i].a = dh_params[i]["a"].get<float>();
                        dh[i].d = dh_params[i]["d"].get<float>();
                        dh[i].thetaOffset = dh_params[i]["theta"].get<float>();
                    }
                    ik_solver.setDHParameters(dh);
                }
                else {
                    std::cerr << "Invalid DH parameters format, expected an array of 6 objects." << std::endl;
                    return 1; // error code for configuration failure
                }
                dh_set = true;
            }


            if(json->find("limits") != json->end()){
                auto& limits = (*json)["limits"];
                if(limits.is_object()){
                    for(int i = 0; i < 6; i++){
                        if(limits.find(std::to_string(i)) != limits.end()){
                            joint_limits[i].min_pos = limits[std::to_string(i)]["min_pos"].get<float>();
                            joint_limits[i].max_pos = limits[std::to_string(i)]["max_pos"].get<float>();
                            joint_limits[i].max_vel = limits[std::to_string(i)]["max_vel"].get<float>();
                            joint_limits[i].max_acc = limits[std::to_string(i)]["max_acc"].get<float>();
                        }
                        else {
                            std::cerr << "Missing limit for joint " << i << std::endl;
                            return 1; // error code for configuration failure
                        }
                    }
                }
                else {
                    std::cerr << "Invalid limits format, expected an object." << std::endl;
                    return 1; // error code for configuration failure
                }
                limits_set = true;
            }


            if(json->find("max_jog_speed") != json->end()){
                max_jog_speed = (*json)["max_jog_speed"].get<float>();
                if(max_jog_speed < 0.0f || max_jog_speed > 1.0f){
                    std::cerr << "Invalid max jog speed, must be between 0.0 and 1.0." << std::endl;
                    return 1; // error code for configuration failure
                }
                else {
                    max_jog_speed_set = true;
                }
            }

            if(json->find("cartesian_limits") != json->end()){
                auto& cartesian_limits_json = (*json)["cartesian_limits"];
                if(cartesian_limits_json.is_object()){
                    for(uint8_t i = 0; i < 6; i++){
                        std::string axis_name;
                        switch(i){
                            case 0: axis_name = "x"; break;
                            case 1: axis_name = "y"; break;
                            case 2: axis_name = "z"; break;
                            case 3: axis_name = "xangle"; break;
                            case 4: axis_name = "yangle"; break;
                            case 5: axis_name = "zangle"; break;
                        }
                        if(cartesian_limits_json.find(axis_name) != cartesian_limits_json.end()){
                            cartesian_limits[i].min_pos = cartesian_limits_json[axis_name]["min_pos"].get<float>();
                            cartesian_limits[i].max_pos = cartesian_limits_json[axis_name]["max_pos"].get<float>();
                            cartesian_limits[i].max_vel = cartesian_limits_json[axis_name]["max_vel"].get<float>();
                            cartesian_limits[i].max_acc = cartesian_limits_json[axis_name]["max_acc"].get<float>();
                        }
                        else {
                            std::cerr << "Missing cartesian limit for axis " << axis_name << std::endl;
                            return 1; // error code for configuration failure
                        }
                    }
                }
                else {
                    std::cerr << "Invalid cartesian limits format, expected an object." << std::endl;
                    return 1; // error code for configuration failure
                }
                cartesian_limits_set = true;
            }

            configured = dh_set && limits_set && max_jog_speed_set && cartesian_limits_set;
            if(!configured){
                std::cerr << "Kins node initial configuration failed, missing required parameters." << std::endl;
                return 1; // error code for configuration failure
            }
            
            return 0;
        }

};
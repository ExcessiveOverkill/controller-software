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
            input* tool_select = nullptr;

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
            MOUSE = 3,
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
            uint8_t tool_select = 0;
            bool reset = false;

        } in_sigs;

        struct output_signals{
            joint_positions cmd_joint_positions;
            cartesian_positions fbk_cartesian_positions;
        } out_sigs;

        InverseKinematics ik_solver;

        bool bypass_limits = true; // if true, no limits will be applied

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

        float max_jog_speed = 0.1f; // max jog speed as a portion of the joint max_vel

        cartesian_positions last_cmd_cartesian_positions; // these are what were used with the inverse kins
        cartesian_positions prev_cmd_cartesian_positions; // TCP position before this cycle's jog step

        jog_modes last_jog_mode = jog_modes::UNDEFINED; // last jog mode used, to prevent unnecessary calculations

        float last_joint_jog_vel = 0.0f; // last jog vel used, to limit acceleration
        void jog_joint(uint8_t jog_axis, float jog_vel, float speed_override);

        float last_cartesian_jog_vel = 0.0f; // last jog vel used, to limit acceleration
        void jog_cartesian(uint8_t jog_axis, float jog_vel, float speed_override);

        cartesian_positions starting_mouse_positions;
        cartesian_positions starting_robot_positions;
        quat_rot starting_mouse_quat;
        quat_rot starting_robot_quat;
        
        void jog_mouse();

        void inverse_kins();

        bool validate_kins_solution(const std::array<float,6>* joint_angles, const cartesian_positions* cartesian_pos, const float pos_tol, const float angle_tol);

        void set_closest_joint_positions(std::array<float,6>* joint_angles);

        float apply_cartesian_joint_rate_limit(std::array<float,6>& joint_angles); // returns min scale applied
        
        bool clamp_to_limits();

        void rot_to_euler(const std::array<float,9>* rot, std::array<float,3>* euler);

        struct tool {
            // offset from flange to TCP
            std::array<float,3> position;
            std::array<float,3> orientation;
        };

        tool* current_tool = nullptr; // current tool offset
        std::vector<tool> tools; // list of tools

        uint32_t select_tool(uint8_t tool_index); // select a tool by index

        uint32_t add_tool(const tool new_tool); // add a new tool to the list

        void apply_tool_offset(const std::array<float,3>& flange_pos,
                                       const std::array<float,3>& flange_eul,
                                       std::array<float,3>& tool_pos,
                                       std::array<float,3>& tool_eul);  // get postion of the current TCP

        void remove_tool_offset(const std::array<float,3>& tcp_pos,
                                       const std::array<float,3>& tcp_eul,
                                       std::array<float,3>& flange_pos,
                                       std::array<float,3>& flange_eul); // get position of the flange from the TCP

        double wrapNearest(double absAngle, double cmdAngle);

        struct AxisFilter {
            float P_prev    = 0.0f;  // last filtered position
            float V_prev    = 0.0f;  // last filtered velocity

            // tuning knobs
            float V_max;            // max speed (units/sec)
            float A_max;            // max accel (units/sec²)

            AxisFilter(float vmax, float amax)
            : V_max(vmax), A_max(amax) {}

            // call this every dt seconds
            float update(float P_cmd, float dt) {
                // 1) desired vel to catch cmd
                float V_des = (P_cmd - P_prev) / dt;

                // 2) saturate velocity
                float V_sat = std::clamp(V_des, -V_max, +V_max);

                // 3) compute needed accel and limit it
                float A_req = (V_sat - V_prev) / dt;
                float A_sat = std::clamp(A_req, -A_max, +A_max);

                // 4) integrate accel → new velocity
                float V_new = V_prev + A_sat * dt;

                // 5) integrate velocity → new position
                float P_new = P_prev + V_new * dt;

                // 6) save for next time
                V_prev = V_new;
                P_prev = P_new;
                return P_new;
            }
        };

        struct CDampedFilter {
            float y     = 0, ydot = 0;
            float ωn;           // natural frequency (rad/s)

            CDampedFilter(float wn) : ωn(wn) {}

            // simple Euler integration; call every dt
            float update(float x, float dt) {
                // compute accel
                float ydd = ωn*ωn*(x - y) - 2*ωn*ydot;
                // integrate
                ydot += ydd * dt;
                y    += ydot * dt;
                return y;
            }

            float reset() {
                y = 0;
                ydot = 0;
                return y;
            }
        };

        quat_rot normalize(const quat_rot &q) {
            float norm = sqrtf(q.w*q.w + q.x*q.x + q.y*q.y + q.z*q.z);
            if (norm < 1e-6f) return {1, 0, 0, 0}; // return identity if norm is too small
            return {q.w/norm, q.x/norm, q.y/norm, q.z/norm};
        }

        quat_rot slerp(const quat_rot &A, const quat_rot &B, float t) {
            // compute cosθ = dot(A,B)
            float cosθ = A.w*B.w + A.x*B.x + A.y*B.y + A.z*B.z;
            // if opposite hemisphere, flip
            quat_rot T = (cosθ < 0 ? quat_rot{-B.w,-B.x,-B.y,-B.z} : B);
            if (std::abs(cosθ) > 0.9995f) {
                // linear fallback
                quat_rot R = { A.w + t*(T.w-A.w),
                        A.x + t*(T.x-A.x),
                        A.y + t*(T.y-A.y),
                        A.z + t*(T.z-A.z) };
                return normalize(R);
            }
            float θ = acosf(fabsf(cosθ));
            float sinθ = sinf(θ);
            float w1 = sinf((1-t)*θ)/sinθ;
            float w2 = sinf(t*θ)/sinθ;
            return normalize(quat_rot{
                A.w*w1 + T.w*w2,
                A.x*w1 + T.x*w2,
                A.y*w1 + T.y*w2,
                A.z*w1 + T.z*w2
            });
        }


        // mouse filters

        // const float mouse_filter_pos_vmax = 0.5f; // max speed for mouse jog (m/s)
        // const float mouse_filter_pos_amax = 1.0f; // max acceleration for mouse (m/s2)
        // const float mouse_filter_angle_vmax = 30.0f * M_PI / 180.0f; // max speed for mouse jog (rad/s)
        // const float mouse_filter_angle_amax = 10.0f * M_PI / 180.0f; // max acceleration for mouse (rad/s2)

        // AxisFilter axis_filters[6] = {
        //     AxisFilter(mouse_filter_pos_vmax, mouse_filter_pos_amax), // X
        //     AxisFilter(mouse_filter_pos_vmax, mouse_filter_pos_amax), // Y
        //     AxisFilter(mouse_filter_pos_vmax, mouse_filter_pos_amax), // Z
        //     AxisFilter(mouse_filter_angle_vmax, mouse_filter_angle_amax), // Xangle
        //     AxisFilter(mouse_filter_angle_vmax, mouse_filter_angle_amax), // Yangle
        //     AxisFilter(mouse_filter_angle_vmax, mouse_filter_angle_amax)  // Zangle
        // };

        CDampedFilter pos_smoothing_filters[3] = {
            CDampedFilter(3.0f), // X
            CDampedFilter(3.0f), // Y
            CDampedFilter(3.0f)  // Z
        };

        const float slerp_a = .0002f;
        quat_rot quat_smoothing_filter = {1.0f, 0.0f, 0.0f, 0.0f};


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
                },

                "tools":[
                    {
                        "position": [0.0, 0.0, 0.0], // offset from flange to TCP in meters
                        "orientation": [0.0, 0.0, 0.0] // offset from flange to TCP in radians
                    }
                    // other tools...
                ]
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


            if(json->find("limits") != json->end()){
                auto& limits = (*json)["limits"];
                if(limits.is_object()){
                    for(int i = 0; i < 6; i++){
                        if(limits.find(std::to_string(i)) != limits.end()){
                            joint_limits[i].min_pos = limits[std::to_string(i)]["min_pos"].get<float>() * M_PI / 180.0f; // convert degrees to radians
                            joint_limits[i].max_pos = limits[std::to_string(i)]["max_pos"].get<float>() * M_PI / 180.0f;
                            joint_limits[i].max_vel = limits[std::to_string(i)]["max_vel"].get<float>() * M_PI / 180.0f;
                            joint_limits[i].max_acc = limits[std::to_string(i)]["max_acc"].get<float>() * M_PI / 180.0f;
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
                        if(i > 2){  // angle axes
                            cartesian_limits[i].min_pos *= M_PI / 180.0f; // convert degrees to radians
                            cartesian_limits[i].max_pos *= M_PI / 180.0f;
                            cartesian_limits[i].max_vel *= M_PI / 180.0f;
                            cartesian_limits[i].max_acc *= M_PI / 180.0f;
                        }
                    }
                }
                else {
                    std::cerr << "Invalid cartesian limits format, expected an object." << std::endl;
                    return 1; // error code for configuration failure
                }
                cartesian_limits_set = true;
            }

            // optionally add tools
            if(json->find("tools") != json->end()){
                auto& tools_json = (*json)["tools"];
                if(tools_json.is_array()){
                    for(auto& tool_json : tools_json){
                        if(tool_json.is_object() && tool_json.contains("position") && tool_json.contains("orientation")){
                            tool new_tool;
                            new_tool.position = tool_json["position"].get<std::array<float,3>>();
                            new_tool.orientation = tool_json["orientation"].get<std::array<float,3>>();
                            // convert orientation from degrees to radians
                            for(auto& angle : new_tool.orientation){
                                angle *= M_PI / 180.0f;
                            }
                            auto ret = add_tool(new_tool);
                            if(ret != 0){
                                std::cerr << "Failed to add tool." << std::endl;
                                return 1; // error code for configuration failure
                            }
                        }
                        else {
                            std::cerr << "Invalid tool format, expected an object with position and orientation arrays." << std::endl;
                            return 1; // error code for configuration failure
                        }
                    }
                }
                else {
                    std::cerr << "Invalid tools format, expected an array." << std::endl;
                    return 1; // error code for configuration failure
                }
            }

            configured |= dh_set && limits_set && max_jog_speed_set && cartesian_limits_set;
            if(!configured){
                std::cerr << "Kins node initial configuration failed, missing required parameters." << std::endl;
                return 1; // error code for configuration failure
            }
            
            return 0;
        }

};
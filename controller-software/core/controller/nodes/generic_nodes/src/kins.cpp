#include "kins.h"
#include <cmath>

kins::kins(){
    // create all IO
    create_inputs();
    create_outputs();
}

void kins::create_inputs(){

    inputs.emplace("j1_fbk_pos", input(io_type::DOUBLE, nullptr));
    inputs.emplace("j2_fbk_pos", input(io_type::DOUBLE, nullptr));
    inputs.emplace("j3_fbk_pos", input(io_type::DOUBLE, nullptr));
    inputs.emplace("j4_fbk_pos", input(io_type::DOUBLE, nullptr));
    inputs.emplace("j5_fbk_pos", input(io_type::DOUBLE, nullptr));
    inputs.emplace("j6_fbk_pos", input(io_type::DOUBLE, nullptr));
    in_sig_ptrs.j1_fbk_pos = &inputs["j1_fbk_pos"];
    in_sig_ptrs.j2_fbk_pos = &inputs["j2_fbk_pos"];
    in_sig_ptrs.j3_fbk_pos = &inputs["j3_fbk_pos"];
    in_sig_ptrs.j4_fbk_pos = &inputs["j4_fbk_pos"];
    in_sig_ptrs.j5_fbk_pos = &inputs["j5_fbk_pos"];
    in_sig_ptrs.j6_fbk_pos = &inputs["j6_fbk_pos"];

    inputs.emplace("jog_axis_select", input(io_type::UINT8, nullptr));
    inputs.emplace("jog_vel", input(io_type::FLOAT, nullptr));
    inputs.emplace("jog_mode", input(io_type::UINT8, nullptr));
    inputs.emplace("control_mode", input(io_type::UINT8, nullptr));
    inputs.emplace("speed_override", input(io_type::FLOAT, nullptr));
    in_sig_ptrs.jog_axis_select = &inputs["jog_axis_select"];
    in_sig_ptrs.jog_vel = &inputs["jog_vel"];
    in_sig_ptrs.jog_mode = &inputs["jog_mode"];
    in_sig_ptrs.control_mode = &inputs["control_mode"];
    in_sig_ptrs.speed_override = &inputs["speed_override"];

    inputs.emplace("j1_cmd_pos", input(io_type::DOUBLE, nullptr));
    inputs.emplace("j2_cmd_pos", input(io_type::DOUBLE, nullptr));
    inputs.emplace("j3_cmd_pos", input(io_type::DOUBLE, nullptr));
    inputs.emplace("j4_cmd_pos", input(io_type::DOUBLE, nullptr));
    inputs.emplace("j5_cmd_pos", input(io_type::DOUBLE, nullptr));
    inputs.emplace("j6_cmd_pos", input(io_type::DOUBLE, nullptr));
    in_sig_ptrs.j1_cmd_pos = &inputs["j1_cmd_pos"];
    in_sig_ptrs.j2_cmd_pos = &inputs["j2_cmd_pos"];
    in_sig_ptrs.j3_cmd_pos = &inputs["j3_cmd_pos"];
    in_sig_ptrs.j4_cmd_pos = &inputs["j4_cmd_pos"];
    in_sig_ptrs.j5_cmd_pos = &inputs["j5_cmd_pos"];
    in_sig_ptrs.j6_cmd_pos = &inputs["j6_cmd_pos"];

    inputs.emplace("x_cmd_pos", input(io_type::FLOAT, nullptr));
    inputs.emplace("y_cmd_pos", input(io_type::FLOAT, nullptr));
    inputs.emplace("z_cmd_pos", input(io_type::FLOAT, nullptr));
    inputs.emplace("xangle_cmd_pos", input(io_type::FLOAT, nullptr));
    inputs.emplace("yangle_cmd_pos", input(io_type::FLOAT, nullptr));
    inputs.emplace("zangle_cmd_pos", input(io_type::FLOAT, nullptr));
    in_sig_ptrs.x_cmd_pos = &inputs["x_cmd_pos"];
    in_sig_ptrs.y_cmd_pos = &inputs["y_cmd_pos"];
    in_sig_ptrs.z_cmd_pos = &inputs["z_cmd_pos"];
    in_sig_ptrs.xangle_cmd_pos = &inputs["xangle_cmd_pos"];
    in_sig_ptrs.yangle_cmd_pos = &inputs["yangle_cmd_pos"];
    in_sig_ptrs.zangle_cmd_pos = &inputs["zangle_cmd_pos"];

    inputs.emplace("reset", input(io_type::BOOL, nullptr));
    in_sig_ptrs.reset = &inputs["reset"];
}

void kins::create_outputs(){
    outputs.emplace("j1_cmd_pos", output(io_type::DOUBLE, &(out_sigs.cmd_joint_positions.j1), &execution_number));
    outputs.emplace("j2_cmd_pos", output(io_type::DOUBLE, &(out_sigs.cmd_joint_positions.j2), &execution_number));
    outputs.emplace("j3_cmd_pos", output(io_type::DOUBLE, &(out_sigs.cmd_joint_positions.j3), &execution_number));
    outputs.emplace("j4_cmd_pos", output(io_type::DOUBLE, &(out_sigs.cmd_joint_positions.j4), &execution_number));
    outputs.emplace("j5_cmd_pos", output(io_type::DOUBLE, &(out_sigs.cmd_joint_positions.j5), &execution_number));
    outputs.emplace("j6_cmd_pos", output(io_type::DOUBLE, &(out_sigs.cmd_joint_positions.j6), &execution_number));

    outputs.emplace("x_fbk_pos", output(io_type::FLOAT, &(out_sigs.fbk_cartesian_positions.x), &execution_number));
    outputs.emplace("y_fbk_pos", output(io_type::FLOAT, &(out_sigs.fbk_cartesian_positions.y), &execution_number));
    outputs.emplace("z_fbk_pos", output(io_type::FLOAT, &(out_sigs.fbk_cartesian_positions.z), &execution_number));
    outputs.emplace("xangle_fbk_pos", output(io_type::FLOAT, &(out_sigs.fbk_cartesian_positions.xangle), &execution_number));
    outputs.emplace("yangle_fbk_pos", output(io_type::FLOAT, &(out_sigs.fbk_cartesian_positions.yangle), &execution_number));
    outputs.emplace("zangle_fbk_pos", output(io_type::FLOAT, &(out_sigs.fbk_cartesian_positions.zangle), &execution_number));
}

void kins::update_inputs(){
    in_sigs.fbk_joint_positions.j1 = *(double*)(in_sig_ptrs.j1_fbk_pos->data_pointer);
    in_sigs.fbk_joint_positions.j2 = *(double*)(in_sig_ptrs.j2_fbk_pos->data_pointer);
    in_sigs.fbk_joint_positions.j3 = *(double*)(in_sig_ptrs.j3_fbk_pos->data_pointer);
    in_sigs.fbk_joint_positions.j4 = *(double*)(in_sig_ptrs.j4_fbk_pos->data_pointer);
    in_sigs.fbk_joint_positions.j5 = *(double*)(in_sig_ptrs.j5_fbk_pos->data_pointer);
    in_sigs.fbk_joint_positions.j6 = *(double*)(in_sig_ptrs.j6_fbk_pos->data_pointer);
    in_sigs.jog_vel = *(float*)(in_sig_ptrs.jog_vel->data_pointer);
    in_sigs.jog_axis_select = *(uint8_t*)(in_sig_ptrs.jog_axis_select->data_pointer);
    in_sigs.jog_mode = (jog_modes)(*(uint8_t*)(in_sig_ptrs.jog_mode->data_pointer));
    in_sigs.control_mode = (control_modes)(*(uint8_t*)(in_sig_ptrs.control_mode->data_pointer));
    in_sigs.speed_override = *(float*)(in_sig_ptrs.speed_override->data_pointer);
    in_sigs.cmd_cartesian_positions.x = *(float*)(in_sig_ptrs.x_cmd_pos->data_pointer);
    in_sigs.cmd_cartesian_positions.y = *(float*)(in_sig_ptrs.y_cmd_pos->data_pointer);
    in_sigs.cmd_cartesian_positions.z = *(float*)(in_sig_ptrs.z_cmd_pos->data_pointer);
    in_sigs.cmd_cartesian_positions.xangle = *(float*)(in_sig_ptrs.xangle_cmd_pos->data_pointer);
    in_sigs.cmd_cartesian_positions.yangle = *(float*)(in_sig_ptrs.yangle_cmd_pos->data_pointer);
    in_sigs.cmd_cartesian_positions.zangle = *(float*)(in_sig_ptrs.zangle_cmd_pos->data_pointer);
    in_sigs.cmd_joint_positions.j1 = *(double*)(in_sig_ptrs.j1_cmd_pos->data_pointer);
    in_sigs.cmd_joint_positions.j2 = *(double*)(in_sig_ptrs.j2_cmd_pos->data_pointer);
    in_sigs.cmd_joint_positions.j3 = *(double*)(in_sig_ptrs.j3_cmd_pos->data_pointer);
    in_sigs.cmd_joint_positions.j4 = *(double*)(in_sig_ptrs.j4_cmd_pos->data_pointer);
    in_sigs.cmd_joint_positions.j5 = *(double*)(in_sig_ptrs.j5_cmd_pos->data_pointer);
    in_sigs.cmd_joint_positions.j6 = *(double*)(in_sig_ptrs.j6_cmd_pos->data_pointer);
    in_sigs.reset = *(bool*)(in_sig_ptrs.reset->data_pointer);

    in_sigs.speed_override = fmax(0.001f, fmin(in_sigs.speed_override, 1.0f)); // clamp speed override to [0.001, 1]
}

uint32_t kins::run(){
    // update inputs
    update_inputs();

    // update forward kinematics outputs (so we always have the current cartesian position)
    update_forward_kinematics_outputs();

    update_joint_distances_to_limit();

    switch(in_sigs.control_mode){
        case control_modes::JOG:
            // update joint positions based on jog inputs
            switch(in_sigs.jog_mode){
                case jog_modes::JOINT:
                    if(last_jog_mode != jog_modes::JOINT || in_sigs.reset){
                        out_sigs.cmd_joint_positions = in_sigs.fbk_joint_positions;
                        last_joint_jog_vel = 0.0f;
                    }
                    // jog in joint space, automatic joint endstop limiting
                    jog_joint(in_sigs.jog_axis_select, in_sigs.jog_vel, in_sigs.speed_override);
                    break;
                case jog_modes::CARTESIAN:
                    if(last_jog_mode != jog_modes::CARTESIAN || in_sigs.reset){
                        last_cmd_cartesian_positions = out_sigs.fbk_cartesian_positions; // reset the commanded cartesian positions to the current positions
                    }
                    // jog in cartesian space, no automatic endstop limiting (yet)
                    jog_cartesian(in_sigs.jog_axis_select, in_sigs.jog_vel, in_sigs.speed_override);
                    break;
                case jog_modes::MOUSE:
                    if(last_jog_mode != jog_modes::MOUSE || in_sigs.reset){
                        last_cmd_cartesian_positions = out_sigs.fbk_cartesian_positions; // reset the commanded cartesian positions to the current positions
                        starting_mouse_positions = in_sigs.cmd_cartesian_positions; // store the starting positions for mouse jog
                        starting_robot_positions = out_sigs.fbk_cartesian_positions; // store the starting robot positions
                        starting_mouse_quat = quatFromZYX(starting_mouse_positions.zangle, starting_mouse_positions.yangle, starting_mouse_positions.xangle);
                        starting_robot_quat = quatFromZYX(starting_robot_positions.zangle, starting_robot_positions.yangle, starting_robot_positions.xangle);
                        // reset filters
                        quat_smoothing_filter = starting_robot_quat;
                        for(int i = 0; i < 3; i++){
                            pos_smoothing_filters[i].reset();
                        }
                    }
                    // jog in cartesian space based on mouse delta, no automatic endstop limiting (yet)
                    jog_mouse();
                    break;
                default:
                    // no jog mode selected, do nothing
                    break;
            }

            last_jog_mode = in_sigs.jog_mode;
            break;
    }

    if(in_sigs.reset){
        // reset the commanded joint positions to the current positions
        out_sigs.cmd_joint_positions = in_sigs.fbk_joint_positions;
    }
    
    // TODO: fix this possibly causing motion on starup if past a limit
    bool on_limit = clamp_to_limits();  // clamp the commanded joint positions to the limits as a failsafe



    return 0; // no errors
}

bool kins::clamp_to_limits(){
    // clamp the commanded joint positions to the limits

    double tol = 1e-3; // tolerance for clamping, to avoid false triggering of flag

    bool modified = false; // flag to check if any joint was modified
    for(int i = 0; i < 6; i++){
        double zero = 0.0;
        double* cmd_pos = nullptr;
        double* relative_cmd_pos = &zero;
        switch(i){
            case 0: cmd_pos = &out_sigs.cmd_joint_positions.j1; break;
            case 1: cmd_pos = &out_sigs.cmd_joint_positions.j2; break;
            case 2:
                cmd_pos = &out_sigs.cmd_joint_positions.j3;
                relative_cmd_pos = &out_sigs.cmd_joint_positions.j2;
                break;
            case 3: cmd_pos = &out_sigs.cmd_joint_positions.j4; break;
            case 4: cmd_pos = &out_sigs.cmd_joint_positions.j5; break;
            case 5: cmd_pos = &out_sigs.cmd_joint_positions.j6; break;
        }

        if(joint_limits[i].min_pos != joint_limits[i].max_pos){ // if the joint is not continuous

            if(*cmd_pos + *relative_cmd_pos < joint_limits[i].min_pos - tol){
                *cmd_pos = joint_limits[i].min_pos - *relative_cmd_pos;
                modified = true;
            }
            else if(*cmd_pos + *relative_cmd_pos > joint_limits[i].max_pos + tol){
                *cmd_pos = joint_limits[i].max_pos - *relative_cmd_pos;
                modified = true;
            }
        }
    }

    return modified; // return true if any joint was modified
}


void kins::update_forward_kinematics_outputs(){
    // convert joint positions to cartesian positions
    
    // use floats for effeciency
    std::array<float,6> joint_angles = {
        (float)in_sigs.fbk_joint_positions.j1,
        (float)in_sigs.fbk_joint_positions.j2,
        (float)in_sigs.fbk_joint_positions.j3,
        (float)in_sigs.fbk_joint_positions.j4,
        (float)in_sigs.fbk_joint_positions.j5,
        (float)in_sigs.fbk_joint_positions.j6
    };

    joint_angles[2] += joint_angles[1]; // j3 is relative to j2, so we need to add them together

    // invert j3, j4, j5 to match the robot's joint directions
    joint_angles[2] = -joint_angles[2];
    joint_angles[3] = -joint_angles[3];
    joint_angles[4] = -joint_angles[4];

    // wrap angles to [-pi, pi]
    for(auto& angle : joint_angles){
        angle = fmod(angle + M_PI, 2.0f * M_PI) - M_PI; // wrap to [-pi, pi]
    }

    // get cartesian position and rotation matrix
    std::array<float,3> pos_out;
    std::array<float,9> rot_out;
    ik_solver.forwardKinematicsF32(joint_angles, pos_out, rot_out);
    
    out_sigs.fbk_cartesian_positions.x = pos_out[0];
    out_sigs.fbk_cartesian_positions.y = pos_out[1];
    out_sigs.fbk_cartesian_positions.z = pos_out[2];

    std::array<float,3> euler_out;
    rot_to_euler(&rot_out, &euler_out);
    out_sigs.fbk_cartesian_positions.xangle = euler_out[0];
    out_sigs.fbk_cartesian_positions.yangle = euler_out[1];
    out_sigs.fbk_cartesian_positions.zangle = euler_out[2];
    return;
}

bool kins::validate_kins_solution(const std::array<float,6>* joint_angles_, const cartesian_positions* cartesian_pos, const float pos_tol, const float angle_tol){
    
    std::array<float,6> joint_angles = {
        (*joint_angles_)[0],
        (*joint_angles_)[1],
        (*joint_angles_)[2],
        (*joint_angles_)[3],
        (*joint_angles_)[4],
        (*joint_angles_)[5]
    };
    
    joint_angles[2] += joint_angles[1]; // j3 is relative to j2, so we need to add them together

    // invert j3, j4, j5 to match the robot's joint directions
    joint_angles[2] = -joint_angles[2];
    joint_angles[3] = -joint_angles[3];
    joint_angles[4] = -joint_angles[4];

    // wrap angles to [-pi, pi]
    for(auto& angle : joint_angles){
        angle = fmod(angle + M_PI, 2.0f * M_PI) - M_PI; // wrap to [-pi, pi]
    }

    // get cartesian position and rotation matrix
    std::array<float,3> pos_out;
    std::array<float,9> rot_out;
    ik_solver.forwardKinematicsF32(joint_angles, pos_out, rot_out);

    cartesian_positions pos_out_cartesian;
    pos_out_cartesian.x = pos_out[0];
    pos_out_cartesian.y = pos_out[1];
    pos_out_cartesian.z = pos_out[2];

    std::array<float,3> euler_out;
    rot_to_euler(&rot_out, &euler_out);
    pos_out_cartesian.xangle = euler_out[0];
    pos_out_cartesian.yangle = euler_out[1];
    pos_out_cartesian.zangle = euler_out[2];

    bool valid = true;

    // check position error
    if(fabs(pos_out_cartesian.x - cartesian_pos->x) > pos_tol ||
       fabs(pos_out_cartesian.y - cartesian_pos->y) > pos_tol ||
       fabs(pos_out_cartesian.z - cartesian_pos->z) > pos_tol){
        valid = false;
    }

    // check angle error (shortest angle distance)
    auto shortest_angle_dist = [](float a, float b) -> float {
        float diff = a - b;
        diff = fmod(diff + M_PI, 2.0f * M_PI);
        if (diff < 0) diff += 2.0f * M_PI;
        return diff - M_PI;
    };
    float angle_error_x = shortest_angle_dist(pos_out_cartesian.xangle, cartesian_pos->xangle);
    float angle_error_y = shortest_angle_dist(pos_out_cartesian.yangle, cartesian_pos->yangle);
    float angle_error_z = shortest_angle_dist(pos_out_cartesian.zangle, cartesian_pos->zangle);
    if(fabs(angle_error_x) > angle_tol ||
       fabs(angle_error_y) > angle_tol ||
       fabs(angle_error_z) > angle_tol){
        valid = false;
    }
    
    return valid;

}

void kins::jog_cartesian(uint8_t jog_axis, float jog_vel_, float speed_override){
    // jog a cartesian axis by a certain velocity (based on the joint max velocity)
    // jog_axis is 0-2 for x, y, z
    // jog_axis is 3-5 for xangle, yangle, zangle
    // speed_override is a multiplier for the jog speed

    if(jog_axis > 5) return; // invalid axis

    // calculate the desired jog speed
    float jog_vel = cartesian_limits[jog_axis].max_vel * jog_vel_ * speed_override * max_jog_speed;

    // apply acceleration limits
    if(last_cartesian_jog_vel != jog_vel){
        double desired_vel_delta = jog_vel - last_cartesian_jog_vel;
        double max_vel_delta = cartesian_limits[jog_axis].max_acc * time_step; // max velocity change in this time step
        if(fabs(desired_vel_delta) > max_vel_delta){
            // limit the jog speed to the max velocity change
            last_cartesian_jog_vel += copysign(max_vel_delta, desired_vel_delta);
        }
        else {
            last_cartesian_jog_vel = jog_vel; // set the last jog vel to the new jog vel
        }
        jog_vel = last_cartesian_jog_vel; // use the limited jog vel for the rest of the function
    }


    jog_vel *= time_step; // convert to position change in this time step

    // update the commanded cartesian position based on the jog speed
    if(jog_axis < 3){ // x, y, z axes
        switch(jog_axis){
            case 0:
                last_cmd_cartesian_positions.x += jog_vel;
                break;
            case 1:
                last_cmd_cartesian_positions.y += jog_vel;
                break;
            case 2:
                last_cmd_cartesian_positions.z += jog_vel;
                break;
        }
    }
    else{   // xangle, yangle, zangle axes
        
        // use quaternions to rotate, so the rotations are intuitive

        quat_rot q = quatFromZYX(
            last_cmd_cartesian_positions.zangle,
            last_cmd_cartesian_positions.yangle,
            last_cmd_cartesian_positions.xangle
        );

        float h = jog_vel * 0.5f;
        float c = cosf(h), s = sinf(h);
        quat_rot jog_rot;
        switch(jog_axis) {
            case 3: jog_rot = quat_rot{c,  0, 0, s}; break; // xangle
            case 4: jog_rot = quat_rot{c,  0, s, 0}; break; // yangle
            case 5: jog_rot = quat_rot{c,  s, 0, 0}; break; // zangle
            default: jog_rot = quat_rot{1,0,0,0}; break; // no rotation
        }

        // apply the jog rotation to the current quaternion
        q = multiply(q, jog_rot);

        // convert the quaternion back to euler angles
        toEulerZYX(&q, 
            &last_cmd_cartesian_positions.zangle, 
            &last_cmd_cartesian_positions.yangle, 
            &last_cmd_cartesian_positions.xangle
        );
        
    }

    inverse_kins(); // calculate the joint positions based on the new cartesian positions

}

void kins::jog_mouse(){
    std::array<float,3> mouse_pos_delta = {
        in_sigs.cmd_cartesian_positions.x - starting_mouse_positions.x,
        in_sigs.cmd_cartesian_positions.y - starting_mouse_positions.y,
        in_sigs.cmd_cartesian_positions.z - starting_mouse_positions.z
    };
    

    // invert x and so they match mouse orientation
    // mouse_pos_delta[0] = -mouse_pos_delta[0];
    // mouse_pos_delta[1] = -mouse_pos_delta[1];

    quat_rot q_mouse_cur = quatFromZYX(in_sigs.cmd_cartesian_positions.zangle, in_sigs.cmd_cartesian_positions.yangle, in_sigs.cmd_cartesian_positions.xangle);
    quat_rot q_delta = multiply(inverse(starting_mouse_quat), q_mouse_cur);
    quat_rot q_robot_new = multiply(starting_robot_quat, q_delta);

    quat_smoothing_filter = slerp(quat_smoothing_filter, q_robot_new, slerp_a);

    float newYaw, newPitch, newRoll;
    toEulerZYX(&quat_smoothing_filter, &newYaw, &newPitch, &newRoll);

    std::array<float,3> mouse_angles_delta = {
        newRoll - starting_robot_positions.xangle,
        newPitch - starting_robot_positions.yangle,
        newYaw - starting_robot_positions.zangle
    };
    
    // filter the position delta for smoothing

    for(int i = 0; i < 3; i++){
        float smooth = pos_smoothing_filters[i].update(mouse_pos_delta[i], 0.001f);   // TODO: use the time step from the system
        mouse_pos_delta[i] = smooth; // update the mouse position delta with the smoothed value
        // mouse_pos_delta[i] = axis_filters[i].update(smooth, 0.001f);
    }

    last_cmd_cartesian_positions.x = starting_robot_positions.x + mouse_pos_delta[0];
    last_cmd_cartesian_positions.y = starting_robot_positions.y + mouse_pos_delta[1];
    last_cmd_cartesian_positions.z = starting_robot_positions.z + mouse_pos_delta[2];
    last_cmd_cartesian_positions.xangle = starting_robot_positions.xangle + mouse_angles_delta[0];
    last_cmd_cartesian_positions.yangle = starting_robot_positions.yangle + mouse_angles_delta[1];
    last_cmd_cartesian_positions.zangle = starting_robot_positions.zangle + mouse_angles_delta[2];

    // absolute angles
    // last_cmd_cartesian_positions.xangle = in_sigs.cmd_cartesian_positions.xangle;
    // last_cmd_cartesian_positions.yangle = in_sigs.cmd_cartesian_positions.yangle;
    // last_cmd_cartesian_positions.zangle = in_sigs.cmd_cartesian_positions.zangle;

    inverse_kins(); // calculate the joint positions based on the new cartesian positions

}

void kins::inverse_kins(){

    std::array<float,3> target_pos = {
        last_cmd_cartesian_positions.x,
        last_cmd_cartesian_positions.y,
        last_cmd_cartesian_positions.z
    };
    std::array<float,3> target_euler = {
        last_cmd_cartesian_positions.xangle,
        last_cmd_cartesian_positions.yangle,
        last_cmd_cartesian_positions.zangle
    };

    // wrap angles to [0, 2pi]
    for(auto& angle : target_euler){
        angle = fmod(angle + 2.0f * M_PI, 2.0f * M_PI);
    }

    std::array<float,6> current_joint_angles = {    // use commanded joint positions as the current joint angles
        (float)out_sigs.cmd_joint_positions.j1,
        (float)out_sigs.cmd_joint_positions.j2,
        (float)out_sigs.cmd_joint_positions.j3,
        (float)out_sigs.cmd_joint_positions.j4,
        (float)out_sigs.cmd_joint_positions.j5,
        (float)out_sigs.cmd_joint_positions.j6
    };

    // std::array<float,6> current_joint_angles = {    // use actual fbk positions as the current joint angles
    //     (float)in_sigs.fbk_joint_positions.j1,
    //     (float)in_sigs.fbk_joint_positions.j2,
    //     (float)in_sigs.fbk_joint_positions.j3,
    //     (float)in_sigs.fbk_joint_positions.j4,
    //     (float)in_sigs.fbk_joint_positions.j5,
    //     (float)in_sigs.fbk_joint_positions.j6
    // };

    // get correct j3 angle
    current_joint_angles[2] += current_joint_angles[1]; // j3 is relative to j2, so we need to add them together

    // wrap angles to [0 - 2pi]
    for(auto& angle : current_joint_angles){
        angle = fmod(angle + 2.0f * M_PI, 2.0f * M_PI);
    }

    // invert j3, j4, j5 to match the robot's joint directions
    current_joint_angles[2] = -current_joint_angles[2];
    current_joint_angles[3] = -current_joint_angles[3];
    current_joint_angles[4] = -current_joint_angles[4];

    std::array<float, 6> cartTwist = {0.0, 0.0, 0.0, 0.0, 0.0, 0.0}; // No twist
    std::array<float, 6> outSolution;
    std::array<float, 6> outVelocities;
    InverseKinematics::IKErrorDetail detail;

    InverseKinematics::ErrorCode ec = ik_solver.solve(
        target_pos, target_euler, current_joint_angles,
        cartTwist, outSolution, outVelocities, detail, 50, 1e-4f);

    if(ec != InverseKinematics::ErrorCode::Success){
        // TODO: handle error
    }
    else{
        // update the commanded joint positions based on the solution

        // TODO: limit for pos, vel, and acc here

        // invert j3, j4, j5 to match the robot's joint directions
        outSolution[2] = -outSolution[2];
        outSolution[3] = -outSolution[3];
        outSolution[4] = -outSolution[4];

        // un-offset j3
        outSolution[2] -= outSolution[1];

        // update the last commanded cartesian positions
        last_cmd_cartesian_positions.x = target_pos[0];
        last_cmd_cartesian_positions.y = target_pos[1];
        last_cmd_cartesian_positions.z = target_pos[2];
        last_cmd_cartesian_positions.xangle = target_euler[0];
        last_cmd_cartesian_positions.yangle = target_euler[1];
        last_cmd_cartesian_positions.zangle = target_euler[2];

        bool valid = validate_kins_solution(&outSolution, &last_cmd_cartesian_positions, 0.01f, 0.01f);

        if(valid){
            set_closest_joint_positions(&outSolution);
        }
        else{
            volatile bool t; // for debugging breakpoint
            t = true; // set a breakpoint here to debug invalid kins solution
        }
    }
}

void kins::set_closest_joint_positions(std::array<float,6>* joint_angles){
    // find the closest final joint position to get to the desired -pi - pi angle
    for(size_t i = 0; i < 6; i++){
        double* actual_joint_pos = nullptr;
        switch(i){
            case 0: actual_joint_pos = &out_sigs.cmd_joint_positions.j1; break;
            case 1: actual_joint_pos = &out_sigs.cmd_joint_positions.j2; break;
            case 2: actual_joint_pos = &out_sigs.cmd_joint_positions.j3; break;
            case 3: actual_joint_pos = &out_sigs.cmd_joint_positions.j4; break;
            case 4: actual_joint_pos = &out_sigs.cmd_joint_positions.j5; break;
            case 5: actual_joint_pos = &out_sigs.cmd_joint_positions.j6; break;
        }

        *actual_joint_pos = wrapNearest(*actual_joint_pos, (double)(*joint_angles)[i]);
        // *actual_joint_pos = (double)(*joint_angles)[i]; // no wrapping for now, just set the joint position to the desired angle

    }
}

double kins::wrapNearest(double absAngle, double cmdAngle){
    const double twoPi = 2.0 * M_PI;

    // 1) first, wrap cmdAngle into [–π, π]
    cmdAngle = fmod(cmdAngle, twoPi);
    if (cmdAngle >  M_PI) cmdAngle -= twoPi;
    if (cmdAngle < -M_PI) cmdAngle += twoPi;

    // 2) figure out how many 2π-turns to add to cmdAngle
    double k = round((absAngle - cmdAngle) / twoPi);

    // 3) shift cmdAngle by that many turns
    return cmdAngle + k * twoPi;
}

void kins::jog_joint(uint8_t jog_axis, float jog_vel_, float speed_override){
    // jog a joint by a certain velocity (based on the joint max velocity)
    // jog_axis is 0-5 for j1-j6
    // speed_override is a multiplier for the jog speed

    if(jog_axis > 5) return; // invalid axis

    if(jog_vel_ > .05){
        volatile bool t;    // for debugging breakpoint
        t = true; // set a breakpoint here to debug jog speed
    }

    // calculate the desired jog speed
    float jog_vel = joint_limits[jog_axis].max_vel * jog_vel_ * speed_override * max_jog_speed;

    // apply acceleration limits
    if(last_joint_jog_vel != jog_vel){
        double desired_vel_delta = jog_vel - last_joint_jog_vel;
        double max_vel_delta = joint_limits[jog_axis].max_acc * time_step; // max velocity change in this time step
        if(fabs(desired_vel_delta) > max_vel_delta){
            // limit the jog speed to the max velocity change
            last_joint_jog_vel += copysign(max_vel_delta, desired_vel_delta);
        }
        else {
            last_joint_jog_vel = jog_vel; // set the last jog vel to the new jog vel
        }
        jog_vel = last_joint_jog_vel; // use the limited jog vel for the rest of the function
    }

    // limit the jog speed to the allowed positive and negative velocities
    if(copysign(1.0f, jog_vel) < 0){
        // jog speed is negative, so we need to limit it to the allowed negative velocity
        jog_vel = fmax(jog_vel, -joint_distances_to_limit[jog_axis].allowed_negative_vel);
    } else {
        // positive jog speed, so we need to limit it to the allowed positive velocity
        jog_vel = fmin(jog_vel, joint_distances_to_limit[jog_axis].allowed_positive_vel);
    }

    // update the commanded joint position based on the jog speed
    switch(jog_axis){
        case 0:
            out_sigs.cmd_joint_positions.j1 += jog_vel * time_step;
            break;
        case 1:
            out_sigs.cmd_joint_positions.j2 += jog_vel * time_step;
            break;
        case 2:
            out_sigs.cmd_joint_positions.j3 += jog_vel * time_step;
            break;
        case 3:
            out_sigs.cmd_joint_positions.j4 += jog_vel * time_step;
            break;
        case 4:
            out_sigs.cmd_joint_positions.j5 += jog_vel * time_step;
            break;
        case 5:
            out_sigs.cmd_joint_positions.j6 += jog_vel * time_step;
            break;
    }

}

void kins::rot_to_euler(const std::array<float,9>* rot, std::array<float,3>* euler){
    // convert rotation matrix to euler angles
    float sy = sqrt((*rot)[0] * (*rot)[0] + (*rot)[3] * (*rot)[3]);
    bool singular = sy < 1e-6;

    if (!singular) {
        (*euler)[0] = atan2((*rot)[7], (*rot)[8]);
        (*euler)[1] = atan2(-(*rot)[6], sy);
        (*euler)[2] = atan2((*rot)[3], (*rot)[0]);
    } else {
        (*euler)[0] = atan2(-(*rot)[5], (*rot)[4]);
        (*euler)[1] = atan2(-(*rot)[6], sy);
        (*euler)[2] = 0;
    }

    // flip x and z
    float temp = (*euler)[0];
    (*euler)[0] = (*euler)[2];
    (*euler)[2] = temp;
}

void kins::update_joint_distances_to_limit(){
    // update how far in each direction each joint is from a limit
    // uses final commanded joint positions

    // TODO: this is hardcoded for the UP6 joint congiguration, should be configurable eventually

    // j4 and 6 are continuous
    joint_distances_to_limit[3].infinite = true;
    joint_distances_to_limit[5].infinite = true;

    // j1
    joint_distances_to_limit[0].to_min = out_sigs.cmd_joint_positions.j1 - joint_limits[0].min_pos;
    joint_distances_to_limit[0].to_max = joint_limits[0].max_pos - out_sigs.cmd_joint_positions.j1;

    // j2 + j3
    // j2 and 3 limits are dependent on eachother, so we need to calculate them together
    double j2_cmd = out_sigs.cmd_joint_positions.j2;
    double j3_cmd = out_sigs.cmd_joint_positions.j3;
    double j3_relative_to_j2 = j3_cmd + j2_cmd; // relative angle between j2 and j3

    // j2 only
    joint_distances_to_limit[1].to_min = j2_cmd - joint_limits[1].min_pos;
    joint_distances_to_limit[1].to_max = joint_limits[1].max_pos - j2_cmd;

    // j2 relative to j3
    joint_distances_to_limit[1].to_min = fmin(joint_distances_to_limit[1].to_min, j3_relative_to_j2 - joint_limits[2].min_pos);
    joint_distances_to_limit[1].to_max = fmin(joint_distances_to_limit[1].to_max, joint_limits[2].max_pos - j3_relative_to_j2);

    // j3 only
    joint_distances_to_limit[2].to_min = j3_relative_to_j2 - joint_limits[2].min_pos;
    joint_distances_to_limit[2].to_max = joint_limits[2].max_pos - j3_relative_to_j2;

    // j4
    // infinite

    // j5
    joint_distances_to_limit[4].to_min = out_sigs.cmd_joint_positions.j5 - joint_limits[4].min_pos;
    joint_distances_to_limit[4].to_max = joint_limits[4].max_pos - out_sigs.cmd_joint_positions.j5;

    // j6
    // infinite


    // calculate allowed speeds based on the distances to the limits
    for(int i = 0; i < 6; i++){
        float acc = joint_limits[i].max_acc;
        float vel = joint_limits[i].max_vel;

        if(joint_distances_to_limit[i].infinite){
            joint_distances_to_limit[i].allowed_positive_vel = vel; // infinite joint, can move at full speed
            joint_distances_to_limit[i].allowed_negative_vel = vel; // infinite joint, can move at full speed
        } else {

            // calculate the distance we need to stop from full speed
            float stop_distance = (vel * vel) / (2.0 * acc); // d = v^2 / (2 * a)

            float pos_speed = 0.0f;
            float neg_speed = 0.0f;

            // calculate the velocity based on the distance to the limit
            if(joint_distances_to_limit[i].to_min > stop_distance){
                // not close to limit
                neg_speed = vel; // we can move at full speed
            } else if(joint_distances_to_limit[i].to_min < 0){
                // outside the limit, we cannot move in this direction
                neg_speed = 0.0f;
            } else {
                // close to limit, we need to slow down
                neg_speed = sqrt(2.0 * acc * joint_distances_to_limit[i].to_min); // v = sqrt(2 * a * d)
            }

            if(joint_distances_to_limit[i].to_max > stop_distance){
                // not close to limit
                pos_speed = vel; // we can move at full speed
            } else if(joint_distances_to_limit[i].to_max < 0){
                // outside the limit, we cannot move in this direction
                pos_speed = 0.0f;
            } else {
                // close to limit, we need to slow down
                pos_speed = sqrt(2.0 * acc * joint_distances_to_limit[i].to_max); // v = sqrt(2 * a * d)
            }
            joint_distances_to_limit[i].allowed_positive_vel = pos_speed;
            joint_distances_to_limit[i].allowed_negative_vel = neg_speed;
        }
    }

}

static Node_Registrar<kins> node_registrar_kins("kins");
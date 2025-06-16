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
    outputs.emplace("j1_cmd_pos", output(io_type::DOUBLE, &out_sigs.cmd_joint_positions.j1, &execution_number));
    outputs.emplace("j2_cmd_pos", output(io_type::DOUBLE, &out_sigs.cmd_joint_positions.j2, &execution_number));
    outputs.emplace("j3_cmd_pos", output(io_type::DOUBLE, &out_sigs.cmd_joint_positions.j3, &execution_number));
    outputs.emplace("j4_cmd_pos", output(io_type::DOUBLE, &out_sigs.cmd_joint_positions.j4, &execution_number));
    outputs.emplace("j5_cmd_pos", output(io_type::DOUBLE, &out_sigs.cmd_joint_positions.j5, &execution_number));
    outputs.emplace("j6_cmd_pos", output(io_type::DOUBLE, &out_sigs.cmd_joint_positions.j6, &execution_number));

    outputs.emplace("x_fbk_pos", output(io_type::FLOAT, &out_sigs.fbk_cartesian_positions.x, &execution_number));
    outputs.emplace("y_fbk_pos", output(io_type::FLOAT, &out_sigs.fbk_cartesian_positions.y, &execution_number));
    outputs.emplace("z_fbk_pos", output(io_type::FLOAT, &out_sigs.fbk_cartesian_positions.z, &execution_number));
    outputs.emplace("xangle_fbk_pos", output(io_type::FLOAT, &out_sigs.fbk_cartesian_positions.xangle, &execution_number));
    outputs.emplace("yangle_fbk_pos", output(io_type::FLOAT, &out_sigs.fbk_cartesian_positions.yangle, &execution_number));
    outputs.emplace("zangle_fbk_pos", output(io_type::FLOAT, &out_sigs.fbk_cartesian_positions.zangle, &execution_number));
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


    switch(in_sigs.control_mode){
        case control_modes::JOG:
            // update joint positions based on jog inputs
            if(in_sigs.jog_mode == jog_modes::JOINT){
                // jog in joint space
                jog_joint(in_sigs.jog_axis_select, in_sigs.jog_vel, in_sigs.speed_override);
            }
            break;
    }

    // reset sets cmds to the current fbk positions
    if(in_sigs.reset){
        out_sigs.cmd_joint_positions = in_sigs.fbk_joint_positions;
        return 0;
    }

    return 0; // no errors
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

void kins::jog_joint(uint8_t jog_axis, float jog_vel_, float speed_override){
    // jog a joint by a certain velocity (based on the joint max velocity)
    // jog_axis is 0-5 for j1-j6
    // speed_override is a multiplier for the jog speed

    if(jog_axis > 5) return; // invalid axis

    // calculate the desired jog speed
    float jog_vel = joint_limits[jog_axis].max_vel * jog_vel_ * speed_override * max_jog_speed;

    // apply acceleration limits
    if(last_jog_vel != jog_vel){
        double desired_vel_delta = jog_vel - last_jog_vel;
        double max_vel_delta = joint_limits[jog_axis].max_acc * time_step; // max velocity change in this time step
        if(fabs(desired_vel_delta) > max_vel_delta){
            // limit the jog speed to the max velocity change
            last_jog_vel += copysign(max_vel_delta, desired_vel_delta);
        }
        else {
            last_jog_vel = jog_vel; // set the last jog vel to the new jog vel
        }
        jog_vel = last_jog_vel; // use the limited jog vel for the rest of the function
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
}

void kins::update_joint_distances_to_limit(){
    // update how far in each direction each joint is from a limit
    // uses final commanded joint positions

    // TODO: this is hardcoded for the UP6 joint congiguration, should be configurable eventually

    // j4 and 6 are continuous
    joint_distances_to_limit[3].infinite = true;
    joint_distances_to_limit[5].infinite = true;

    // j1
    joint_distances_to_limit[0].to_min = in_sigs.cmd_joint_positions.j1 - joint_limits[0].min_pos;
    joint_distances_to_limit[0].to_max = joint_limits[0].max_pos - in_sigs.cmd_joint_positions.j1;

    // j2 + j3
    // j2 and 3 limits are dependent on eachother, so we need to calculate them together
    double j2_cmd = in_sigs.cmd_joint_positions.j2;
    double j3_cmd = in_sigs.cmd_joint_positions.j3;
    double j3_relative_to_j2 = j3_cmd + j2_cmd; // relative angle between j2 and j3

    // j2 only
    joint_distances_to_limit[1].to_min = j2_cmd - joint_limits[1].min_pos;
    joint_distances_to_limit[1].to_max = joint_limits[1].max_pos - j2_cmd;

    // j2 relative to j3
    joint_distances_to_limit[1].to_min = fmin(joint_distances_to_limit[1].to_min, j3_relative_to_j2 - joint_limits[2].min_pos);
    joint_distances_to_limit[1].to_max = fmin(joint_distances_to_limit[1].to_max, joint_limits[2].max_pos - j3_relative_to_j2);

    // j3 only
    joint_distances_to_limit[2].to_min = j3_cmd - joint_limits[2].min_pos;
    joint_distances_to_limit[2].to_max = joint_limits[2].max_pos - j3_cmd;

    // j4
    // infinite

    // j5
    joint_distances_to_limit[4].to_min = in_sigs.cmd_joint_positions.j5 - joint_limits[4].min_pos;
    joint_distances_to_limit[4].to_max = joint_limits[4].max_pos - in_sigs.cmd_joint_positions.j5;

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
                pos_speed = vel; // we can move at full speed
            } else {
                // close to limit, we need to slow down
                pos_speed = sqrt(2.0 * acc * joint_distances_to_limit[i].to_min); // v = sqrt(2 * a * d)
            }
            if(joint_distances_to_limit[i].to_max > stop_distance){
                // not close to limit
                neg_speed = vel; // we can move at full speed
            } else {
                // close to limit, we need to slow down
                neg_speed = sqrt(2.0 * acc * joint_distances_to_limit[i].to_max); // v = sqrt(2 * a * d)
            }
            joint_distances_to_limit[i].allowed_positive_vel = pos_speed;
            joint_distances_to_limit[i].allowed_negative_vel = neg_speed;
        }
    }

}

static Node_Registrar<kins> node_registrar_kins("kins");
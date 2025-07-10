#include "mouse.h"
#include <cmath>

mouse::mouse(){
    // create all IO
    create_inputs();
    create_outputs();
}

void mouse::create_inputs(){

    inputs.emplace("j1_fbk_pos", input(io_type::INT32, nullptr));
    inputs.emplace("j2_fbk_pos", input(io_type::INT32, nullptr));
    inputs.emplace("j3_fbk_pos", input(io_type::INT32, nullptr));
    inputs.emplace("j4_fbk_pos", input(io_type::INT32, nullptr));
    inputs.emplace("j5_fbk_pos", input(io_type::INT32, nullptr));
    inputs.emplace("j6_fbk_pos", input(io_type::INT32, nullptr));
    in_sig_ptrs.j1_fbk_pos = &inputs["j1_fbk_pos"];
    in_sig_ptrs.j2_fbk_pos = &inputs["j2_fbk_pos"];
    in_sig_ptrs.j3_fbk_pos = &inputs["j3_fbk_pos"];
    in_sig_ptrs.j4_fbk_pos = &inputs["j4_fbk_pos"];
    in_sig_ptrs.j5_fbk_pos = &inputs["j5_fbk_pos"];
    in_sig_ptrs.j6_fbk_pos = &inputs["j6_fbk_pos"];

}

void mouse::create_outputs(){

    outputs.emplace("x_fbk_pos", output(io_type::FLOAT, &(out_sigs.fbk_cartesian_positions.x), &execution_number));
    outputs.emplace("y_fbk_pos", output(io_type::FLOAT, &(out_sigs.fbk_cartesian_positions.y), &execution_number));
    outputs.emplace("z_fbk_pos", output(io_type::FLOAT, &(out_sigs.fbk_cartesian_positions.z), &execution_number));
    outputs.emplace("xangle_fbk_pos", output(io_type::FLOAT, &(out_sigs.fbk_cartesian_positions.xangle), &execution_number));
    outputs.emplace("yangle_fbk_pos", output(io_type::FLOAT, &(out_sigs.fbk_cartesian_positions.yangle), &execution_number));
    outputs.emplace("zangle_fbk_pos", output(io_type::FLOAT, &(out_sigs.fbk_cartesian_positions.zangle), &execution_number));
}

void mouse::update_inputs(){
    std::array<float*,6> joint_angles = {
        (float*)&in_sigs.fbk_joint_positions.j1,
        (float*)&in_sigs.fbk_joint_positions.j2,
        (float*)&in_sigs.fbk_joint_positions.j3,
        (float*)&in_sigs.fbk_joint_positions.j4,
        (float*)&in_sigs.fbk_joint_positions.j5,
        (float*)&in_sigs.fbk_joint_positions.j6
    };

    std::array<int32_t,6> in_cnts = {
        *(int32_t*)in_sig_ptrs.j1_fbk_pos->data_pointer,
        *(int32_t*)in_sig_ptrs.j2_fbk_pos->data_pointer,
        *(int32_t*)in_sig_ptrs.j3_fbk_pos->data_pointer,
        *(int32_t*)in_sig_ptrs.j4_fbk_pos->data_pointer,
        *(int32_t*)in_sig_ptrs.j5_fbk_pos->data_pointer,
        *(int32_t*)in_sig_ptrs.j6_fbk_pos->data_pointer
    };

    for(int i = 0; i < 6; i++){

        // convert to turns
        float turns = (float)(in_cnts[i] - joint_parameters[i].home_pos) / (float)joint_parameters[i].count_per_rev; // convert to turns

        // convert to radians
        *joint_angles[i] = turns * 2.0f * M_PI; // convert to radians
    }

    *joint_angles[2] -= *joint_angles[1]; // j3 is relative to j2, so we need to subtract j2 from j3

}

uint32_t mouse::run(){
    // update inputs
    update_inputs();

    // update forward kinematics outputs (so we always have the current cartesian position)
    update_forward_kinematics_outputs();

    return 0; // no errors
}


void mouse::update_forward_kinematics_outputs(){
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

void mouse::rot_to_euler(const std::array<float,9>* rot, std::array<float,3>* euler){
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

static Node_Registrar<mouse> node_registrar_mouse("mouse");
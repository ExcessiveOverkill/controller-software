#include "kins.h"
#include <cmath>

kins::kins(){
    // create all IO
    create_inputs();
    create_outputs();

    // add a default tool (no offset)
    std::array<float,3> default_tool_pos = {0.0f, 0.0f, 0.0f};
    std::array<float,3> default_tool_eul = {0.0f, 0.0f, 0.0f};
    tool default_tool;
    default_tool.position = default_tool_pos;
    default_tool.orientation = default_tool_eul;
    add_tool(default_tool);
    select_tool(0); // select the default tool
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

    inputs.emplace("tool_select", input(io_type::UINT8, nullptr));
    in_sig_ptrs.tool_select = &inputs["tool_select"];

    inputs.emplace("reset", input(io_type::BOOL, nullptr));
    in_sig_ptrs.reset = &inputs["reset"];

    inputs.emplace("collision_detected", input(io_type::BOOL, nullptr));
    in_sig_ptrs.collision_detected = &inputs["collision_detected"];
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
    outputs.emplace("at_cmd_pos", output(io_type::BOOL, &(out_sigs.at_cmd_pos), &execution_number));
    outputs.emplace("backup_buffer_exhausted", output(io_type::BOOL, &(out_sigs.backup_buffer_exhausted), &execution_number));
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
    in_sigs.tool_select = *(uint8_t*)(in_sig_ptrs.tool_select->data_pointer);
    in_sigs.reset = *(bool*)(in_sig_ptrs.reset->data_pointer);
    in_sigs.collision_detected = *(bool*)(in_sig_ptrs.collision_detected->data_pointer);

    in_sigs.speed_override = fmax(0.001f, fmin(in_sigs.speed_override, 1.0f)); // clamp speed override to [0.001, 1]
}

uint32_t kins::run(){
    // update inputs
    update_inputs();

    if(!cmd_joint_positions_initialized){
        // out_sigs.cmd_joint_positions otherwise defaults to all-zero, which is essentially never
        // the robot's real joint configuration. move_joints()'s ramp "current", inverse_kins()'s IK
        // initial guess, and clamp_to_joint_limits()'s baseline all depend on this being correct
        // from cycle 1, not just after the first reset or JOG+JOINT jog.
        out_sigs.cmd_joint_positions = in_sigs.fbk_joint_positions;
        last_output_joint_positions = out_sigs.cmd_joint_positions;
        cmd_joint_positions_initialized = true;
    }

    if(in_sigs.tool_select != last_tool_select || in_sigs.reset){
        select_tool(in_sigs.tool_select);
        last_tool_select = in_sigs.tool_select;
    }

    // update forward kinematics outputs (so we always have the current cartesian position)
    update_forward_kinematics_outputs();

    if(in_sigs.control_mode != last_control_mode){
        // Control mode just changed (in either direction, e.g. JOG -> CARTESIAN, JOINT -> JOG,
        // etc.) -- invalidate any previously-stored synchronized-move target for BOTH joint and
        // cartesian moves, regardless of which mode set it. Without this, a "set joint"/"set
        // cartesian" command sent while in a *different* control mode (most commonly JOG) stays
        // latched in move_joint_cmd_positions/move_cartesian_cmd_positions, and the instant the
        // mode is switched into JOINT/CARTESIAN, move_joints()/move_cartesian() would immediately
        // start moving toward that stale target with no new command ever having been issued in
        // the now-active mode.
        //
        // Deliberately target_valid = FALSE here, not true: move_joints()/move_cartesian() both
        // early-return the instant their target_valid flag is false, WITHOUT calling
        // inverse_kins() at all -- that early-return is what keeps the IK solver from running
        // every single cycle when no real command has been issued yet in the new mode. Setting
        // target_valid = true instead (target := current position, so distance is zero) looks
        // equivalent at first glance, but it is not: it forces move_cartesian() to execute its
        // full ramp + inverse_kins() body every cycle, forever, from the moment the mode is
        // entered -- a large, unconditional new CPU cost at this 1kHz loop (time_step = 0.001f),
        // and if that solve ever fails to converge from whatever guess exists at that instant, it
        // burns the full 50-iteration cap every cycle indefinitely, which is exactly the "IK
        // taking way longer than it should, missing IRQs" regression this replaces.
        // (move_joint_cmd_positions/move_cartesian_cmd_positions are still snapped to the current
        // position below, purely so a "get" query reports the real current position rather than a
        // stale prior target -- they are not used as a ramp target while target_valid is false.)
        move_joint_cmd_positions = out_sigs.cmd_joint_positions;
        move_joint_target_valid = false;
        move_joint_last_vel.fill(0.0f);

        move_cartesian_cmd_positions = out_sigs.fbk_cartesian_positions;
        move_cartesian_target_valid = false;
        move_cartesian_last_vel.fill(0.0f);
        move_cartesian_last_ang_vel = 0.0f;
    }

    if(in_sigs.control_mode == control_modes::CARTESIAN && (last_control_mode != control_modes::CARTESIAN || in_sigs.reset)){
        // last_cmd_cartesian_positions is only kept in sync by inverse_kins() (called from
        // jog_cartesian/jog_mouse/move_cartesian); if we just arrived here from JOINT mode or JOG
        // mode it can be stale. Resync here, before update_cartesian_distances_to_limit() runs below,
        // so this cycle's velocity caps are computed from the correct position.
        last_cmd_cartesian_positions = out_sigs.fbk_cartesian_positions;
    }

    update_joint_distances_to_limit();
    update_cartesian_distances_to_limit();

    // ---- collision backup (retrace-path) edge detection & dispatch ----
    // Phrased directly off collision_backup_state (not just a raw last_collision_detected edge)
    // so a reassertion of collision_detected while still DECELERATING immediately re-arms
    // RETRACING -- a live collision signal always wins over an in-progress graceful stop.
    bool collision_rising = in_sigs.collision_detected && collision_backup_state != backup_state::RETRACING;
    bool collision_falling = !in_sigs.collision_detected && collision_backup_state == backup_state::RETRACING;

    if(collision_rising && cmd_joint_positions_initialized &&
       (in_sigs.control_mode == control_modes::JOINT || in_sigs.control_mode == control_modes::CARTESIAN)){
        start_collision_backup();
    }
    if(collision_falling){
        begin_backup_decel();
    }
    last_collision_detected = in_sigs.collision_detected;

    if(collision_backup_state == backup_state::RETRACING){
        // collision backup in progress -- overrides the normal control-mode dispatch entirely,
        // driving purely in joint space regardless of which control mode was active when
        // collision_detected first fired.
        run_backup_retrace();
    }
    else if(collision_backup_state == backup_state::DECELERATING){
        run_backup_decel();
    }
    else
    switch(in_sigs.control_mode){
        case control_modes::JOG:
            // update joint positions based on jog inputs
            // (tool selection and forward kinematics are already handled above, unconditionally)

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
        
        case control_modes::JOINT:
            // commanded joint positions are directly set by the API
            // accel, vel, and position limits are applied, speed is scaled
            // joints move in sync
            move_joints();
            break;

        case control_modes::CARTESIAN:
            // commanded cartesian positions are directly set by the API
            // accel, vel, and position limits are applied, speed is scaled
            // cartesian axes move in sync, IK solved every cycle via inverse_kins()
            move_cartesian();
            break;
    }
    last_control_mode = in_sigs.control_mode;

    if(in_sigs.reset){
        // reset the commanded joint positions to the current positions
        out_sigs.cmd_joint_positions = in_sigs.fbk_joint_positions;
        last_output_joint_positions = out_sigs.cmd_joint_positions;

        // reset overrides/cancels any in-progress collision backup, same "cancel and resync to
        // feedback" role reset already plays for normal moves above
        collision_backup_state = backup_state::IDLE;
        backup_last_vel.fill(0.0f);
    }

    bool on_joint_limit = clamp_to_joint_limits();  // clamp the commanded joint positions to the limits as a failsafe

    // record collision-backup history from the fully-finalized, clamped output for this cycle --
    // only while not itself backing up (don't record the retreating path while retracing it), and
    // only for JOINT/CARTESIAN moves (this feature does not apply to JOG).
    if(collision_backup_state == backup_state::IDLE && cmd_joint_positions_initialized &&
       (in_sigs.control_mode == control_modes::JOINT || in_sigs.control_mode == control_modes::CARTESIAN)){
        record_backup_point();
    }
    out_sigs.backup_buffer_exhausted = (collision_backup_state == backup_state::RETRACING && backup_count == 0);



    return 0; // no errors
}

bool kins::clamp_to_joint_limits(){
    // Failsafe backstop, applied after every control-mode dispatch (JOG/JOINT/CARTESIAN alike):
    // caps this cycle's per-joint commanded delta so it can never exceed what
    // joint_distances_to_limit[i].allowed_positive_vel/allowed_negative_vel already permit.
    // update_joint_distances_to_limit() (called once per cycle, before dispatch) is the single
    // source of truth for "which direction is safe to move a joint that is at/over a limit, and
    // how fast" -- jog_joint(), ramp_joints_to_target(), and apply_cartesian_joint_rate_limit()
    // all already cap their own output to these same values, so this function is a true no-op for
    // every currently-exercised control path (in particular: zero jog input -> zero delta -> never
    // exceeds either cap -> the joint holds, exactly as intended). It only ever engages for a delta
    // that exceeds the allowed velocity, in which case it caps the step (or fully holds, when
    // allowed_vel is 0 because the joint is already past that limit) -- it never teleports to the
    // limit boundary.

    if(bypass_joint_limits){
        last_output_joint_positions = out_sigs.cmd_joint_positions;
        return false; // if bypassing limits, do nothing
    }

    bool outside_limit = false;

    for(int i = 0; i < 6; i++){
        if(joint_distances_to_limit[i].infinite) continue; // continuous joint, nothing to enforce

        double *cmd_pos = nullptr;
        double last_cmd_pos = 0.0;
        switch(i){
            case 0: cmd_pos = &out_sigs.cmd_joint_positions.j1; last_cmd_pos = last_output_joint_positions.j1; break;
            case 1: cmd_pos = &out_sigs.cmd_joint_positions.j2; last_cmd_pos = last_output_joint_positions.j2; break;
            case 2: cmd_pos = &out_sigs.cmd_joint_positions.j3; last_cmd_pos = last_output_joint_positions.j3; break;
            case 3: cmd_pos = &out_sigs.cmd_joint_positions.j4; last_cmd_pos = last_output_joint_positions.j4; break;
            case 4: cmd_pos = &out_sigs.cmd_joint_positions.j5; last_cmd_pos = last_output_joint_positions.j5; break;
            case 5: cmd_pos = &out_sigs.cmd_joint_positions.j6; last_cmd_pos = last_output_joint_positions.j6; break;
        }

        double delta = *cmd_pos - last_cmd_pos;
        double max_pos_delta = (double)joint_distances_to_limit[i].allowed_positive_vel * (double)time_step;
        double max_neg_delta = (double)joint_distances_to_limit[i].allowed_negative_vel * (double)time_step;

        if(delta > max_pos_delta){
            *cmd_pos = last_cmd_pos + max_pos_delta; // hold (max_pos_delta==0 past the max limit) or cap the step
            outside_limit = true;
        }
        else if(delta < -max_neg_delta){
            *cmd_pos = last_cmd_pos - max_neg_delta; // hold (max_neg_delta==0 past the min limit) or cap the step
            outside_limit = true;
        }

        // flag genuine limit violations even on cycles where this cycle's delta didn't need
        // capping (e.g. an idle joint already sitting past a limit) -- preserves the original
        // return-value meaning ("is any joint currently outside its limit"), not just "did we
        // clamp this cycle".
        if(joint_distances_to_limit[i].to_min < 0.0 || joint_distances_to_limit[i].to_max < 0.0){
            outside_limit = true;
        }
    }

    last_output_joint_positions = out_sigs.cmd_joint_positions;

    return outside_limit; // true if any joint is currently outside its limit
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

    std::array<float,3> euler_out;
    rot_to_euler(&rot_out, &euler_out);

    std::array<float,3> pos_out_tcp;
    std::array<float,3> euler_out_tcp;

    apply_tool_offset(pos_out, euler_out, pos_out_tcp, euler_out_tcp);

    out_sigs.fbk_cartesian_positions.x = pos_out_tcp[0];
    out_sigs.fbk_cartesian_positions.y = pos_out_tcp[1];
    out_sigs.fbk_cartesian_positions.z = pos_out_tcp[2];

    // euler_out_tcp is [Z-rot, Y-rot, X-rot] (rot_to_euler()'s internal flip + matrixToPose()'s
    // convention) -- assign so .xangle genuinely means X-rotation and .zangle means Z-rotation,
    // matching jog_cartesian()/jog_mouse() and inverse_kins().
    // unwrap bounded angle axes onto their limit band's branch (matches decompose_cmd_angles());
    // continuous axes keep the principal value. index 3,4,5 == xangle,yangle,zangle.
    out_sigs.fbk_cartesian_positions.zangle = unwrap_cartesian_angle(5, euler_out_tcp[0]);
    out_sigs.fbk_cartesian_positions.yangle = unwrap_cartesian_angle(4, euler_out_tcp[1]);
    out_sigs.fbk_cartesian_positions.xangle = unwrap_cartesian_angle(3, euler_out_tcp[2]);
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

    // Run FK to get flange position and rotation matrix.
    std::array<float,3> pos_out;
    std::array<float,9> rot_out;
    ik_solver.forwardKinematicsF32(joint_angles, pos_out, rot_out);

    // Build a 4×4 rigid-body transform from the FK result so we can apply the tool offset as a
    // matrix multiplication, without going through an intermediate Euler decomposition.
    // rot_out is row-major: rot_out[r*3+c] = R[r][c].
    Mat4 T_flange{};
    for(int r = 0; r < 3; r++)
        for(int c = 0; c < 3; c++)
            T_flange.m[r][c] = rot_out[r * 3 + c];
    T_flange.m[0][3] = pos_out[0];
    T_flange.m[1][3] = pos_out[1];
    T_flange.m[2][3] = pos_out[2];
    T_flange.m[3][3] = 1.0f;

    // Apply tool offset (matrix form).  When no tool is active this is a pass-through.
    Mat4 T_tcp;
    if(current_tool != nullptr){
        Mat4 T_tool = poseToMatrix(current_tool->position, current_tool->orientation);
        T_tcp = mul(T_flange, T_tool);
    } else {
        T_tcp = T_flange;
    }

    bool valid = true;

    // --- Position check ---
    if(fabsf(T_tcp.m[0][3] - cartesian_pos->x) > pos_tol ||
       fabsf(T_tcp.m[1][3] - cartesian_pos->y) > pos_tol ||
       fabsf(T_tcp.m[2][3] - cartesian_pos->z) > pos_tol){
        valid = false;
    }

    // --- Orientation check via rotation matrices ---
    // We deliberately avoid comparing per-axis Euler angles here.  At yangle = ±π/2 (gimbal lock)
    // the Euler decomposition is degenerate: toEulerZYX (jog path) can produce xangle/zangle jumps
    // of ±π while the physical orientation is completely unchanged.  A per-axis Euler comparison
    // then sees a large error even though the joints are exactly right, causing a false failure.
    //
    // Rotation matrices have no such singularity.  Two matrices that represent the same physical
    // orientation are element-wise identical, so the Frobenius norm of their difference is zero
    // regardless of which Euler decomposition was used to produce them.
    //
    // Convention in this code:
    //   xangle = X-rotation (roll), yangle = Y-rotation (pitch), zangle = Z-rotation (yaw)
    //   poseToMatrix(pos, zyx) interprets zyx as [Z-rot, Y-rot, X-rot] → Rz·Ry·Rx
    // So {zangle, yangle, xangle} maps onto the [phi, theta, psi] argument of poseToMatrix.
    //
    // Threshold: ||R1 - R2||_F² ≈ 2·θ² for a single-axis error of θ, so the threshold is 2·angle_tol².
    const std::array<float,3> dummy_pos  = {0.0f, 0.0f, 0.0f};
    const std::array<float,3> target_zyx = {cartesian_pos->zangle, cartesian_pos->yangle, cartesian_pos->xangle};
    Mat4 T_target = poseToMatrix(dummy_pos, target_zyx);

    float frob_sq = 0.0f;
    for(int r = 0; r < 3; r++)
        for(int c = 0; c < 3; c++){
            float d = T_tcp.m[r][c] - T_target.m[r][c];
            frob_sq += d * d;
        }
    if(frob_sq > 2.0f * angle_tol * angle_tol){
        valid = false;
    }

    if(!valid){
        volatile int a = 0; // for debugging
        a = 1;
    }

    return valid;

}

void kins::jog_cartesian(uint8_t jog_axis, float jog_vel_, float speed_override){
    prev_cmd_cartesian_positions = last_cmd_cartesian_positions;
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

    // limit the jog speed to the allowed positive and negative velocities
    if(copysign(1.0f, jog_vel) < 0){
        // jog speed is negative, so we need to limit it to the allowed negative velocity
        jog_vel = fmax(jog_vel, -cartesian_distances_to_limit[jog_axis].allowed_negative_vel);
    } else {
        // positive jog speed, so we need to limit it to the allowed positive velocity
        jog_vel = fmin(jog_vel, cartesian_distances_to_limit[jog_axis].allowed_positive_vel);
    }

    if(jog_vel == 0.0f){    // no movement
        out_sigs.at_cmd_pos = true;
        return;
    }
    out_sigs.at_cmd_pos = false;


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

        // convert back to the stored triple, each angle axis unwrapped onto its limit band's
        // branch (periodicity-only; does not change the orientation handed to inverse_kins()).
        // Angular jog velocity is already throttled toward limits via
        // cartesian_distances_to_limit[jog_axis]; do NOT hard-clamp the Euler components here --
        // see the note in move_cartesian() (post-clamping a component makes the pose unreachable
        // and stalls IK).
        decompose_cmd_angles(q,
            last_cmd_cartesian_positions.zangle,
            last_cmd_cartesian_positions.yangle,
            last_cmd_cartesian_positions.xangle
        );
    }

    inverse_kins(); // calculate the joint positions based on the new cartesian positions

}

void kins::jog_mouse(){
    prev_cmd_cartesian_positions = last_cmd_cartesian_positions;
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

uint32_t kins::set_move_joint_cmd_positions(const joint_positions& new_target){

    if(collision_backup_state != backup_state::IDLE){
        // reject new move targets while a collision backup is retracing or decelerating -- the
        // robot must not resume forward motion until it has fully stopped and collision_detected
        // has cleared.
        return 1;
    }

    double prev[6] = {
        move_joint_cmd_positions.j1, move_joint_cmd_positions.j2, move_joint_cmd_positions.j3,
        move_joint_cmd_positions.j4, move_joint_cmd_positions.j5, move_joint_cmd_positions.j6
    };
    double req[6] = {
        new_target.j1, new_target.j2, new_target.j3,
        new_target.j4, new_target.j5, new_target.j6
    };

    // largest fraction of this call's requested move that every constrained channel can tolerate
    float s = 1.0f;
    auto constrain = [&](double base, double target_val, double min_pos, double max_pos){
        if(target_val >= min_pos && target_val <= max_pos) return; // already in range
        double delta = target_val - base;
        if(delta == 0.0) return;                                   // this channel isn't moving
        double limit = (target_val > max_pos) ? max_pos : min_pos;
        s = fminf(s, (float)std::clamp((limit - base) / delta, 0.0, 1.0));
    };

    if(!joint_distances_to_limit[0].infinite) constrain(prev[0], req[0], joint_limits[0].min_pos, joint_limits[0].max_pos);
    if(!joint_distances_to_limit[1].infinite) constrain(prev[1], req[1], joint_limits[1].min_pos, joint_limits[1].max_pos);
    // j3 is commanded relative to j2 (servo coupling); constrain the combined angle, same
    // convention as clamp_to_joint_limits()/update_joint_distances_to_limit().
    // TODO: verify this j2 and j3 limiting
    if(!joint_distances_to_limit[2].infinite) constrain(prev[1]+prev[2], req[1]+req[2], joint_limits[2].min_pos, joint_limits[2].max_pos);
    if(!joint_distances_to_limit[3].infinite) constrain(prev[3], req[3], joint_limits[3].min_pos, joint_limits[3].max_pos);
    if(!joint_distances_to_limit[4].infinite) constrain(prev[4], req[4], joint_limits[4].min_pos, joint_limits[4].max_pos);
    if(!joint_distances_to_limit[5].infinite) constrain(prev[5], req[5], joint_limits[5].min_pos, joint_limits[5].max_pos);

    double result[6];
    for(int i = 0; i < 6; i++) result[i] = prev[i] + s * (req[i] - prev[i]);

    move_joint_cmd_positions = {result[0], result[1], result[2], result[3], result[4], result[5]};
    move_joint_target_valid = true;

    return 0; // success
}

void kins::move_joints(){
    // synchronized joint-space point-to-point move: ramp all 6 joints toward
    // move_joint_cmd_positions together, respecting each joint's max velocity, max
    // acceleration, and position limits. The target may change at any time (via
    // set_move_joint_cmd_positions()); motion replans from the current position/velocity
    // every cycle.

    if(in_sigs.reset){
        // cancel any in-flight move -- run() overwrites cmd_joint_positions with the fbk
        // position right after this call, so match that here.
        move_joint_cmd_positions = in_sigs.fbk_joint_positions;
        move_joint_target_valid = true;
        move_joint_last_vel.fill(0.0f);
        out_sigs.at_cmd_pos = true;
        return;
    }

    if(!move_joint_target_valid){
        // no commanded target has been set yet -- nothing to do
        out_sigs.at_cmd_pos = true;
        return;
    }

    out_sigs.at_cmd_pos = ramp_joints_to_target(move_joint_cmd_positions, 1.0f, move_joint_last_vel);
}

bool kins::ramp_joints_to_target(const joint_positions& target_, float speed_scale, std::array<float,6>& last_vel){
    // shared trapezoidal ramp core -- extracted from move_joints() so the collision-backup
    // retrace (run_backup_retrace()) can drive the exact same accel-limited, synchronized-arrival
    // motion profile at a reduced speed cap, without duplicating this logic. speed_scale only
    // caps the velocity ceiling (joint_limits[i].max_vel * speed_scale); the stop-distance
    // calculation and the accel limit itself (joint_limits[i].max_acc) are never scaled, so
    // acceleration/deceleration behavior is identical between a normal move and a backup retrace --
    // only the top speed differs.

    std::array<double,6> current = {
        out_sigs.cmd_joint_positions.j1, out_sigs.cmd_joint_positions.j2,
        out_sigs.cmd_joint_positions.j3, out_sigs.cmd_joint_positions.j4,
        out_sigs.cmd_joint_positions.j5, out_sigs.cmd_joint_positions.j6
    };
    std::array<double,6> target = {
        target_.j1, target_.j2, target_.j3, target_.j4, target_.j5, target_.j6
    };

    const double pos_tol = 1e-5; // position tolerance for considering a joint "at target"

    std::array<double,6> dist;  // signed distance remaining, per joint
    std::array<float,6> v_cap;  // this joint's own max safe speed toward target this cycle
    for(int i = 0; i < 6; i++){
        dist[i] = target[i] - current[i];

        float acc = joint_limits[i].max_acc;
        float stop_vel = sqrtf(2.0f * acc * (float)fabs(dist[i])); // v such that we can still stop exactly at target
        float v = fminf(joint_limits[i].max_vel * speed_scale, stop_vel);

        if(dist[i] > 0)      v = fminf(v, joint_distances_to_limit[i].allowed_positive_vel);
        else if(dist[i] < 0) v = fminf(v, joint_distances_to_limit[i].allowed_negative_vel);
        else                 v = 0.0f;

        v_cap[i] = fmaxf(v, 0.0f);
    }

    // find the slowest joint (longest time to arrive at its own capped speed) and synchronize
    // every other joint's speed to match that time, so all 6 arrive together.
    float t_sync = 0.0f;
    for(int i = 0; i < 6; i++){
        if(fabs(dist[i]) > pos_tol && v_cap[i] > 1e-6f){
            t_sync = fmaxf(t_sync, (float)fabs(dist[i]) / v_cap[i]);
        }
    }

    std::array<double,6> new_pos;
    for(int i = 0; i < 6; i++){
        float v_target = 0.0f;
        if(fabs(dist[i]) > pos_tol){
            v_target = (t_sync > 1e-6f) ? (float)fabs(dist[i]) / t_sync : v_cap[i];
            v_target = fminf(v_target, v_cap[i]); // never exceed this joint's own safe speed
            v_target = copysignf(v_target, (float)dist[i]);
        }

        // accel-limit the change from last cycle's commanded velocity
        float max_dv = joint_limits[i].max_acc * time_step;
        float dv = v_target - last_vel[i];
        if(fabs(dv) > max_dv) v_target = last_vel[i] + copysignf(max_dv, dv);
        last_vel[i] = v_target;

        double step = (double)v_target * time_step;
        new_pos[i] = current[i] + step;

        // don't overshoot the target
        if((dist[i] >= 0 && new_pos[i] > target[i]) || (dist[i] < 0 && new_pos[i] < target[i])){
            new_pos[i] = target[i];
            last_vel[i] = 0.0f;
        }
    }

    out_sigs.cmd_joint_positions = {new_pos[0], new_pos[1], new_pos[2], new_pos[3], new_pos[4], new_pos[5]};

    bool all_at_target = true;
    for(int i = 0; i < 6; i++){
        if(fabs(target[i] - new_pos[i]) > pos_tol) all_at_target = false;
    }
    return all_at_target;
}

uint32_t kins::set_move_cartesian_cmd_positions(const cartesian_positions& new_target){

    if(collision_backup_state != backup_state::IDLE){
        // reject new move targets while a collision backup is retracing or decelerating -- the
        // robot must not resume forward motion until it has fully stopped and collision_detected
        // has cleared.
        return 1;
    }

    float prev[6] = {
        move_cartesian_cmd_positions.x, move_cartesian_cmd_positions.y, move_cartesian_cmd_positions.z,
        move_cartesian_cmd_positions.xangle, move_cartesian_cmd_positions.yangle, move_cartesian_cmd_positions.zangle
    };
    float req[6] = {
        new_target.x, new_target.y, new_target.z,
        new_target.xangle, new_target.yangle, new_target.zangle
    };

    // unwrap requested angle targets onto their limit band's branch so off-centre / wide bands
    // compare correctly in constrain() below. index 3,4,5 == xangle,yangle,zangle.
    for(int i = 3; i <= 5; i++) req[i] = unwrap_cartesian_angle(i, req[i]);

    // largest fraction of this call's requested move that every constrained axis can tolerate
    float s = 1.0f;
    auto constrain = [&](float base, float target_val, float min_pos, float max_pos){
        if(target_val >= min_pos && target_val <= max_pos) return; // already in range
        float delta = target_val - base;
        if(delta == 0.0f) return;                                  // this axis isn't moving
        float limit = (target_val > max_pos) ? max_pos : min_pos;
        s = fminf(s, std::clamp((limit - base) / delta, 0.0f, 1.0f));
    };

    for(int i = 0; i < 6; i++){
        if(!cartesian_distances_to_limit[i].infinite) constrain(prev[i], req[i], cartesian_limits[i].min_pos, cartesian_limits[i].max_pos);
    }

    float result[6];
    for(int i = 0; i < 6; i++) result[i] = prev[i] + s * (req[i] - prev[i]);

    move_cartesian_cmd_positions = {result[0], result[1], result[2], result[3], result[4], result[5]};
    move_cartesian_target_valid = true;

    return 0; // success
}

void kins::move_cartesian(){
    // synchronized cartesian-space point-to-point move: ramp x/y/z and a combined 3D orientation
    // channel toward move_cartesian_cmd_positions together, respecting each channel's max
    // velocity/acceleration. The target may change at any time (via
    // set_move_cartesian_cmd_positions()); motion replans from the current position/velocity
    // every cycle. IK is solved every cycle via inverse_kins(), same as jog_cartesian().
    //
    // Orientation (previously xangle/yangle/zangle ramped as 3 independent Euler-angle scalars)
    // is ramped as a SINGLE quaternion-slerp channel instead. Independent per-axis Euler ramping
    // is only a valid stand-in for "shortest path in 3D orientation" away from gimbal lock
    // (yangle ~= +/-90deg); near gimbal lock a tiny real orientation change can require
    // xangle/zangle to swing by up to 180deg in the Euler representation, so per-axis ramping
    // (even shortest-angular-path-per-axis) can command large, wrong-direction wrist motion.
    // Quaternion slerp interpolates the actual 3D rotation directly and has no such singularity.
    //
    // Known limitation: for a config where the angle axes are bounded (non-infinite, e.g.
    // new_node_config.json's +/-180deg) this channel has no mechanism to keep the interpolated
    // path's per-axis Euler decomposition within cartesian_limits[3..5].min_pos/max_pos --
    // set_move_cartesian_cmd_positions() still clamps the *endpoint* into range, but the
    // intermediate slerp path is not guaranteed to stay in range the way per-axis linear
    // interpolation used to. Out of scope here; the active config (r2000) has all 3 angle axes
    // unbounded (continuous).

    if(in_sigs.reset){
        // cancel any in-flight move -- resync to the true current pose, same as the CARTESIAN
        // mode entry check in run() does.
        move_cartesian_cmd_positions = out_sigs.fbk_cartesian_positions;
        last_cmd_cartesian_positions = out_sigs.fbk_cartesian_positions;
        move_cartesian_target_valid = true;
        move_cartesian_last_vel.fill(0.0f);
        move_cartesian_last_ang_vel = 0.0f;
        out_sigs.at_cmd_pos = true;
        return;
    }

    if(!move_cartesian_target_valid){
        // no commanded target has been set yet -- nothing to do
        out_sigs.at_cmd_pos = true;
        return;
    }

    // position / orientation tolerances for "at target" (also gate the ramp termination below).
    // Configurable; defaults deliberately clear of float32 noise -- angle_tol in particular must
    // stay above the ~7e-4 rad resolution floor of 2*acosf(|dot|) near dot=1, or the ramp never
    // snaps to target and at_cmd_pos never latches. See kins.h.
    const float pos_tol   = at_cmd_pos_tol;
    const float angle_tol = at_cmd_pos_angle_tol;

    // ---- x, y, z: per-axis accel/vel-limited ramp (unchanged) ----------------------------------
    std::array<float,3> current = {
        last_cmd_cartesian_positions.x, last_cmd_cartesian_positions.y, last_cmd_cartesian_positions.z
    };
    std::array<float,3> target = {
        move_cartesian_cmd_positions.x, move_cartesian_cmd_positions.y, move_cartesian_cmd_positions.z
    };

    std::array<float,3> dist;  // signed distance remaining, per axis
    std::array<float,3> v_cap; // this axis's own max safe speed toward target this cycle
    for(int i = 0; i < 3; i++){
        dist[i] = target[i] - current[i];

        float acc = cartesian_limits[i].max_acc;
        float stop_vel = sqrtf(2.0f * acc * fabsf(dist[i])); // v such that we can still stop exactly at target
        float v = fminf(cartesian_limits[i].max_vel, stop_vel);

        if(dist[i] > 0)      v = fminf(v, cartesian_distances_to_limit[i].allowed_positive_vel);
        else if(dist[i] < 0) v = fminf(v, cartesian_distances_to_limit[i].allowed_negative_vel);
        else                 v = 0.0f;

        v_cap[i] = fmaxf(v, 0.0f);
    }

    // ---- orientation: single combined quaternion-slerp channel ---------------------------------
    quat_rot q_current = quatFromZYX(last_cmd_cartesian_positions.zangle, last_cmd_cartesian_positions.yangle, last_cmd_cartesian_positions.xangle);
    quat_rot q_target  = quatFromZYX(move_cartesian_cmd_positions.zangle,  move_cartesian_cmd_positions.yangle,  move_cartesian_cmd_positions.xangle);

    // total remaining rotation angle, shortest path, radians -- see quat_angle_between().
    float ang_dist = quat_angle_between(q_current, q_target);

    // combined max_vel/max_acc for the orientation channel: the most conservative (MIN) of the 3
    // configured angle-axis limits. A single physical TCP can't be told "rotate at X deg/s about
    // combined axis A but only Y deg/s about combined axis B" -- a single scalar cap per channel
    // is the closest sane approximation without a genuinely axis-decomposed 3D angular-rate-limit
    // model. cartesian_distances_to_limit[3..5] (per-axis Euler position-limit throttling) is
    // intentionally NOT applied to this channel -- see the known-limitation note above.
    float ang_max_vel = fminf(fminf(cartesian_limits[3].max_vel, cartesian_limits[4].max_vel), cartesian_limits[5].max_vel);
    float ang_max_acc = fminf(fminf(cartesian_limits[3].max_acc, cartesian_limits[4].max_acc), cartesian_limits[5].max_acc);

    float ang_stop_vel = sqrtf(2.0f * ang_max_acc * ang_dist); // v such that we can still stop exactly at target
    float ang_v_cap = fmaxf(fminf(ang_max_vel, ang_stop_vel), 0.0f);

    // find the slowest channel (longest time to arrive at its own capped speed) and synchronize
    // every other channel's speed to match that time, so translation and reorientation finish
    // together.
    float t_sync = 0.0f;
    for(int i = 0; i < 3; i++){
        if(fabsf(dist[i]) > pos_tol && v_cap[i] > 1e-6f){
            t_sync = fmaxf(t_sync, fabsf(dist[i]) / v_cap[i]);
        }
    }
    if(ang_dist > angle_tol && ang_v_cap > 1e-6f){
        t_sync = fmaxf(t_sync, ang_dist / ang_v_cap);
    }

    std::array<float,3> new_pos;
    for(int i = 0; i < 3; i++){
        float v_target = 0.0f;
        if(fabsf(dist[i]) > pos_tol){
            v_target = (t_sync > 1e-6f) ? fabsf(dist[i]) / t_sync : v_cap[i];
            v_target = fminf(v_target, v_cap[i]); // never exceed this axis's own safe speed
            v_target = copysignf(v_target, dist[i]);
        }

        // accel-limit the change from last cycle's commanded velocity
        float max_dv = cartesian_limits[i].max_acc * time_step;
        float dv = v_target - move_cartesian_last_vel[i];
        if(fabsf(dv) > max_dv) v_target = move_cartesian_last_vel[i] + copysignf(max_dv, dv);
        move_cartesian_last_vel[i] = v_target;

        float step = v_target * time_step;
        new_pos[i] = current[i] + step;

        // don't overshoot the target
        if((dist[i] >= 0 && new_pos[i] > target[i]) || (dist[i] < 0 && new_pos[i] < target[i])){
            new_pos[i] = target[i];
            move_cartesian_last_vel[i] = 0.0f;
        }
    }

    // orientation channel: same stop-distance / sync-to-t_sync / accel-limit-from-last-cycle
    // pattern as x/y/z above, but as a single non-negative scalar speed -- direction is encoded
    // entirely in q_current/q_target/slerp(), there is no separate sign to track.
    float ang_v_target = 0.0f;
    if(ang_dist > angle_tol){
        ang_v_target = (t_sync > 1e-6f) ? ang_dist / t_sync : ang_v_cap;
        ang_v_target = fminf(ang_v_target, ang_v_cap); // never exceed this channel's own safe speed
    }

    float ang_max_dv = ang_max_acc * time_step;
    float ang_dv = ang_v_target - move_cartesian_last_ang_vel;
    if(fabsf(ang_dv) > ang_max_dv) ang_v_target = move_cartesian_last_ang_vel + copysignf(ang_max_dv, ang_dv);
    // unlike x/y/z's v_target, this channel has no reverse direction -- accel-limiting toward a
    // shrinking target could otherwise drive it negative, which would feed slerp() a negative
    // fraction (extrapolating backward past q_current instead of a no-op).
    ang_v_target = fmaxf(ang_v_target, 0.0f);
    move_cartesian_last_ang_vel = ang_v_target;

    float ang_step = ang_v_target * time_step; // this cycle's rotation step, radians

    quat_rot q_new;
    if(ang_dist > angle_tol){
        float frac = ang_step / ang_dist; // slerp fraction, using ang_dist measured at cycle start
        if(frac >= 1.0f){
            // don't overshoot the target
            q_new = q_target;
            move_cartesian_last_ang_vel = 0.0f;
        } else {
            q_new = slerp(q_current, q_target, frac);
        }
    } else {
        q_new = q_current; // already within tolerance -- hold, same as the linear axes do
    }

    float new_zangle, new_yangle, new_xangle;
    // decompose with each angle axis unwrapped onto its limit band's branch so the limit checks
    // and feedback share one 2*pi branch. This is periodicity-only (k*2pi shift) -- it does NOT
    // change the orientation handed to inverse_kins().
    //
    // NB: do NOT clamp new_xangle/new_yangle/new_zangle to cartesian_limits here. Overwriting a
    // single Euler component produces a different physical orientation than the slerp point, and
    // combined with the still-advancing linear position that pose is frequently unreachable ->
    // inverse_kins() diverges and rolls back every cycle -> permanent stall. The slerp path's
    // Euler decomposition is allowed to transiently leave the configured angle box (pre-existing
    // limitation, see this function's header comment); any real path limiting must constrain the
    // slerp itself, never post-clamp a component.
    decompose_cmd_angles(q_new, new_zangle, new_yangle, new_xangle);

    // snapshot before mutating, same convention jog_cartesian()/jog_mouse() use -- inverse_kins()
    // rolls last_cmd_cartesian_positions back to this snapshot on IK failure or failed validation.
    prev_cmd_cartesian_positions = last_cmd_cartesian_positions;
    last_cmd_cartesian_positions = {new_pos[0], new_pos[1], new_pos[2], new_xangle, new_yangle, new_zangle};
    inverse_kins(); // solve IK, rate-limit, validate, and write out_sigs.cmd_joint_positions on success

    // check against the post-inverse_kins() state, not the pre-IK ramped new_pos -- if IK failed
    // this cycle and rolled back, we did not actually reach new_pos and at_cmd_pos must reflect
    // that.
    std::array<float,3> reached = {
        last_cmd_cartesian_positions.x, last_cmd_cartesian_positions.y, last_cmd_cartesian_positions.z
    };
    bool all_at_target = true;
    for(int i = 0; i < 3; i++){
        if(fabsf(target[i] - reached[i]) > pos_tol) all_at_target = false;
    }

    // orientation: shortest-path rotation angle between the post-inverse_kins() reached
    // orientation and the target orientation -- NOT a per-axis Euler comparison. At/near gimbal
    // lock, toEulerZYX() (and inverse_kins()'s rate-limited-path re-derivation via
    // atan2/matrixToPose) can land on a different xangle/zangle split than commanded for the
    // exact same physical orientation, so a per-axis Euler comparison would false-trigger here --
    // same reasoning validate_kins_solution() already uses its rotation-matrix Frobenius check for.
    quat_rot q_reached = quatFromZYX(last_cmd_cartesian_positions.zangle, last_cmd_cartesian_positions.yangle, last_cmd_cartesian_positions.xangle);
    // cancellation-free equivalent of quat_angle_between(q_reached, q_target) <= angle_tol:
    // |dot| >= cos(angle_tol/2). Avoids acosf near dot=1 (infinite slope -> ~7e-4 rad float32
    // resolution floor); cosf near 0 is well-conditioned so the threshold stays meaningful.
    {
        float ori_dot = fabsf(q_reached.w*q_target.w + q_reached.x*q_target.x
                            + q_reached.y*q_target.y + q_reached.z*q_target.z);
        if(ori_dot < cosf(0.5f * angle_tol)) all_at_target = false;
    }

    out_sigs.at_cmd_pos = all_at_target;
}

// ============================================================================================
// collision backup (retrace-path)
// ============================================================================================

void kins::record_backup_point(){
    // append the current commanded joint position to the history buffer if it moved far enough
    // (in joint space or in cartesian space) since the last recorded point. Called once per cycle
    // from run(), only while control_mode is JOINT/CARTESIAN and no backup is in progress.

    joint_positions candidate = out_sigs.cmd_joint_positions;

    if(backup_count == 0){
        // always keep at least one valid point once motion starts -- seed unconditionally
        backup_buffer[backup_write_index] = candidate;
        backup_last_recorded_cartesian = out_sigs.fbk_cartesian_positions;
        backup_write_index = (backup_write_index + 1) % backup_buffer.size();
        backup_count = 1;
        return;
    }

    const joint_positions& last = backup_buffer[backup_newest_index()];
    double max_joint_delta = fmax(fmax(fabs(candidate.j1 - last.j1), fabs(candidate.j2 - last.j2)),
                              fmax(fmax(fabs(candidate.j3 - last.j3), fabs(candidate.j4 - last.j4)),
                                   fmax(fabs(candidate.j5 - last.j5), fabs(candidate.j6 - last.j6))));

    float dx = out_sigs.fbk_cartesian_positions.x - backup_last_recorded_cartesian.x;
    float dy = out_sigs.fbk_cartesian_positions.y - backup_last_recorded_cartesian.y;
    float dz = out_sigs.fbk_cartesian_positions.z - backup_last_recorded_cartesian.z;
    float cart_delta = sqrtf(dx*dx + dy*dy + dz*dz);

    if(max_joint_delta > backup_record_joint_threshold || cart_delta > backup_record_cartesian_threshold){
        if(backup_count < backup_buffer.size()) backup_count++; // else: buffer full, oldest slot is overwritten below
        backup_buffer[backup_write_index] = candidate;
        backup_last_recorded_cartesian = out_sigs.fbk_cartesian_positions;
        backup_write_index = (backup_write_index + 1) % backup_buffer.size();
    }
}

void kins::start_collision_backup(){
    // collision_detected rising edge (or a reassertion mid-DECELERATING): abandon whatever move
    // was in flight immediately, ignoring accel limits entirely -- identical semantics to the
    // existing in_sigs.reset branches in move_joints()/move_cartesian() (snap the target to the
    // current position, zero velocity state), so there is no decel ramp before backup begins.

    move_joint_cmd_positions = out_sigs.cmd_joint_positions;
    move_joint_target_valid = true;
    move_joint_last_vel.fill(0.0f);

    move_cartesian_cmd_positions = out_sigs.fbk_cartesian_positions;
    last_cmd_cartesian_positions = out_sigs.fbk_cartesian_positions;
    move_cartesian_target_valid = true;
    move_cartesian_last_vel.fill(0.0f);
    move_cartesian_last_ang_vel = 0.0f;

    collision_backup_state = backup_state::RETRACING;
    backup_last_vel.fill(0.0f); // fresh ramp state -- the retrace accelerates from zero, at full max_acc, up to backup_speed_scale * max_vel
}

void kins::run_backup_retrace(){
    // drive backward toward the newest buffered point at backup_speed_scale; once reached, pop it
    // (advance to the next older point). If the buffer is empty, there is nowhere left to go --
    // hold position (out_sigs.backup_buffer_exhausted is computed centrally in run()).

    if(backup_count == 0){
        backup_last_vel.fill(0.0f);
        out_sigs.at_cmd_pos = true;
        return;
    }

    const joint_positions& target = backup_buffer[backup_newest_index()];
    bool reached = ramp_joints_to_target(target, backup_speed_scale, backup_last_vel);

    if(reached){
        // "pop" this waypoint -- it's now behind us. This is also the undo half of the undo/redo
        // buffer semantics: once backup ends, whatever remains in the buffer is exactly the
        // history behind the stopping point, and new recordings will overwrite forward from there.
        backup_write_index = backup_newest_index();
        backup_count--;
    }
    out_sigs.at_cmd_pos = false;
}

void kins::begin_backup_decel(){
    // collision_detected falling edge: stop retracing and begin an accel-limited decel to a full
    // stop, even if not exactly at a stored waypoint.

    if(backup_count > 0){
        // we were mid-transit toward this waypoint and never actually reached it -- discard it
        // (same pop operation as run_backup_retrace()'s "reached" branch) so it doesn't remain
        // claimed as valid history; new recording will overwrite forward from the actual
        // stopping point once decel finishes.
        backup_write_index = backup_newest_index();
        backup_count--;
    }

    collision_backup_state = backup_state::DECELERATING;
    // backup_last_vel is intentionally NOT reset here -- decel continues ramping down from
    // whatever velocity the retrace ramp had, for velocity continuity (no discontinuity).
}

void kins::run_backup_decel(){
    // ramp all 6 joints' velocity to zero at joint_limits[i].max_acc, with no position target
    // (and therefore no overshoot clamp) -- this is what makes it "obey normal decel limits"
    // rather than the abrupt cancel used to enter backup.

    std::array<double,6> new_pos = {
        out_sigs.cmd_joint_positions.j1, out_sigs.cmd_joint_positions.j2,
        out_sigs.cmd_joint_positions.j3, out_sigs.cmd_joint_positions.j4,
        out_sigs.cmd_joint_positions.j5, out_sigs.cmd_joint_positions.j6
    };

    bool all_stopped = true;
    for(int i = 0; i < 6; i++){
        float v = backup_last_vel[i];
        if(v != 0.0f){
            float max_dv = joint_limits[i].max_acc * time_step;
            v = (fabsf(v) > max_dv) ? (v - copysignf(max_dv, v)) : 0.0f;
            backup_last_vel[i] = v;
        }
        if(v != 0.0f) all_stopped = false;
        new_pos[i] += (double)v * time_step;
    }

    out_sigs.cmd_joint_positions = {new_pos[0], new_pos[1], new_pos[2], new_pos[3], new_pos[4], new_pos[5]};

    if(all_stopped){
        // fully stopped -- exit backup and resync all move-tracking state to this final position,
        // so a subsequent JOINT or CARTESIAN move (whichever mode is active) starts cleanly from
        // here rather than jumping back toward the original pre-collision target.
        //
        // target_valid = false (not true, unlike a plain reset) so move_joints()/move_cartesian()
        // early-return -- doing nothing, calling no IK -- until an actual new command is issued.
        // Forcing target_valid = true here would make move_cartesian() call inverse_kins() every
        // cycle from this point on with no command ever having been sent, the same needless
        // solver-load regression described in run()'s mode-change handling above.
        move_joint_cmd_positions = out_sigs.cmd_joint_positions;
        move_joint_target_valid = false;
        move_joint_last_vel.fill(0.0f);

        move_cartesian_cmd_positions = out_sigs.fbk_cartesian_positions;
        last_cmd_cartesian_positions = out_sigs.fbk_cartesian_positions;
        move_cartesian_target_valid = false;
        move_cartesian_last_vel.fill(0.0f);
        move_cartesian_last_ang_vel = 0.0f;

        collision_backup_state = backup_state::IDLE;
        backup_last_vel.fill(0.0f);
    }
    out_sigs.at_cmd_pos = all_stopped;
}

void kins::inverse_kins(){

    std::array<float,3> target_pos_tool = {
        last_cmd_cartesian_positions.x,
        last_cmd_cartesian_positions.y,
        last_cmd_cartesian_positions.z
    };
    // poseToMatrix()/matrixToPose() interpret this array as [Z-rot, Y-rot, X-rot] (see
    // validate_kins_solution()'s convention note), so build it from zangle/yangle/xangle in that
    // order -- this is what makes .xangle genuinely mean "rotate about physical X" and .zangle mean
    // "rotate about physical Z", matching jog_cartesian()/jog_mouse()'s convention.
    std::array<float,3> target_euler_tool = {
        last_cmd_cartesian_positions.zangle,  // zyx[0] = Z-rotation
        last_cmd_cartesian_positions.yangle,  // zyx[1] = Y-rotation
        last_cmd_cartesian_positions.xangle   // zyx[2] = X-rotation
    };

    // un-apply tool offset to get the flange position and orientation
    std::array<float,3> target_pos;
    std::array<float,3> target_euler;
    remove_tool_offset(target_pos_tool, target_euler_tool, target_pos, target_euler);

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
        // IK failed (singular Jacobian or no convergence — common at wrist/elbow singularities).
        // jog_cartesian already advanced last_cmd_cartesian_positions; roll it back to prevent
        // the integrator from drifting away from the joint positions and creating a deadlock.
        last_cmd_cartesian_positions = prev_cmd_cartesian_positions;
    }
    else{
        // update the commanded joint positions based on the solution

        // invert j3, j4, j5 to match the robot's joint directions
        outSolution[2] = -outSolution[2];
        outSolution[3] = -outSolution[3];
        outSolution[4] = -outSolution[4];

        // un-offset j3
        outSolution[2] -= outSolution[1];

        // apply joint velocity rate limiting — all joints scale by the same factor to stay in sync
        float rate_scale = apply_cartesian_joint_rate_limit(outSolution);
        // float rate_scale = 1.0f; // bypass for testing

        // update the last commanded cartesian positions
        last_cmd_cartesian_positions.x      = target_pos_tool[0];
        last_cmd_cartesian_positions.y      = target_pos_tool[1];
        last_cmd_cartesian_positions.z      = target_pos_tool[2];
        last_cmd_cartesian_positions.zangle = target_euler_tool[0];
        last_cmd_cartesian_positions.yangle = target_euler_tool[1];
        last_cmd_cartesian_positions.xangle = target_euler_tool[2];

        // If rate limiting reduced the step, set the integrator to the FK of the rate-limited
        // joints rather than a proportional interpolation in Cartesian space.
        // Proportional rollback is inaccurate because FK is nonlinear:
        //   FK(current_j + delta*s)  ≠  prev_cart + (target_cart - prev_cart)*s
        // That mismatch accumulates each cycle and causes Newton-Raphson to see an error term
        // beyond the jog step.  Near a joint direction-change point (Jacobian column ~ 0),
        // the extra term drives convergence to the wrong branch.
        // FK-based update makes the invariant exact:
        //   FK(cmd_joint_positions) == last_cmd_cartesian_positions
        // so the next IK call sees a pure one-step error with a fully consistent initial guess.
        if(rate_scale < 1.0f){
            // Same motor→IK convention as validate_kins_solution.
            std::array<float,6> fk_joints = outSolution;
            fk_joints[2] += fk_joints[1];   // j3_servo = j3_rel + j2
            fk_joints[2] = -fk_joints[2];   // IK convention: negate j3
            fk_joints[3] = -fk_joints[3];   // IK convention: negate j4
            fk_joints[4] = -fk_joints[4];   // IK convention: negate j5
            for(auto& a : fk_joints)         // wrap to [-π, π]
                a = fmod(a + (float)M_PI, 2.0f * (float)M_PI) - (float)M_PI;

            std::array<float,3> rl_pos;
            std::array<float,9> rl_rot;
            ik_solver.forwardKinematicsF32(fk_joints, rl_pos, rl_rot);

            Mat4 T_fl{};
            for(int r = 0; r < 3; r++)
                for(int c = 0; c < 3; c++)
                    T_fl.m[r][c] = rl_rot[r * 3 + c];
            T_fl.m[0][3] = rl_pos[0]; T_fl.m[1][3] = rl_pos[1]; T_fl.m[2][3] = rl_pos[2];
            T_fl.m[3][3] = 1.0f;

            Mat4 T_tcp_rl = (current_tool != nullptr)
                ? mul(T_fl, poseToMatrix(current_tool->position, current_tool->orientation))
                : T_fl;

            std::array<float,3> tcp_pos_rl, tcp_eul_rl;
            matrixToPose(T_tcp_rl, tcp_pos_rl, tcp_eul_rl);

            last_cmd_cartesian_positions.x      = tcp_pos_rl[0];
            last_cmd_cartesian_positions.y      = tcp_pos_rl[1];
            last_cmd_cartesian_positions.z      = tcp_pos_rl[2];
            // unwrap bounded angle axes onto their limit band's branch (index 3,4,5 ==
            // xangle,yangle,zangle)
            last_cmd_cartesian_positions.zangle = unwrap_cartesian_angle(5, tcp_eul_rl[0]);
            last_cmd_cartesian_positions.yangle = unwrap_cartesian_angle(4, tcp_eul_rl[1]);
            last_cmd_cartesian_positions.xangle = unwrap_cartesian_angle(3, tcp_eul_rl[2]);
        }

        bool valid = validate_kins_solution(&outSolution, &last_cmd_cartesian_positions, 0.02f, 0.05f);

        if(valid){
            set_closest_joint_positions(&outSolution);
        }
        else{
            // IK solution doesn't match the commanded position — freeze rather than go to wrong pose.
            // Roll the integrator back so it stays in sync with the actual joint positions.
            last_cmd_cartesian_positions = prev_cmd_cartesian_positions;
            volatile bool t; // for debugging breakpoint
            t = true;        // set a breakpoint here to debug invalid kins solution
        }
    }
}

float kins::apply_cartesian_joint_rate_limit(std::array<float,6>& joint_angles) {
    const double current_pos[6] = {
        out_sigs.cmd_joint_positions.j1, out_sigs.cmd_joint_positions.j2,
        out_sigs.cmd_joint_positions.j3, out_sigs.cmd_joint_positions.j4,
        out_sigs.cmd_joint_positions.j5, out_sigs.cmd_joint_positions.j6
    };

    double deltas[6];
    for(size_t i = 0; i < 6; i++){
        deltas[i] = wrapNearest(current_pos[i], (double)joint_angles[i]) - current_pos[i];
    }

    float min_scale = 1.0f;
    for(size_t i = 0; i < 6; i++){
        double effective_delta = deltas[i];
        if(i == 2) effective_delta += deltas[1]; // J3 servo sees J2 + J3 (parallelogram coupling)

        double vel = effective_delta / time_step;
        float max_vel = (vel >= 0.0)
            ? joint_distances_to_limit[i].allowed_positive_vel
            : joint_distances_to_limit[i].allowed_negative_vel;

        if(max_vel <= 0.0f){
            if(vel != 0.0) min_scale = 0.0f;
        } else if(std::abs(vel) > (double)max_vel){
            float s = max_vel / (float)std::abs(vel);
            if(s < min_scale) min_scale = s;
        }
    }

    if(min_scale >= 1.0f) return 1.0f;

    for(size_t i = 0; i < 6; i++){
        joint_angles[i] = (float)(current_pos[i] + deltas[i] * min_scale);
    }
    return min_scale;
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

    if(jog_vel == 0.0f){    // no movement
        out_sigs.at_cmd_pos = true;
        return;
    }
    out_sigs.at_cmd_pos = false;

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

float kins::unwrap_cartesian_angle(int axis_index, float angle){
    if(cartesian_distances_to_limit[axis_index].infinite) return angle; // continuous axis, principal range
    // centre the unwrap on the band midpoint: any angle actually within the band (width < 2*pi)
    // lands on the band's branch regardless of which +-pi branch toEulerZYX() produced.
    float mid = 0.5f * (cartesian_limits[axis_index].min_pos + cartesian_limits[axis_index].max_pos);
    return (float)wrapNearest(mid, angle);
}

void kins::decompose_cmd_angles(const quat_rot& q, float& zangle, float& yangle, float& xangle){
    // cartesian_limits / cartesian_distances_to_limit index 3,4,5 == xangle,yangle,zangle
    toEulerZYX(&q, &zangle, &yangle, &xangle);
    zangle = unwrap_cartesian_angle(5, zangle);
    yangle = unwrap_cartesian_angle(4, yangle);
    xangle = unwrap_cartesian_angle(3, xangle);
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

        if(joint_distances_to_limit[i].infinite || bypass_joint_limits){
            joint_distances_to_limit[i].allowed_positive_vel = vel; // infinite joint, can move at full speed
            joint_distances_to_limit[i].allowed_negative_vel = vel; // infinite joint, can move at full speed
            joint_distances_to_limit[i].to_min = std::numeric_limits<float>::infinity();
            joint_distances_to_limit[i].to_max = std::numeric_limits<float>::infinity();
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

void kins::update_cartesian_distances_to_limit(){
    // update how far in each direction each cartesian axis is from a limit
    // uses final commanded cartesian positions

    // x, y, z and xangle, yangle, zangle are all limited
    for(int i = 0; i < 6; i++){

        if(cartesian_distances_to_limit[i].infinite || bypass_cartesian_limits){
            cartesian_distances_to_limit[i].to_min = std::numeric_limits<float>::infinity();
            cartesian_distances_to_limit[i].to_max = std::numeric_limits<float>::infinity();
            cartesian_distances_to_limit[i].allowed_positive_vel = cartesian_limits[i].max_vel;
            cartesian_distances_to_limit[i].allowed_negative_vel = cartesian_limits[i].max_vel;
            continue;
        }

        float pos = 0.0f;
        switch(i){
            case 0: pos = last_cmd_cartesian_positions.x; break;
            case 1: pos = last_cmd_cartesian_positions.y; break;
            case 2: pos = last_cmd_cartesian_positions.z; break;
            case 3: pos = last_cmd_cartesian_positions.xangle; break;
            case 4: pos = last_cmd_cartesian_positions.yangle; break;
            case 5: pos = last_cmd_cartesian_positions.zangle; break;
        }
        cartesian_distances_to_limit[i].to_min = pos - cartesian_limits[i].min_pos;
        cartesian_distances_to_limit[i].to_max = cartesian_limits[i].max_pos - pos;

        // calculate allowed speeds based on the distances to the limits
        float acc = cartesian_limits[i].max_acc;
        float vel = cartesian_limits[i].max_vel;

        // calculate the distance we need to stop from full speed
        float stop_distance = (vel * vel) / (2.0 * acc); // d = v^2 / (2 * a)

        float pos_speed = 0.0f;
        float neg_speed = 0.0f;

        // calculate the velocity based on the distance to the limit
        if(cartesian_distances_to_limit[i].to_min > stop_distance){
            // not close to limit
            neg_speed = vel; // we can move at full speed
        } else if(cartesian_distances_to_limit[i].to_min < 0){
            // outside the limit, we cannot move in this direction
            neg_speed = 0.0f;
        } else {
            // close to limit, we need to slow down
            neg_speed = sqrt(2.0 * acc * cartesian_distances_to_limit[i].to_min); // v = sqrt(2 * a * d)
        }

        if(cartesian_distances_to_limit[i].to_max > stop_distance){
            // not close to limit
            pos_speed = vel; // we can move at full speed
        } else if(cartesian_distances_to_limit[i].to_max < 0){
            // outside the limit, we cannot move in this direction
            pos_speed = 0.0f;
        } else {
            // close to limit, we need to slow down
                pos_speed = sqrt(2.0 * acc * cartesian_distances_to_limit[i].to_max); // v = sqrt(2 * a * d)
            }

        cartesian_distances_to_limit[i].allowed_positive_vel = pos_speed;
        cartesian_distances_to_limit[i].allowed_negative_vel = neg_speed;

    }
}

uint32_t kins::select_tool(uint8_t tool_index) {
    if (tool_index < tools.size()) {
        current_tool = &tools[tool_index];
        return 0; // success
    }
    current_tool = nullptr;
    return 1; // error: tool index out of range
}

uint32_t kins::add_tool(const tool new_tool) {
    if (tools.size() < 32) {
        tools.push_back(new_tool);
        return 0; // success
    }
    return 1; // error: too many tools
}

void kins::apply_tool_offset(const std::array<float,3>& flange_pos,
                                       const std::array<float,3>& flange_eul,
                                       std::array<float,3>& tool_pos,
                                       std::array<float,3>& tool_eul) {
    if (current_tool != nullptr) {
        // Apply the current tool's offset to the TCP position and orientation
        applyTool(flange_pos, flange_eul, current_tool->position, current_tool->orientation, tool_pos, tool_eul);
    }
    else {
        // If no tool is selected, just copy the flange position and orientation
        for (int i = 0; i < 3; i++) {
            tool_pos[i] = flange_pos[i];
            tool_eul[i] = flange_eul[i];
        }
    }
}

void kins::remove_tool_offset(const std::array<float,3>& tcp_pos,
                                       const std::array<float,3>& tcp_eul,
                                       std::array<float,3>& flange_pos,
                                       std::array<float,3>& flange_eul) {
    if (current_tool != nullptr) {
        // Remove the current tool's offset from the TCP position and orientation
        removeTool(tcp_pos, tcp_eul, current_tool->position, current_tool->orientation, flange_pos, flange_eul);
    }
    else {
        // If no tool is selected, just copy the TCP position and orientation
        for (int i = 0; i < 3; i++) {
            flange_pos[i] = tcp_pos[i];
            flange_eul[i] = tcp_eul[i];
        }
    }
}

static Node_Registrar<kins> node_registrar_kins("kins");
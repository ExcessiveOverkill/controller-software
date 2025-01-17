from motor_drives.em_hvsd import EM_HVSD
from motors.motor_rotary import ROTARY_MOTOR
from connectors import CONNECTOR_FUNCTIONS

if __name__ == '__main__':
    motor = ROTARY_MOTOR()
    drive = EM_HVSD()

    motor.set_mode(motor.MODES.PMSM)
    motor.common_params.rated_speed.value = 3000
    motor.common_params.rated_voltage.value = 220
    motor.common_params.hard_max_current.value = 10
    #motor.specific_params.pole_pairs.value = 4

    drive.set_mode(drive.MODES.PMSM_FOC)
    drive.common_params.serial_address.value = 1

    print(CONNECTOR_FUNCTIONS.validate_connect_to(drive.connectors["motor"], [motor.connectors["motor"]]))

    print(CONNECTOR_FUNCTIONS.connect_to(drive.connectors["motor"], [motor.connectors["motor"]]))

    print(CONNECTOR_FUNCTIONS.update_from_connector(drive.connectors["motor"], motor.connectors["motor"]))
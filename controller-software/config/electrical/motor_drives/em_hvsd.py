#### High Voltage Servo Drive (HVSD) ####

from dataclasses import dataclass, field, asdict
from typing import Union
from enum import Enum, auto

import sys
import os

motor_drives_dir = os.path.dirname(os.path.realpath(__file__))
electrical_dir = os.path.dirname(motor_drives_dir)

sys.path.append(electrical_dir)

from parameters import TEXT_PARAMETER, FLOAT_PARAMETER, INT_PARAMETER, set_parents
from user_params import USER_PARAMS
from connectors import CONNECTOR, CONNECTOR_TYPES



# setup all parameters for device

@dataclass
class COMMON:
    # Common parameters for all modes

    serial_address: INT_PARAMETER = field(default_factory=lambda: INT_PARAMETER(description="Serial address of the motor drive", required=True, min_value=0, max_value=255))
    hard_current_limit: FLOAT_PARAMETER = field(default_factory=lambda: FLOAT_PARAMETER(description="Hard current limit", unit="Amps", required=True, min_value=0.0, max_value=60.0, auto_fill_enable=True, auto_fill_by=["hard_max_current"]))
    # hard_current_limit: float = field(default=None, metadata={'desc': 'Hard current limit in A', 'required':False})
    # soft_current_limit: float = field(default=None, metadata={'desc': 'Soft current limit in A', 'required':False})
    # max_bus_voltage: float = field(default=None, metadata={'unit': 'V', 'desc': 'Maximum bus voltage in V', 'required':False})
    # min_bus_voltage: float = field(default=None, metadata={'unit': 'V', 'desc': 'Minimum bus voltage in V', 'required':False})
    # phase_resistance: float = field(default=None, metadata={'unit': 'Ohm', 'desc': 'Phase resistance of the motor', 'required':False})
    # phase_inductance: float = field(default=None, metadata={'unit': 'H', 'desc': 'Phase inductance of the motor', 'required':False})
    
    def __post_init__(self):
        set_parents(self)
    
@dataclass
class PMSM_FOC:
    # PMSM FOC specific parameters
    commutation_offset = FLOAT_PARAMETER(description="Commutation offset", unit="radians", required=False, auto_fill_enable=True)
    # commutation_offset: float = field(default=None, metadata={'unit': 'rad', 'desc': 'Commutation offset in radians', 'required':False})
    # commutation_divider: int = field(default=None, metadata={'desc': 'Commutation divider', 'required':False})
    # torque_constant: float = field(default=None, metadata={'unit': 'Nm/A', 'desc': 'Torque constant of the motor', 'required':False})
    # bemf_constant: float = field(default=None, metadata={'unit': 'V/(rad/s)', 'desc': 'Back EMF constant of the motor', 'required':False})
    # current_loop_bandwidth: float = field(default=None, metadata={'unit': 'Hz', 'desc': 'Current loop bandwidth', 'required':False})
    # current_loop_P_gain: float = field(default=None, metadata={'desc': 'Current loop P gain', 'required':False})
    # current_loop_I_gain: float = field(default=None, metadata={'desc': 'Current loop I gain', 'required':False})
    # current_loop_I_limit: float = field(default=None, metadata={'desc': 'Current loop I limit', 'required':False})

    def __post_init__(self):
        set_parents(self)

@dataclass
class DC_CURRENT:
    # DC current specific parameters
    torque_constant = FLOAT_PARAMETER(description="Torque constant", unit="Nm/A", required=False, auto_fill_enable=True, auto_fill_by=["torque_constant"])
    # torque_constant: float = field(default=None, metadata={'unit': 'Nm/A', 'desc': 'Torque constant of the motor', 'required':False})
    # bemf_constant: float = field(default=None, metadata={'unit': 'V/(rad/s)', 'desc': 'Back EMF constant of the motor', 'required':False})
    # current_loop_bandwidth: float = field(default=None, metadata={'unit': 'Hz', 'desc': 'Current loop bandwidth', 'required':False})
    # current_loop_P_gain: float = field(default=None, metadata={'desc': 'Current loop P gain', 'required':False})
    # current_loop_I_gain: float = field(default=None, metadata={'desc': 'Current loop I gain', 'required':False})
    # current_loop_I_limit: float = field(default=None, metadata={'desc': 'Current loop I limit', 'required':False})

    def __post_init__(self):
        set_parents(self)

@dataclass
class PFC_RECTIFIER:
    # PFC specific parameters
    line_frequency = INT_PARAMETER(description="Line frequency", unit="Hz", required=True, auto_fill_enable=True, auto_fill_by=["line_frequency"])
    # line_frequency: float = field(default=None, metadata={'unit': 'Hz', 'desc': 'Line frequency in Hz', 'required':True})
    # line_voltage: float = field(default=None, metadata={'unit': 'V', 'desc': 'Line voltage in V', 'required':True})
    # max_voltage_fluxuation: float = field(default=10, metadata={'unit': '%', 'desc': 'Max voltage fluxuation in percent', 'required':True})
    # line_positive_current_limit: float = field(default=None, metadata={'unit': 'A', 'desc': 'Max current allowed to be pulled from the line', 'required':True})
    # line_negative_current_limit: float = field(default=0.0, metadata={'unit': 'A', 'desc': 'Max current allowed to be pushed into the line', 'required':True})
    # target_dc_voltage: float = field(default=None, metadata={'unit': 'V', 'desc': 'Target voltage to regulate the DC bus to', 'required':True})
    # current_loop_bandwidth: float = field(default=None, metadata={'unit': 'Hz', 'desc': 'Current loop bandwidth', 'required':False})
    # current_loop_P_gain: float = field(default=None, metadata={'desc': 'Current loop P gain', 'required':False})
    # current_loop_I_gain: float = field(default=None, metadata={'desc': 'Current loop I gain', 'required':False})
    # current_loop_I_limit: float = field(default=None, metadata={'desc': 'Current loop I limit', 'required':False})

    def __post_init__(self):
        set_parents(self)



class EM_HVSD:
    def __init__(self):
        self.module_type = 'motor_drive'
        self.common_params = COMMON()
        self.user_params = USER_PARAMS()
        self.mode_params = {
            self.MODES.PMSM_FOC: PMSM_FOC(),
            self.MODES.DC_CURRENT: DC_CURRENT(),
            self.MODES.PFC_RECTIFIER: PFC_RECTIFIER()
        }
        self.mode = None
        self.connectors = {}

        self.connectors["sto"] = CONNECTOR(description="STO terminals", required=True, exclusive=True, enabled=True, direction="bidirectional", can_connect_to=[CONNECTOR_TYPES.STO_2], type=CONNECTOR_TYPES.STO_2, hardware_identifier="STO")
        self.connectors["serial_bus_A"] = CONNECTOR(description="EM serial bus", required=True, exclusive=True, enabled=True, direction="bidirectional", can_connect_to=[CONNECTOR_TYPES.EM_SERIAL_422], type=CONNECTOR_TYPES.EM_SERIAL_422, hardware_identifier="RS422 A")
        self.connectors["serial_bus_B"] = CONNECTOR(description="EM serial bus", required=False, exclusive=True, enabled=True, direction="bidirectional", can_connect_to=[CONNECTOR_TYPES.EM_SERIAL_422], type=CONNECTOR_TYPES.EM_SERIAL_422, hardware_identifier="RS422 B")
        self.connectors["motor"] = CONNECTOR(description="PMSM(BLDC) motor connection", required=True, exclusive=True, enabled=True, direction="output", can_connect_to=[CONNECTOR_TYPES.MOTOR_PMSM], type=CONNECTOR_TYPES.DRIVE_PMSM, hardware_identifier="UVW")
        self.connectors["pfc_sense"] = CONNECTOR(description="PFC voltage sense", required=False, exclusive=True, enabled=False, direction="input", can_connect_to=[CONNECTOR_TYPES.POWER_AC1, CONNECTOR_TYPES.POWER_AC3, CONNECTOR_TYPES.POWER_DC], hardware_identifier="PFC sense")
        self.connectors["dc_bus"] = CONNECTOR(description="DC bus", required=True, exclusive=False, enabled=True, direction="input", can_connect_to=[CONNECTOR_TYPES.POWER_DC], hardware_identifier="DC bus")
        self.connectors["line"] = CONNECTOR(description="Line AC connection", required=True, exclusive=True, enabled=True, direction="input", can_connect_to=[CONNECTOR_TYPES.POWER_AC1, CONNECTOR_TYPES.POWER_AC3, CONNECTOR_TYPES.POWER_DC], hardware_identifier="UVW")

        # Automatically set the parent reference for all connectors
        for connector in self.connectors.values():
            connector.parent = self
        

    class MODES(Enum):
        PMSM_FOC = auto()
        DC_CURRENT = auto()
        PFC_RECTIFIER = auto()

    def common_parameters(self):
        return self.common_params
    
    def mode_parameters(self):
        return self.mode_params[self.mode]
    
    def set_mode(self, mode):
        
        if mode == self.MODES.PMSM_FOC:
            self.connectors["motor"].enabled = True
            self.connectors["motor"].can_connect_to=[CONNECTOR_TYPES.MOTOR_PMSM]
            self.connectors["motor"].type=CONNECTOR_TYPES.DRIVE_PMSM
            self.connectors["motor"].exclusive = True
            self.connectors["motor"].description = "PMSM(BLDC) motor connection"
            self.connectors["pfc_sense"].enabled = False
            self.connectors["dc_bus"].direction = "input"
            self.connectors["line"].enabled = False
            self.mode = mode

        elif mode == self.MODES.DC_CURRENT:
            self.connectors["motor"].enabled = True
            self.connectors["motor"].can_connect_to=[CONNECTOR_TYPES.MOTOR_DC]
            self.connectors["motor"].type=CONNECTOR_TYPES.DRIVE_DC
            self.connectors["motor"].exclusive = False
            self.connectors["motor"].description = "DC motor connection"
            self.connectors["pfc_sense"].enabled = False
            self.connectors["dc_bus"].direction = "input"
            self.connectors["line"].enabled = False
            self.mode = mode

        elif mode == self.MODES.PFC_RECTIFIER:
            self.connectors["motor"].enabled = False
            self.connectors["pfc_sense"].enabled = True
            self.connectors["dc_bus"].direction = "output"
            self.connectors["line"].enabled = True
            self.mode = mode
        else:
            raise ValueError(f'Invalid mode: {mode}')

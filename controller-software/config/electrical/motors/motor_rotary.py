from dataclasses import dataclass, field
from enum import Enum, auto
import sys
import os

motor_drives_dir = os.path.dirname(os.path.realpath(__file__))
electrical_dir = os.path.dirname(motor_drives_dir)

sys.path.append(electrical_dir)

from parameters import TEXT_PARAMETER, FLOAT_PARAMETER, INT_PARAMETER, set_parents
from connectors import CONNECTOR, CONNECTOR_TYPES
from user_params import USER_PARAMS


@dataclass
class CONNECTORS:
    # Motor connectors
    motor: None = field(default=None, metadata={'desc': 'What is connected to the motor', 'required':True, 'chainable':False})

@dataclass
class COMMON:
    # Common motor parameters
    rated_speed: FLOAT_PARAMETER = field(default_factory=lambda: FLOAT_PARAMETER(description="Rated speed of the motor", unit="RPM", required=True))
    rated_voltage: FLOAT_PARAMETER = field(default_factory=lambda: FLOAT_PARAMETER(description="Rated voltage of the motor", unit="Volts", required=True))
    hard_max_current: FLOAT_PARAMETER = field(default_factory=lambda: FLOAT_PARAMETER(description="Hard maximum current of the motor, beyond which the motor will be demagnetized", unit="Amps", required=True))
    
    # rated_speed: float = field(default=None, metadata={'unit': 'RPM', 'desc': 'Rated speed of the motor', 'required':True})
    # max_speed: float = field(default=None, metadata={'unit': 'RPM', 'desc': 'Maximum speed of the motor', 'required':False})
    # rated_voltage: float = field(default=None, metadata={'unit': 'V', 'desc': 'Rated voltage of the motor', 'required':True})
    # rated_current: float = field(default=None, metadata={'unit': 'A', 'desc': 'Rated current of the motor', 'required':True})
    # soft_max_current: float = field(default=None, metadata={'unit': 'A', 'desc': 'Soft maximum current of the motor', 'required':False})
    # hard_max_current: float = field(default=None, metadata={'unit': 'A', 'desc': 'Hard maximum current of the motor, beyond which the motor will be demagnetized', 'required':True})
    # torque_constant: float = field(default=None, metadata={'unit': 'Nm/A', 'desc': 'Torque constant of the motor', 'required':False})
    # bemf_constant: float = field(default=None, metadata={'unit': 'V/(rad/s)', 'desc': 'Back EMF constant of the motor', 'required':False})
    # phase_resistance: float = field(default=None, metadata={'unit': 'Ohm', 'desc': 'Phase resistance of the motor', 'required':False})
    # phase_inductance: float = field(default=None, metadata={'unit': 'H', 'desc': 'Phase inductance of the motor', 'required':False})
    # inertia: float = field(default=None, metadata={'unit': 'kg*m^2', 'desc': 'Inertia of the motor', 'required':False})
    # thermal_time_constant: float = field(default=None, metadata={'unit': 's', 'desc': 'Thermal time constant of the motor', 'required':False})
    # thermal_resistance: float = field(default=None, metadata={'unit': 'K/W', 'desc': 'Thermal resistance of the motor to ambient', 'required':False})
    # max_temperature: float = field(default=None, metadata={'unit': 'C', 'desc': 'Maximum temperature of the motor', 'required':False})

    def __post_init__(self):
        set_parents(self)

@dataclass
class PMSM:
    # PMSM specific parameters
    pole_pairs = INT_PARAMETER(description="Number of pole pairs in the motor", required=False)
    # pole_pairs: int = field(default=None, metadata={'desc': 'Number of pole pairs in the motor', 'required':False})

    def __post_init__(self):
        set_parents(self)

@dataclass
class DC:
    # DC specific parameters
    pass

    def __post_init__(self):
        set_parents(self)

# @dataclass
# class STEPPER_BIPOLAR:
#     # Bipolar stepper specific parameters
#     steps_per_rev: int = field(default=None, metadata={'desc': 'Number of steps per revolution of the motor', 'required':True})

# @dataclass
# class STEPPER_UNIPOLAR:
#     # Unipolar stepper specific parameters
#     steps_per_rev: int = field(default=None, metadata={'desc': 'Number of steps per revolution of the motor', 'required':True})

# @dataclass
# class INDUCTION:
#     # Induction motor specific parameters
#     #TODO: Add induction motor specific parameters
#     pass




class ROTARY_MOTOR:
    def __init__(self):
        self.module_type = 'motor'
        self.common_params = COMMON()
        self.user_params = USER_PARAMS()
        self.mode_params = {
            self.MODES.PMSM: PMSM(),
            self.MODES.DC: DC()
        }
        self.mode = None
        self.connectors = {}

        self.connectors["motor"] = CONNECTOR(description="Motor terminals", required=True, exclusive=True, enabled=True, direction="input")

        # Automatically set the parent reference in child instances
        for connector in self.connectors.values():
            connector.parent = self

    class MODES(Enum):
        PMSM = auto()
        DC = auto()

    def common_parameters(self):
        return self.common_params
    
    def mode_parameters(self):
        return self.mode_params[self.mode]

    def set_mode(self, mode: MODES):
        if mode == self.MODES.PMSM:
            self.connectors["motor"].can_connect_to=[CONNECTOR_TYPES.DRIVE_PMSM]
            self.connectors["motor"].type=CONNECTOR_TYPES.MOTOR_PMSM
            self.connectors["motor"].exclusive = True
            self.connectors["motor"].hardware_identifier = "UVW"
            self.mode = mode
        elif mode == self.MODES.DC:
            self.connectors["motor"].can_connect_to=[CONNECTOR_TYPES.DRIVE_DC, CONNECTOR_TYPES.POWER_DC]
            self.connectors["motor"].type=CONNECTOR_TYPES.MOTOR_DC
            self.connectors["motor"].exclusive = False
            self.connectors["motor"].hardware_identifier = "+ -"
            self.mode = mode
        else:
            raise ValueError('Motor type not supported')
        
        self.connectors["motor"].is_connected_to = []
            
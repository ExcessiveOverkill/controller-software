from dataclasses import dataclass, field

class LINEAR_MOTOR:
    def __init__(self):
        self.types = [self.PMAC]
        self.motor_type = None
        self.user_params = None


    @dataclass
    class COMMON:
        # Common motor parameters
        rated_speed: float = field(default=None, metadata={'unit': 'm/s', 'desc': 'Rated speed of the motor', 'required':True})
        max_speed: float = field(default=None, metadata={'unit': 'm/s', 'desc': 'Maximum speed of the motor', 'required':False})
        rated_voltage: float = field(default=None, metadata={'unit': 'V', 'desc': 'Rated voltage of the motor', 'required':True})
        rated_current: float = field(default=None, metadata={'unit': 'A', 'desc': 'Rated current of the motor', 'required':True})
        soft_max_current: float = field(default=None, metadata={'unit': 'A', 'desc': 'Soft maximum current of the motor', 'required':False})
        hard_max_current: float = field(default=None, metadata={'unit': 'A', 'desc': 'Hard maximum current of the motor, beyond which the motor will be damaged', 'required':True})
        force_constant: float = field(default=None, metadata={'unit': 'N/A', 'desc': 'Force constant of the motor', 'required':False})
        bemf_constant: float = field(default=None, metadata={'unit': 'V/(m/s)', 'desc': 'Back EMF constant of the motor', 'required':False})
        phase_resistance: float = field(default=None, metadata={'unit': 'Ohm', 'desc': 'Phase resistance of the motor', 'required':False})
        phase_inductance: float = field(default=None, metadata={'unit': 'H', 'desc': 'Phase inductance of the motor', 'required':False})
        mass: float = field(default=None, metadata={'unit': 'kg', 'desc': 'Mass of the moving part of the motor', 'required':False})
        thermal_time_constant: float = field(default=None, metadata={'unit': 's', 'desc': 'Thermal time constant of the motor', 'required':False})
        thermal_resistance: float = field(default=None, metadata={'unit': 'K/W', 'desc': 'Thermal resistance of the motor to ambient', 'required':False})
        max_temperature: float = field(default=None, metadata={'unit': 'C', 'desc': 'Maximum temperature of the motor', 'required':False})

    @dataclass
    class PMAC:
        # PMAC specific parameters
        pole_distance: int = field(default=None, metadata={'desc': 'Distance between poles on the motor', 'required':False})

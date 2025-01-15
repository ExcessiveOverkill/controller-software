from dataclasses import dataclass, field

class POWER_SOURCE:
    def __init__(self):
        self.types = [self.AC1, self.AC3, self.DC, self.BATTERY]
        self.supply_type = None
        self.user_params = None

    @dataclass
    class COMMON:
        # common power source parameters
        nominal_voltage: float = field(default=None, metadata={'unit': 'V', 'desc': 'Nominal voltage of the power source', 'required':True})
        max_positive_current: float = field(default=None, metadata={'unit': 'A', 'desc': 'Maximum current allowed to be pulled from the supply', 'required':True})
        max_negative_current: float = field(default=0.0, metadata={'unit': 'A', 'desc': 'Maximum current allowed to be pushed to the supply', 'required':True})

    @dataclass
    class AC1:
        # AC1 specific parameters
        line_frequency: float = field(default=None, metadata={'unit': 'Hz', 'desc': 'Line frequency in Hz', 'required':True})
        allowed_voltage_fluxuation: float = field(default=10, metadata={'unit': '%', 'desc': 'Max voltage fluxuation in percent', 'required':True})

    @dataclass
    class AC3:
        # AC3 specific parameters
        line_frequency: float = field(default=None, metadata={'unit': 'Hz', 'desc': 'Line frequency in Hz', 'required':True})
        allowed_voltage_fluxuation: float = field(default=10, metadata={'unit': '%', 'desc': 'Max voltage fluxuation in percent', 'required':True})

    @dataclass
    class DC:
        # DC specific parameters
        allowed_voltage_fluxuation: float = field(default=10, metadata={'unit': '%', 'desc': 'Max voltage fluxuation in percent', 'required':True})

    @dataclass
    class BATTERY:
        # Battery specific parameters
        max_voltage: float = field(default=None, metadata={'unit': 'V', 'desc': 'Maximum voltage of the battery', 'required':True})
        min_voltage: float = field(default=None, metadata={'unit': 'V', 'desc': 'Minimum voltage of the battery', 'required':True})
        over_under_voltage_trip_time: float = field(default=5, metadata={'unit': 's', 'desc': 'Time before the battery trips on over/under voltage', 'required':True})
        max_temperature: float = field(default=None, metadata={'unit': 'C', 'desc': 'Maximum temperature allowed for the battery', 'required':False})
        min_temperature: float = field(default=None, metadata={'unit': 'C', 'desc': 'Minimum temperature allowed for the battery', 'required':False})
        cell_count: int = field(default=None, metadata={'desc': 'Number of cells in the battery', 'required':False})
        chemistry: str = field(default=None, metadata={'desc': 'Chemistry of the battery', 'required':False})
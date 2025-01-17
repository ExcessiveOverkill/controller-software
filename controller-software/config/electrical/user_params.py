from dataclasses import dataclass, field


# default param options to add to every device class

@dataclass
class USER_PARAMS:

    # User defined parameters
    name: str = field(default="PMAC Motor", metadata={'desc': 'Name of the motor', 'required':False})
    model_number: str = field(default=None, metadata={'desc': 'Model number of the motor', 'required':False})
    manufacturer: str = field(default=None, metadata={'desc': 'Manufacturer of the motor', 'required':False})
    location: str = field(default=None, metadata={'desc': 'Location of the motor', 'required':False})
    description: str = field(default=None, metadata={'desc': 'Description of the motor', 'required':False})
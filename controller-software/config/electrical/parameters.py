from dataclasses import dataclass, field, asdict
import math
from typing import Any

def set_parents(parent):
    # Automatically set the parent reference in child instances
    for field_name in parent.__dataclass_fields__:
        value = getattr(parent, field_name)
        if hasattr(value, 'parent'):
            value.parent = parent

@dataclass
class TEXT_PARAMETER:
    description: str = field(default="", metadata={'desc': 'Description'})
    required: bool = field(default=False, metadata={'desc': 'Required for system to startup (not able to be auto calibrated or calculated)'})
    value: str = field(default=None, metadata={'desc': 'Input text'})
    auto_fill_enable: bool = field(default=False, metadata={'desc': 'Locked from being auto filled'})
    manual_fill_enable: bool = field(default=True, metadata={'desc': 'Locked from being manually filled'})
    max_length: int = field(default=100, metadata={'desc': 'Maximum length of the text'})
    auto_fill_by: list = field(default_factory=list, metadata={'desc': 'External parameters that can auto fill this parameter'})
    
    parent: Any = field(default=None, repr=False, compare=False, metadata={'exclude': True})


@dataclass
class FLOAT_PARAMETER:
    description: str = field(default="", metadata={'desc': 'Description'})
    unit: str = field(default="", metadata={'desc': 'Unit of the parameter'})
    required: bool = field(default=False, metadata={'desc': 'Required for system to startup (not able to be auto calibrated or calculated)'})
    value: float = field(default=None, metadata={'desc': 'Input value'})
    auto_fill_enable: bool = field(default=False, metadata={'desc': 'Locked from being auto filled'})
    manual_fill_enable: bool = field(default=True, metadata={'desc': 'Locked from being manually filled'})
    max_value: float = field(default=math.inf, metadata={'desc': 'Maximum value'})
    min_value: float = field(default=-math.inf, metadata={'desc': 'Minimum value'})
    auto_fill_by: list = field(default_factory=list, metadata={'desc': 'External parameters that can auto fill this parameter'})

    parent: Any = field(default=None, repr=False, compare=False, metadata={'exclude': True})

@dataclass
class INT_PARAMETER:
    description: str = field(default="", metadata={'desc': 'Description'})
    unit: str = field(default="", metadata={'desc': 'Unit of the parameter'})
    required: bool = field(default=False, metadata={'desc': 'Required for system to startup (not able to be auto calibrated or calculated)'})
    value: int = field(default=None, metadata={'desc': 'Input value'})
    auto_fill_enable: bool = field(default=False, metadata={'desc': 'Locked from being auto filled'})
    manual_fill_enable: bool = field(default=True, metadata={'desc': 'Locked from being manually filled'})
    max_value: int = field(default=math.inf, metadata={'desc': 'Maximum value'})
    min_value: int = field(default=-math.inf, metadata={'desc': 'Minimum value'})
    auto_fill_by: list = field(default_factory=list, metadata={'desc': 'External parameters that can auto fill this parameter'})

    parent: Any = field(default=None, repr=False, compare=False, metadata={'exclude': True})


# User defined parameters that aren't device specific
@dataclass
class USER_PARAMS:

    name: TEXT_PARAMETER = field(default_factory=lambda: TEXT_PARAMETER(description="Name of the device", required=True, max_length=25))
    model_number: TEXT_PARAMETER = field(default_factory=lambda: TEXT_PARAMETER(description="Model number of the device", max_length=100))
    manufacturer: TEXT_PARAMETER = field(default_factory=lambda: TEXT_PARAMETER(description="Manufacturer of the device", max_length=100))
    location: TEXT_PARAMETER = field(default_factory=lambda: TEXT_PARAMETER(description="Location of the device", max_length=100))
    description: TEXT_PARAMETER = field(default_factory=lambda: TEXT_PARAMETER(description="Description of the device", max_length=1000))

    def __post_init__(self):
        set_parents(self)
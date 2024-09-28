from dataclasses import dataclass, field, asdict, is_dataclass, fields
from enum import Enum, auto
from typing import List, Any


class CONNECTOR_TYPES(Enum):

    # Motors
    MOTOR_PMSM = auto()
    MOTOR_DC = auto()
    MOTOR_STEPPER_BIPOLAR = auto()
    MOTOR_STEPPER_UNIPOLAR = auto()
    MOTOR_INDUCTION = auto()

    # Drives
    DRIVE_PMSM = auto()
    DRIVE_DC = auto()
    DRIVE_STEPPER_BIPOLAR = auto()
    DRIVE_STEPPER_UNIPOLAR = auto()
    DRIVE_INDUCTION = auto()

    # Power sources
    POWER_DC = auto()
    POWER_AC1 = auto()
    POWER_AC3 = auto()

    # IO
    OUTPUT_DIGITAL_DC = auto()
    INPUT_DIGITAL_DC = auto()
    OUTPUT_ANALOG = auto()
    INPUT_ANALOG = auto()

    # Communication
    EM_SERIAL_485 = auto()
    EM_SERIAL_422 = auto()

    # Safety
    STO_2 = auto()    # 2 ch sto with feedback

    ANY = auto()


@dataclass
class CONNECTOR:
    description: str = field(default="Default description", metadata={'desc': 'What is connected to the motor connector'})
    required: bool = field(default=False, metadata={'desc': 'Is the connector required to be connected'})
    exclusive: bool = field(default=False, metadata={'desc': 'Only allowed to connect to a single other connector (no branches)'})
    enabled: bool = field(default=False, metadata={'desc': 'Is the connector required to be connected'})
    direction: str = field(default="input", metadata={'desc': 'Typical direction of the connection (input/output/bidirectional)'})
    can_connect_to: list = field(default_factory=list, metadata={'desc': 'List of connectors that this connector can connect to'})
    is_connected_to: list = field(default_factory=list, metadata={'desc': 'List of connectors that this connector is connected to'})
    hardware_identifier: str = field(default="None", metadata={'desc': 'ID of connector on hardware'})
    type: CONNECTOR_TYPES = field(default=CONNECTOR_TYPES.ANY, metadata={'desc': 'Type of connector'})

    parent: Any = field(default=None, repr=False, compare=False, metadata={'exclude': True})


class CONNECTOR_FUNCTIONS:
    @staticmethod
    def validate_connect_to(own_connector: CONNECTOR, external_connectors: list[CONNECTOR]):
        
        if own_connector not in own_connector.parent.connectors.values():
            raise ValueError(f'Connector not found in device ({own_connector.description})')
        
        if not own_connector.enabled:
            return [False, "Source connector is disabled"]
        
        if own_connector.exclusive and len(external_connectors) > 1:
            return [False, "Source connector can't connect to multiple connectors"]
        
        for external_connector in external_connectors:

            if own_connector == external_connector:
                return [False, "Source connector can't connect to itself"]
            
            if external_connector in own_connector.is_connected_to or own_connector in external_connector.is_connected_to:
                if not external_connector in own_connector.is_connected_to or not own_connector in external_connector.is_connected_to:
                    return [False, "Connections between devices are not synced"]    # TODO: handle this case better
                return [False, "Source connector is already connected to target connector"]
            
            if not external_connector.enabled:
                return [False, "Source connector is disabled"]
            
            if external_connector.exclusive and len(external_connectors) > 1:
                return [False, f"Target connector ({external_connector.description}) can't connect to multiple connectors"]

            if external_connector.type not in own_connector.can_connect_to:
                return [False, f"Target connector type mismatch, {own_connector.description} can't connect to {external_connector.type}"]

            if own_connector.exclusive:
                if own_connector.direction == "input" and external_connector.direction == "input":
                    return [False, "Source connector is an input and target connector is an input, one must be an output"]
                elif own_connector.direction == "output" and external_connector.direction == "output":
                    return [False, "Source connector is an output and target connector is an output, one must be an input"]

        return [True, ""]
    
    @staticmethod
    def connect_to(own_connector: CONNECTOR, external_connectors: list[CONNECTOR]):
        
        success, error = CONNECTOR_FUNCTIONS.validate_connect_to(own_connector, external_connectors)
        
        if not success:
            return [False, f"Connector validation failed ({error})"]
        
        for external_connector in external_connectors:
            own_connector.is_connected_to.append(external_connector)
            external_connector.is_connected_to.append(own_connector)
        
        
        return [True, ""]
    
    @staticmethod
    def disconnect_from(own_connector: CONNECTOR, external_connectors: list[CONNECTOR]):
        
        error = ""

        for external_connector in external_connectors:
            if external_connector in own_connector.is_connected_to:
                own_connector.is_connected_to.remove(external_connector)
            else:
                error += f"Connector {own_connector.description} is not connected to {external_connector.description}, nothing to disconnect\n"
            
            if own_connector in external_connector.is_connected_to:
                external_connector.is_connected_to.remove(own_connector)
            else:
                error += f"Connector {external_connector.description} is not connected to {own_connector.description}, nothing to disconnect\n"
        
        if error != "":
            return [False, error]
        
        return [True, ""]
    
    @staticmethod
    def update_from_connector(own_connector: CONNECTOR, external_connector: CONNECTOR = None):
        if external_connector is None and len(own_connector.is_connected_to) > 1:
            return [False, "Multiple connections, please specify external connector to update from"]
        
        if own_connector not in own_connector.parent.connectors.values():
            raise ValueError(f'Connector not found in device ({external_connector.parent.module_type}/{own_connector.description})')
        
        if len(own_connector.is_connected_to) == 0:
            return [False, "Connector is not connected to anything"]
        
        update_count = 0

        if external_connector.parent.module_type == "motor":
            for own_param in fields(own_connector.parent.common_params):
                own_param_name = own_param.name
                own_param_data = getattr(own_connector.parent.common_params, own_param_name)

                if own_param_data.auto_fill_enable:
                    for auto_fill_source in own_param_data.auto_fill_by:
                        for external_param in fields(external_connector.parent.common_params):
                            external_param_name = external_param.name
                            if auto_fill_source == external_param_name:
                                getattr(own_connector.parent.common_params, own_param_name).value = getattr(external_connector.parent.common_params, auto_fill_source).value
                                update_count += 1
                                print(f"Updated {own_param_name} to {getattr(own_connector.parent.common_params, own_param_name).value}")
        else:
            return [False, "Cannot auto update from type: " + external_connector.parent.module_type]
            
        if update_count == 1:
            return [True, f"{update_count} value updated"]
        return [True, f"{update_count} values updated"]
        
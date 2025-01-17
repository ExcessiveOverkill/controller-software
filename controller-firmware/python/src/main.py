import os, json




# create required files for a given build config

def generate_config(build_config_path:str):
    # make sure config file exists
    if not os.path.exists(build_config_path):
        print("Config file not found")
        return
    
import logging

DEFAULT_CONFIG = {
    "server-name": "CrossCraft Server",
    "motd": "A CrossCraft Server",
    "public": "false",
    "port": "25565",
    "server-ip": "0.0.0.0",
    "max-players": "10",
    "max-connections": "3",
}
CONFIG = {}

def load_properties():
    global CONFIG
    try:
        with open("server.properties", "r") as f:
            for line in f:
                if "=" in line and not line.startswith("#"):
                    key, value = line.strip().split("=", 1)
                    CONFIG[key] = value
                    logging.info(f"Loaded property: {key} = {value}")

    except FileNotFoundError:
        logging.warning("server.properties not found, creating a new one.")
        with open("server.properties", "w") as f:
            for key, value in DEFAULT_CONFIG.items():
                f.write(f"{key}={value}\n")
        CONFIG = DEFAULT_CONFIG.copy()

def get_property(key):
    return CONFIG.get(key, DEFAULT_CONFIG.get(key))

def get_int_property(key):
    return int(get_property(key))

def get_bool_property(key):
    return get_property(key).lower() == "true"

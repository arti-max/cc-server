import logging
import os

DEFAULT_CONFIG = {
    "server-name": "CrossCraft Server",
    "motd": "A CrossCraft Server",
    "port": "25565",
    "public": "true",
    "verify-names": "true",
    "max-players": "16",
    "max-connections": "3",
}

CONFIG_ORDER = [
    "server-name", "motd", "port", "public", "verify-names", 
    "max-players", "max-connections"
]

CONFIG = {}

def load_properties():
    global CONFIG
    
    CONFIG = DEFAULT_CONFIG.copy()
    
    properties_file = "server.properties"
    
    if os.path.exists(properties_file):
        try:
            with open(properties_file, "r", encoding="utf-8") as f:
                for line in f:
                    line = line.strip()
                    if line and "=" in line and not line.startswith("#"):
                        key, value = line.split("=", 1)
                        CONFIG[key.strip()] = value.strip()
            logging.info("Loaded server.properties")
        except Exception as e:
            logging.error(f"Failed to load server.properties: {e}")

    try:
        with open(properties_file, "w", encoding="utf-8") as f:
            f.write("#Minecraft server properties\n")
            f.write("#" +  str(os.path.getmtime(properties_file) if os.path.exists(properties_file) else "") + "\n")
            
            for key in CONFIG_ORDER:
                value = CONFIG.get(key, DEFAULT_CONFIG.get(key, ""))
                f.write(f"{key}={value}\n")
                
            for key, value in CONFIG.items():
                if key not in CONFIG_ORDER:
                    f.write(f"{key}={value}\n")
                    
        logging.info("Saved server.properties")
    except Exception as e:
        logging.error(f"Failed to save server.properties: {e}")

def get_property(key):
    return CONFIG.get(key, DEFAULT_CONFIG.get(key, ""))

def get_int_property(key):
    try:
        return int(get_property(key))
    except ValueError:
        return int(DEFAULT_CONFIG.get(key, 0))

def get_bool_property(key):
    return get_property(key).lower() == "true"

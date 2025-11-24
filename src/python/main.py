import logging
import threading
import sys
import requests
from network import start_server, broadcast
from world_manager import init_world, tick_world_loop, save_world_periodically, shutdown_server
from config import load_properties, get_property, get_int_property
from ban_manager import load_bans
from heartbeat import heartbeat_loop, send_heartbeat
from admin_manager import load_admins

def get_public_ip():
    try:
        response = requests.get('https://api.ipify.org', timeout=5)
        response.raise_for_status()
        ip = response.text.strip()
        logging.info(f"Successfully detected public IP: {ip}")
        return ip
    except requests.exceptions.RequestException as e:
        logging.warning(f"Could not automatically detect public IP address: {e}")
        logging.warning("externalurl.txt will contain a placeholder. You may need to enter your IP manually.")
        return None

if __name__ == "__main__":
    load_properties()
    load_bans()
    load_admins()
    
    port = get_int_property("port")
    public_ip = get_public_ip()
    if not public_ip:
        public_ip = "0.0.0.0"
        
    with open("externalurl.txt", "w") as f:
        f.write(f"http://crosscraftweb.ddns.net/play.jsp?server={public_ip}&port={port}\n")
    
    log_format = '[%(asctime)s - %(threadName)s - %(levelname)s] %(message)s'
    root_logger = logging.getLogger()
    root_logger.setLevel(logging.INFO)

    if root_logger.hasHandlers():
        root_logger.handlers.clear()

    log_handler = logging.StreamHandler(sys.stdout)
    log_handler.setFormatter(logging.Formatter(log_format))
    root_logger.addHandler(log_handler)
    
    logging.info("Server is starting...")
    init_world(256, 256, 64)
    
    tick_thread = threading.Thread(target=tick_world_loop, name="WorldTickThread", daemon=True)
    tick_thread.start()
    save_thread = threading.Thread(target=save_world_periodically, name="WorldSaveThread", daemon=True)
    save_thread.start()
    heartbeat_thread = threading.Thread(target=heartbeat_loop, name="HeartbeatThread", daemon=True)
    heartbeat_thread.start()
    logging.info("All background threads started.")
    
    try:
        start_server()
    finally:
        shutdown_server()

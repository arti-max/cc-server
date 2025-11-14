import logging
import threading
import sys
from network import start_server, broadcast
from world_manager import init_world, tick_world_loop, save_world_periodically, shutdown_server
from config import load_properties, get_property, get_int_property
from ban_manager import load_bans

if __name__ == "__main__":
    load_properties()
    load_bans()
    
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
    logging.info("C++ world tick loop started in a separate thread.")
    
    try:
        start_server()
    finally:
        shutdown_server()

import requests
import time
from config import *
from player_manager import get_all_players

HEARTBEAT_URL = "http://crosscraftweb.ddns.net/heartbeat.jsp"

def send_heartbeat():
    try:
        payload = {
            "name": get_property("server-name"),
            "users": len(get_all_players()),
            "max": get_int_property("max-players"),
            "public": str(get_bool_property("public")).lower(),
            "port": get_int_property("port"),
        }
        
        response = requests.post(HEARTBEAT_URL, data=payload, timeout=10)
        response.raise_for_status()
        
        logging.info(f"Heartbeat sent successfully. Status: OK")

    except requests.exceptions.RequestException as e:
        logging.warning(f"Could not send heartbeat to master server: {e}")
    except Exception as e:
        logging.error(f"An unexpected error occurred in send_heartbeat: {e}")

def heartbeat_loop():
    logging.info("Heartbeat thread started. Will send updates every 60 seconds")
    while True:
        send_heartbeat()
        time.sleep(60)
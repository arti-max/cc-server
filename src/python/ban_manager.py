import logging

BANNED_USERS = set()
BANNED_IPS = set()

def load_bans():
    try:
        with open("banned.txt", "r") as f:
            BANNED_USERS.update(line.strip() for line in f if line.strip())
        logging.info(f"Loaded {len(BANNED_USERS)} banned users.")
    except FileNotFoundError:
        logging.warning("banned.txt not found, creating empty file.")
        open("banned.txt", "w").close()

    try:
        with open("banned-ip.txt", "r") as f:
            BANNED_IPS.update(line.strip() for line in f if line.strip())
        logging.info(f"Loaded {len(BANNED_IPS)} banned IPs.")
    except FileNotFoundError:
        logging.warning("banned-ip.txt not found, creating empty file.")
        open("banned-ip.txt", "w").close()

def is_user_banned(username):
    return username.lower() in BANNED_USERS

def is_ip_banned(ip_address):
    return ip_address in BANNED_IPS

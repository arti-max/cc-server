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


def add_ban(username):
    username_lower = username.lower()
    if username_lower not in BANNED_USERS:
        BANNED_USERS.add(username_lower)
        try:
            with open("banned.txt", "a") as f:
                f.write(f"\n{username}")
            logging.info(f"User '{username}' has been banned and saved to file.")
            return True
        except IOError as e:
            logging.error(f"Could not write to banned.txt: {e}")
            return False
    return False

def add_ip_ban(ip_address):
    if ip_address not in BANNED_IPS:
        BANNED_IPS.add(ip_address)
        try:
            with open("banned-ip.txt", "a") as f:
                f.write(f"\n{ip_address}")
            logging.info(f"IP Address '{ip_address}' has been banned and saved to file.")
            return True
        except IOError as e:
            logging.error(f"Could not write to banned-ip.txt: {e}")
            return False
    return False

def unban_user(username):
    username_lower = username.lower()
    if username_lower in BANNED_USERS:
        BANNED_USERS.remove(username_lower)
        try:
            with open("banned.txt", "w") as f:
                for user in BANNED_USERS:
                    f.write(f"{user}\n")
            logging.info(f"User '{username}' unbanned.")
            return True
        except IOError:
            return False
    return False
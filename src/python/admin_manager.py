import logging

ADMINS = set()

def load_admins():
    global ADMINS
    try:
        with open("admins.txt", "r") as f:
            ADMINS = {line.strip().lower() for line in f if line.strip()}
        logging.info(f"Loaded {len(ADMINS)} admins.")
    except FileNotFoundError:
        logging.warning("admins.txt not found, creating an empty one. No one is an admin.")
        open("admins.txt", "w").close()

def is_admin(username):
    return username.lower() in ADMINS

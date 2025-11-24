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

def add_admin(username):
    username_lower = username.lower()
    if username_lower not in ADMINS:
        ADMINS.add(username_lower)
        try:
            with open("admins.txt", "a") as f:
                f.write(f"\n{username}")
            logging.info(f"User '{username}' is now an admin.")
            return True
        except IOError:
            return False
    return False

def remove_admin(username):
    username_lower = username.lower()
    if username_lower in ADMINS:
        ADMINS.remove(username_lower)
        try:
            with open("admins.txt", "w") as f:
                for admin in ADMINS:
                    f.write(f"{admin}\n")
            logging.info(f"User '{username}' is no longer an admin.")
            return True
        except IOError:
            return False
    return False
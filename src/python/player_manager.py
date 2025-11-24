import logging
import re
import os

class Player:
    def __init__(self, websocket, player_id, username):
        self.ws = websocket
        self.id = player_id
        self.username = username
        self.x = 0.0
        self.y = 0.0
        self.z = 0.0
        self.yaw = 0.0
        self.pitch = 0.0

    def set_position(self, x, y, z, yaw, pitch):
        self.x, self.y, self.z = x, y, z
        self.yaw, self.pitch = yaw, pitch

PLAYERS = {}
SESSIONS = {}
next_player_id = 1

def update_logged_in_file():
    try:
        with open("logged-in.txt", "w", encoding="utf-8") as f:
            for player in PLAYERS.values():
                f.write(f"{player.username}\n")
    except IOError as e:
        logging.error(f"Failed to update logged-in.txt: {e}")

def get_all_players():
    return len(PLAYERS)

def is_username_taken(username):
    lower_username = username.lower()
    for player in PLAYERS.values():
        if player.username.lower() == lower_username:
            return True
    return False

def validate_session(username, session_id):
    if not session_id or len(session_id) < 16:
        return False, "Invalid session ID."
        
    if session_id in SESSIONS and SESSIONS[session_id].lower() != username.lower():
        return False, "Session ID is already in use by another player."
        
    return True, ""

def validate_username(username):
    if len(username) < 2 or len(username) > 16:
        return False
    if not re.match(r"^[a-zA-Z0-9_-]+$", username):
        return False
    return True

def add_player(websocket, username, session_id):
    global next_player_id
    player_id = next_player_id
    next_player_id += 1
    
    new_player = Player(websocket, player_id, username)
    PLAYERS[websocket] = new_player
    SESSIONS[session_id] = username
    update_logged_in_file()
    logging.info(f"Player '{username}' (ID: {player_id}) added to the game.")
    return new_player

def remove_player(websocket):
    if websocket in PLAYERS:
        player = PLAYERS.pop(websocket)
        session_to_remove = None
        for sid, uname in SESSIONS.items():
            if uname.lower() == player.username.lower():
                session_to_remove = sid
                break
        if session_to_remove:
            del SESSIONS[session_to_remove]   
        update_logged_in_file()
        logging.info(f"Player '{player.username}' (ID: {player.id}) removed.")
        return player
    return None

def get_player(websocket):
    return PLAYERS.get(websocket)

def get_all_players():
    return list(PLAYERS.values())

def find_player_by_username(username):
    lower_username = username.lower()
    for player in PLAYERS.values():
        if player.username.lower() == lower_username:
            return player
    return None

import logging

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
next_player_id = 1

def add_player(websocket, username):
    global next_player_id
    player_id = next_player_id
    next_player_id += 1
    
    new_player = Player(websocket, player_id, username)
    PLAYERS[websocket] = new_player
    logging.info(f"Player '{username}' (ID: {player_id}) added to the game.")
    return new_player

def remove_player(websocket):
    if websocket in PLAYERS:
        player = PLAYERS.pop(websocket)
        logging.info(f"Player '{player.username}' (ID: {player.id}) removed.")
        return player
    return None

def get_player(websocket):
    return PLAYERS.get(websocket)

def get_all_players():
    return list(PLAYERS.values())

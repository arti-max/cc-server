# --- External Imports ---

import asyncio
import ctypes
import gzip
import logging
import platform
import struct
import sys
import threading
import time
import websockets

# --- Content from ban_manager.py ---


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


# --- Content from config.py ---


DEFAULT_CONFIG = {
    "server-name": "CrossCraft Server",
    "motd": "A CrossCraft Server",
    "public": "false",
    "port": "25565",
    "server-ip": "0.0.0.0",
    "max-players": "10",
}
CONFIG = {}

def load_properties():
    global CONFIG
    try:
        with open("server.properties", "r") as f:
            for line in f:
                if "=" in line and not line.startswith("#"):
                    key, value = line.strip().split("=", 1)
                    CONFIG[key] = value
                    logging.info(f"Loaded property: {key} = {value}")

    except FileNotFoundError:
        logging.warning("server.properties not found, creating a new one.")
        with open("server.properties", "w") as f:
            for key, value in DEFAULT_CONFIG.items():
                f.write(f"{key}={value}\n")
        CONFIG = DEFAULT_CONFIG.copy()

def get_property(key):
    return CONFIG.get(key, DEFAULT_CONFIG.get(key))

def get_int_property(key):
    return int(get_property(key))

def get_bool_property(key):
    return get_property(key).lower() == "true"


# --- Content from player_manager.py ---


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


# --- Content from packets.py ---


PROTOCOL_VERSION = 1


def create_server_identification_packet():
    packet_id = 0x11
    name_bytes = get_property('server-name').encode('utf-8')
    motd_bytes = get_property('motd').encode('utf-8')
    
    packet_data = struct.pack('!B', PROTOCOL_VERSION) + \
                  struct.pack('!h', len(name_bytes)) + name_bytes + \
                  struct.pack('!h', len(motd_bytes)) + motd_bytes
    
    return struct.pack('!B', packet_id) + packet_data

def create_login_response_packet():
    return struct.pack('!B', 0x10)

def create_level_data_packet(width, height, depth, compressed_world_data):
    if not compressed_world_data:
        raise ValueError("World data for packet is not available.")
        
    header = struct.pack('!hhh', width, height, depth) + \
             struct.pack('!i', len(compressed_world_data))
             
    return struct.pack('!B', 0x12) + header + compressed_world_data

def create_block_update_packet(x, y, z, block_type):
    packet_id = 0x17
    packet_data = struct.pack('!iiiB', x, y, z, block_type)
    return struct.pack('!B', packet_id) + packet_data


def parse_block_change_packet(message):
    try:
        x, y, z, block_type, placed_byte = struct.unpack('!iiiB?', message[1:])
        return x, y, z, block_type, bool(placed_byte)
    except struct.error as e:
        return None

def create_spawn_player_packet(player_id, username, x, y, z, yaw, pitch):
    packet_id = 0x20
    name_bytes = username.encode('utf-8')
    packet_data = struct.pack('!B', player_id) + \
                  struct.pack('!h', len(name_bytes)) + name_bytes + \
                  struct.pack('!fffff', x, y, z, yaw, pitch)
    return struct.pack('!B', packet_id) + packet_data

def create_set_player_position_packet(player_id, x, y, z, yaw, pitch):
    packet_id = 0x21
    packet_data = struct.pack('!bfffff', player_id, x, y, z, yaw, pitch)
    return struct.pack('!B', packet_id) + packet_data

def create_despawn_player_packet(player_id):
    packet_id = 0x22
    return struct.pack('!BB', packet_id, player_id)

def parse_position_packet(message):
    try:
        _player_id, x, y, z, yaw, pitch = struct.unpack('!bfffff', message[1:])
        return x, y, z, yaw, pitch
    except struct.error:
        return None
    
def create_set_spawn_position_packet(x, y, z, yaw):
    packet_id = 0x23
    packet_data = struct.pack('!ihii', x, y, z, yaw)
    return struct.pack('!B', packet_id) + packet_data

def parse_request_spawn_position_packet(message):
    if message[0] == 0x24:
        return True
    return False


# --- Content from world_manager.py ---


WORLD_DATA_COMPRESSED = None

lib_name = "core.so" if platform.system() != "Windows" else "core.dll"

try:
    core_lib = ctypes.CDLL(f'./{lib_name}')
except OSError as e:
    logging.error(f"FATAL: Could not load core library '{lib_name}'. Make sure it's in the same directory. Error: {e}")
    exit(1)

class WorldData(ctypes.Structure):
    _fields_ = [("blocks", ctypes.POINTER(ctypes.c_uint8)),
                ("size", ctypes.c_size_t)]
    
class SpawnPosition(ctypes.Structure):
    _fields_ = [("x", ctypes.c_int),
                ("y", ctypes.c_int),
                ("z", ctypes.c_int),
                ("rot", ctypes.c_int)]

try:
    core_lib.create_world_generator.restype = ctypes.c_void_p

    core_lib.generate_world.argtypes = [ctypes.c_void_p, ctypes.c_int, ctypes.c_int, ctypes.c_int]
    core_lib.generate_world.restype = WorldData

    core_lib.free_world_data.argtypes = [WorldData]

    core_lib.destroy_world_generator.argtypes = [ctypes.c_void_p]
    
    logging.info(f"✅ Successfully loaded and configured '{lib_name}'")

except AttributeError as e:
    logging.error(f"FATAL: A function is missing in the C++ core library. Did you implement all API functions? Error: {e}")
    exit(1)

def get_compressed_world_data():
    return WORLD_DATA_COMPRESSED

BLOCK_CHANGED_CALLBACK = ctypes.CFUNCTYPE(None, ctypes.c_int, ctypes.c_int, ctypes.c_int, ctypes.c_int)

core_lib.set_block_changed_callback.argtypes = [ctypes.c_void_p, BLOCK_CHANGED_CALLBACK]
core_lib.tick_world.argtypes = [ctypes.c_void_p]
core_lib.set_block.argtypes = [ctypes.c_void_p, ctypes.c_int, ctypes.c_int, ctypes.c_int, ctypes.c_int]
core_lib.save_world_to_file.argtypes = [ctypes.c_void_p, ctypes.c_char_p]
core_lib.save_world_to_file.restype = ctypes.c_bool
core_lib.load_world_from_file.argtypes = [ctypes.c_void_p, ctypes.c_char_p]
core_lib.load_world_from_file.restype = ctypes.c_bool
core_lib.get_world_data.argtypes = [ctypes.c_void_p]
core_lib.get_world_data.restype = WorldData
core_lib.get_spawn_position.argtypes = [ctypes.c_void_p]
core_lib.get_spawn_position.restype = SpawnPosition

WORLD_INSTANCE = None
BROADCAST_FUNCTION = None
MAIN_EVENT_LOOP = None

def init_world(width, height, depth):
    global WORLD_INSTANCE, WORLD_DATA_COMPRESSED
    if WORLD_INSTANCE is not None:
        return
    
    logging.info("Creating and generating world via C++ core...")
    WORLD_INSTANCE = core_lib.create_world_generator()
    
    if not WORLD_INSTANCE:
        logging.error("FATAL: Failed to create C++ world instance.")
        exit(1)

    if core_lib.load_world_from_file(WORLD_INSTANCE, b"level.dat"):
        logging.info("Successfully loaded world from level.dat.")
    else:
        logging.warning("level.dat not found or failed to load, generating a new world.")
        try:
            logging.info(f"Generating a {width}x{height}x{depth} world...")
            world_struct = core_lib.generate_world(WORLD_INSTANCE, width, height, depth)
            
            if not world_struct.blocks or world_struct.size == 0:
                raise ValueError("C++ core returned an empty world.")

            level_bytes = ctypes.string_at(world_struct.blocks, world_struct.size)
            core_lib.free_world_data(world_struct)
            
            logging.info(f"World generated ({len(level_bytes)} bytes). Compressing with gzip...")
            WORLD_DATA_COMPRESSED = gzip.compress(level_bytes)
            logging.info(f"World cached successfully. Compressed size: {len(WORLD_DATA_COMPRESSED)} bytes")
        except Exception as e:
            logging.error(f"An error occurred during world generation: {e}")
            core_lib.destroy_world_generator(WORLD_INSTANCE)
            WORLD_INSTANCE = None
            exit(1)
    
    def on_block_changed_from_cpp(x, y, z, block_type):
        if BROADCAST_FUNCTION and MAIN_EVENT_LOOP:
            
            update_packet = create_block_update_packet(x, y, z, block_type)
            
            MAIN_EVENT_LOOP.call_soon_threadsafe(
                asyncio.create_task, BROADCAST_FUNCTION(update_packet)
            )
    
    callback_ptr = BLOCK_CHANGED_CALLBACK(on_block_changed_from_cpp)
    core_lib.set_block_changed_callback(WORLD_INSTANCE, callback_ptr)
    
    setattr(core_lib, '_callback_ptr', callback_ptr)
    
def get_raw_world_data_from_cpp():
    if not WORLD_INSTANCE:
        logging.error("Attempted to get world data before world was initialized.")
        return None
        
    world_struct = core_lib.get_world_data(WORLD_INSTANCE)
    if not world_struct.blocks or world_struct.size == 0:
        logging.error("C++ core returned empty world data for packet.")
        return None
        
    try:
        level_bytes = ctypes.string_at(world_struct.blocks, world_struct.size)
        compressed_data = gzip.compress(level_bytes)
        return compressed_data
    finally:
        core_lib.free_world_data(world_struct)
    
def set_callbacks(broadcast_func, loop):
    global BROADCAST_FUNCTION, MAIN_EVENT_LOOP
    BROADCAST_FUNCTION = broadcast_func
    MAIN_EVENT_LOOP = loop
    logging.info("Callbacks and event loop have been set for the C++ core.")

def set_broadcast_function(func):
    global BROADCAST_FUNCTION
    BROADCAST_FUNCTION = func

def tick_world_loop():
    while True:
        if WORLD_INSTANCE:
            core_lib.tick_world(WORLD_INSTANCE)
        time.sleep(1/20.0)

def set_block(x, y, z, block_type):
    if WORLD_INSTANCE:
        core_lib.set_block(WORLD_INSTANCE, x, y, z, block_type)
        
def save_world_periodically():
    while True:
        time.sleep(300)
        if WORLD_INSTANCE:
            logging.info("Periodically saving world to level.dat...")
            if core_lib.save_world_to_file(WORLD_INSTANCE, b"level.dat"):
                logging.info("World saved successfully.")
            else:
                logging.error("Failed to save world.")
                
def shutdown_server():
    if WORLD_INSTANCE:
        logging.info("Shutting down. Saving final world state...")
        if core_lib.save_world_to_file(WORLD_INSTANCE, b"level.dat"):
            logging.info("Final world state saved.")
        else:
            logging.error("Failed to save final world state.")
            
def get_spawn_pos():
    if WORLD_INSTANCE:
        return core_lib.get_spawn_position(WORLD_INSTANCE)
    return None

# --- Content from network.py ---


CLIENTS = set()

async def safe_send(client_ws, message):
    try:
        await client_ws.send(message)
    except websockets.exceptions.ConnectionClosed:
        logging.debug(f"Failed to send message to already closed client {client_ws.remote_address}")
        pass 

async def broadcast(message, exclude_ws=None):
    if CLIENTS:
        tasks = []
        for client in CLIENTS:
            if client != exclude_ws:
                task = asyncio.create_task(safe_send(client, message))
                tasks.append(task)
        if tasks:
            await asyncio.gather(*tasks)
            

async def handler(websocket):
    ip_address = websocket.remote_address[0]
    if is_ip_banned(ip_address):
        logging.warning(f"Connection refused for banned IP: {ip_address}")
        await websocket.close(1000, "You are banned.")
        return

    if len(CLIENTS) >= get_int_property("max-players"):
        logging.warning("Connection refused: server is full.")
        await websocket.close(1013, "Server is full!")
        return

    logging.info(f"Client connected from {websocket.remote_address}. Total clients: {len(CLIENTS) + 1}")
    CLIENTS.add(websocket)
    
    player = None
    try:
        await websocket.send(create_server_identification_packet())
        logging.info(f"-> Sent Server ID to {websocket.remote_address}")
        
        login_packet = await websocket.recv()
        username = f"Player_{websocket.remote_address[1]}"
        logging.info(f"<- Received Login packet from {websocket.remote_address}")

        player = add_player(websocket, username)
        
        await websocket.send(create_login_response_packet())
        logging.info(f"-> Sent Login Response to {websocket.remote_address}")
        
        for other_player in get_all_players():
            if other_player.id != player.id:
                spawn_packet = create_spawn_player_packet(
                    other_player.id, other_player.username,
                    other_player.x, other_player.y, other_player.z,
                    other_player.yaw, other_player.pitch
                )
                await websocket.send(spawn_packet)

        spawn_packet_for_others = create_spawn_player_packet(
            player.id, player.username, 0, 0, 0, 0, 0
        )
        await broadcast(spawn_packet_for_others, exclude_ws=websocket)

        compressed_data = get_raw_world_data_from_cpp()
        await websocket.send(create_level_data_packet(256, 256, 64, compressed_data))
        logging.info(f"-> Sent Level Data to {websocket.remote_address}")
        
        async for message in websocket:
            packet_id = message[0]
            logging.debug(f"<- RECV packet 0x{packet_id:02x} from {websocket.remote_address}")
            
            if packet_id == 0x02: # Block Change
                parsed_data = parse_block_change_packet(message)
                if parsed_data:
                    x, y, z, block_type, placed = parsed_data
                    new_block_type = block_type if placed else 0
                    
                    set_block(x, y, z, new_block_type)
                else:
                    logging.warning("Failed to parse Block Change packet.")
                    
            elif packet_id == 0x21: # Player Position
                # logging.info(f"Handling PLAYER_POSITION for player {player.id}")
                pos_data = parse_position_packet(message)
                if pos_data and player:
                    x, y, z, yaw, pitch = pos_data
                    player.set_position(x, y, z, yaw, pitch)
                    
                    update_packet = create_set_player_position_packet(
                        player.id, x, y, z, yaw, pitch
                    )
                    await broadcast(update_packet, exclude_ws=websocket)
            elif packet_id == 0x24: # Request Spawn Position
                logging.info(f"<- Received Spawn Position Request from {player.username}")
                spawn_pos = get_spawn_pos()
                if spawn_pos:
                    response_packet = create_set_spawn_position_packet(
                        spawn_pos.x, spawn_pos.y, spawn_pos.z, spawn_pos.rot
                    )
                    await websocket.send(response_packet)
                    logging.info(f"-> Sent Spawn Position to {player.username}")

    except websockets.exceptions.ConnectionClosed as e:
        logging.info(f"Client {websocket.remote_address} disconnected (code: {e.code})")
    except Exception as e:
        logging.error(f"An error occurred with client {websocket.remote_address}: {e}", exc_info=True)
    finally:
        player = remove_player(websocket)
        if player:
            logging.info(f"Broadcasting despawn for player {player.id}")
            despawn_packet = create_despawn_player_packet(player.id)
            
            await broadcast(despawn_packet, exclude_ws=websocket)
        
        CLIENTS.remove(websocket)
        logging.info(f"Client {websocket.remote_address} removed. Total clients: {len(CLIENTS)}")

def start_server():
    async def main():
        loop = asyncio.get_running_loop()
        set_callbacks(broadcast, loop)
        
        async with websockets.serve(handler, get_property('server-ip'), get_property('port')):
            logging.info(f"✅ Server started on ws://{get_property('server-ip')}:{get_property('port')}")
            await asyncio.Future()

    try:
        asyncio.run(main())
    except KeyboardInterrupt:
        logging.info("Server is shutting down.")


# --- Content from main.py ---


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



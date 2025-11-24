import ctypes
import logging
import platform
import asyncio
import gzip
import time
import os

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
core_lib.set_spawn_position.argtypes = [ctypes.c_void_p, ctypes.c_int, ctypes.c_int, ctypes.c_int, ctypes.c_int]
core_lib.set_spawn_position.restype = None

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
            from packets import create_block_update_packet
            
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
    os.remove("./externalurl.txt");
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

def set_spawn_pos(x, y, z, rot):
    if WORLD_INSTANCE:
        core_lib.set_spawn_position(WORLD_INSTANCE, x, y, z, rot)
        core_lib.save_world_to_file(WORLD_INSTANCE, b"level.dat") 
        return True
    return False
import struct
from config import *

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

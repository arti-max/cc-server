import struct
from config import *

PROTOCOL_VERSION = 3


def create_server_identification_packet():
    packet_id = 0x11
    name_bytes = get_property('server-name').encode('utf-8')
    motd_bytes = get_property('motd').encode('utf-8')
    
    packet_data = struct.pack('!B', PROTOCOL_VERSION) + \
                  struct.pack('!h', len(name_bytes)) + name_bytes + \
                  struct.pack('!h', len(motd_bytes)) + motd_bytes
    
    return struct.pack('!B', packet_id) + packet_data

def create_login_response_packet(username, playerId):
    packet_id = 0x10
    name_bytes = username.encode('utf-8')
    
    packet_data = struct.pack('!bh', playerId, len(name_bytes)) + name_bytes
    
    return struct.pack('!B', packet_id) + packet_data

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

def create_set_player_position_packet(player_id, x, y, z, yaw, pitch, type=0):
    packet_id = 0x21
    packet_data = struct.pack('!bbfffff', player_id, type, x, y, z, yaw, pitch)
    return struct.pack('!B', packet_id) + packet_data

def create_despawn_player_packet(player_id):
    packet_id = 0x22
    return struct.pack('!BB', packet_id, player_id)

def parse_position_packet(message):
    try:
        _player_id, _type, x, y, z, yaw, pitch = struct.unpack('!bbfffff', message[1:])
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

def parse_login_packet(message):
    try:
        offset = 1
        protocol_v = struct.unpack('!b', message[offset:offset+1])[0]
        offset += 1
        
        name_len = struct.unpack('!h', message[offset:offset+2])[0]
        offset += 2
        
        username = message[offset:offset+name_len].decode('utf-8')
        offset += name_len
        
        session_len = struct.unpack('!h', message[offset:offset+2])[0]
        offset += 2
        
        session_id = message[offset:offset+session_len].decode('utf-8')
        
        return username, session_id, int(protocol_v)
    except (struct.error, IndexError, UnicodeDecodeError) as e:
        logging.error(f"Failed to parse login packet: {e}")
        return None, None
    
def create_chat_message_packet(message_text):
    packet_id = 0x30
    message_bytes = message_text.encode('utf-8')
    if len(message_bytes) > 256: 
        message_bytes = message_bytes[:256]
        
    packet_data = struct.pack('!h', len(message_bytes)) + message_bytes
    return struct.pack('!B', packet_id) + packet_data

def parse_chat_message_packet(message):
    try:
        offset = 1
        msg_len = struct.unpack('!h', message[offset:offset+2])[0]
        offset += 2
        
        if msg_len > 64: 
            msg_len = 64
            
        message_text = message[offset:offset+msg_len].decode('utf-8')
        return message_text
    except Exception as e:
        logging.error(f"Failed to parse chat message packet: {e}")
        return None
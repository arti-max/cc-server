import asyncio
import websockets
from websockets.http import Headers
from websockets.typing import Subprotocol
from websockets.server import ServerProtocol
import logging
from world_manager import get_compressed_world_data, set_block, set_callbacks, get_raw_world_data_from_cpp, get_spawn_pos
from config import get_int_property, get_bool_property, get_property
from ban_manager import is_user_banned, is_ip_banned
from player_manager import add_player, remove_player, get_player, get_all_players, is_username_taken, validate_session
from packets import *
import gzip

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
    
    connections_from_this_ip = 0
    for client_ws in CLIENTS:
        if client_ws.remote_address[0] == ip_address:
            connections_from_this_ip += 1
            
    max_conn = get_int_property("max-connections")
    if connections_from_this_ip >= max_conn:
        logging.warning(f"Connection refused for {ip_address}: connection limit ({max_conn}) exceeded.")
        await websocket.close(1008, f"Connection limit from your IP ({max_conn}) exceeded.")
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
        
        login_message = await websocket.recv()
        username, session_id = parse_login_packet(login_message)
        
        if not username or not session_id:
            logging.warning(f"Failed to parse login packet from {websocket.remote_address}. Disconnecting.")
            await websocket.close(1002, "Invalid login packet")
            return
            
        
        logging.info(f"<- Login attempt from '{username}' ({websocket.remote_address})")
        
        if is_username_taken(username):
            logging.warning(f"Connection refused for {username}: already logged in.")
            await websocket.close(1008, "Someone with this username is already on the server!")
            return
        
        is_valid, reason = validate_session(username, session_id)
        if not is_valid:
            logging.warning(f"Login rejected for '{username}': {reason}")
            await websocket.close(1008, reason)
            return
        
        if is_user_banned(username):
            logging.warning(f"Connection refused for banned: {username}")
            await websocket.close(1000, "You are banned.")
        
        player = add_player(websocket, username, session_id)
        
        join_message = f"&e{player.username} joined the game"
        asyncio.create_task(broadcast(create_chat_message_packet(join_message)))
        
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
                    asyncio.create_task(broadcast(update_packet, exclude_ws=websocket))
            elif packet_id == 0x24: # Request Spawn Position
                logging.info(f"<- Received Spawn Position Request from {player.username}")
                spawn_pos = get_spawn_pos()
                if spawn_pos:
                    response_packet = create_set_spawn_position_packet(
                        spawn_pos.x, spawn_pos.y, spawn_pos.z, spawn_pos.rot
                    )
                    await websocket.send(response_packet)
                    logging.info(f"-> Sent Spawn Position to {player.username}")
                    
            elif packet_id == 0x31:
                chat_text = parse_chat_message_packet(message)
                if chat_text and player:
                    
                    formatted_message = f"{player.username}: {chat_text}"
                    logging.info(f"[CHAT] {formatted_message}")
                    
                    response_packet = create_chat_message_packet(formatted_message)
                    asyncio.create_task(broadcast(response_packet))

    except websockets.exceptions.ConnectionClosed as e:
        logging.info(f"Client {websocket.remote_address} disconnected (code: {e.code})")
    except Exception as e:
        logging.error(f"An error occurred with client {websocket.remote_address}: {e}", exc_info=True)
    finally:
        player = remove_player(websocket)
        if player:
            left_message = f"&e{player.username} left the game"
            asyncio.create_task(broadcast(create_chat_message_packet(left_message)))
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

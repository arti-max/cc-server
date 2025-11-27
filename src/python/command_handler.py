import logging
import asyncio
from admin_manager import is_admin, add_admin, remove_admin
from player_manager import find_player_by_username
from world_manager import set_spawn_pos
from ban_manager import add_ban, add_ip_ban, unban_user
from packets import create_chat_message_packet, create_set_spawn_position_packet, create_set_player_position_packet
from network import safe_send

async def send_private_message(player, message):
    from network import safe_send
    if player and player.ws:
        packet = create_chat_message_packet(f"&e{message}")
        await safe_send(player.ws, packet)

async def handle_command(sender_player, command_str, broadcast_func):
    if not is_admin(sender_player.username):
        await send_private_message(sender_player, "You do not have permission to use commands.")
        return

    parts = command_str.strip().split()
    command = parts[0].lower()
    args = parts[1:]

    logging.info(f"Admin '{sender_player.username}' issued command: {command_str}")

    # --- /kick <username> [reason] ---
    if command == "/kick" and len(args) >= 1:
        target_username = args[0]
        reason = " ".join(args[1:]) if len(args) > 1 else "Kicked by an admin."
        target_player = find_player_by_username(target_username)

        if target_player:
            await target_player.ws.close(1000, reason)
            await broadcast_func(create_chat_message_packet(f"&e{target_player.username} was kicked. ({reason})"))
        else:
            await send_private_message(sender_player, f"Player '{target_username}' not found.")
    
    # --- /ban <username> ---
    elif command == "/ban" and len(args) == 1:
        target_username = args[0]
        if add_ban(target_username):
            await send_private_message(sender_player, f"Player '{target_username}' has been banned.")
            target_player = find_player_by_username(target_username)
            if target_player:
                await target_player.ws.close(1000, "You have been banned.")
            await broadcast_func(create_chat_message_packet(f"&e{target_username} was banned."))
        else:
            await send_private_message(sender_player, f"Player '{target_username}' is already banned.")
            
    # --- /banip <username> ---
    elif command == "/banip" and len(args) == 1:
        target_username = args[0]
        target_player = find_player_by_username(target_username)
        if target_player:
            ip_address = target_player.ws.remote_address[0]
            if add_ip_ban(ip_address):
                await send_private_message(sender_player, f"IP {ip_address} for player '{target_username}' has been banned.")
                await target_player.ws.close(1000, "You have been IP banned.")
                await broadcast_func(create_chat_message_packet(f"&e{target_username} was IP banned."))
            else:
                await send_private_message(sender_player, f"IP {ip_address} is already banned.")
        else:
            await send_private_message(sender_player, f"Player '{target_username}' not found to get IP from.")

    # --- /broadcast <message> or /say <message> ---
    elif (command == "/broadcast" or command == "/say") and len(args) >= 1:
        message_to_broadcast = " ".join(args)
        await broadcast_func(create_chat_message_packet(f"&e{message_to_broadcast}"))
        
    elif command == "/setspawn":
        x, y, z = int(sender_player.x), int(sender_player.y), int(sender_player.z)
        rot = int(sender_player.yaw)
        
        if set_spawn_pos(x, y, z, rot):
            spawn_packet = create_set_spawn_position_packet(x, y, z, rot)
            await broadcast_func(spawn_packet)
            
            await broadcast_func(create_chat_message_packet(f"&eSpawn point updated to ({x}, {y}, {z})."))
        else:
            await send_private_message(sender_player, "Failed to set spawn point.")
            
    elif command == "/unban" and len(args) == 1:
        target_username = args[0]
        if unban_user(target_username):
            await send_private_message(sender_player, f"Player '{target_username}' unbanned.")
            await broadcast_func(create_chat_message_packet(f"&e{target_username} was unbanned."))
        else:
            await send_private_message(sender_player, f"Player '{target_username}' is not banned.")

    # --- /op <username> ---
    elif command == "/op" and len(args) == 1:
        target_username = args[0]
        if add_admin(target_username):
            await send_private_message(sender_player, f"Player '{target_username}' is now an operator.")
        
            target_player = find_player_by_username(target_username)
            if target_player:
                await send_private_message(target_player, "You are now an operator!")
        else:
            await send_private_message(sender_player, f"Player '{target_username}' is already an operator.")

    # --- /deop <username> ---
    elif command == "/deop" and len(args) == 1:
        target_username = args[0]
        if target_username.lower() == sender_player.username.lower():
             await send_private_message(sender_player, "You cannot de-op yourself.")
        elif remove_admin(target_username):
            await send_private_message(sender_player, f"Player '{target_username}' is no longer an operator.")
            target_player = find_player_by_username(target_username)
            if target_player:
                await send_private_message(target_player, "You are no longer an operator.")
        else:
            await send_private_message(sender_player, f"Player '{target_username}' is not an operator.")
    # --- /tp <who> <where> or /tp <x> <y> <z>     
    elif command == "/tp" or command == "/teleport":
        if len(args) == 2:
            tp_name = args[0]
            target_name = args[1]
            target_player = find_player_by_username(target_name)
            tp_player = find_player_by_username(tp_name)
            
            if target_player:
                sender_player.set_position(target_player.x, target_player.y, target_player.z, target_player.yaw, target_player.pitch)
                
                
                teleport_packet = create_set_player_position_packet(
                    tp_player.id, 
                    target_player.x, target_player.y + 1.6, target_player.z,
                    tp_player.yaw, tp_player.pitch, 1
                )
                
                await safe_send(sender_player.ws, teleport_packet)
                
                await broadcast_func(teleport_packet, exclude_ws=sender_player.ws)
                
                await send_private_message(sender_player, f"{tp_player.username} teleported to {target_player.username}.")
            else:
                await send_private_message(sender_player, f"Player '{target_name}' not found.")
                
        elif len(args) == 3:
            try:
                x = float(args[0])
                y = float(args[1])
                z = float(args[2])
                
                sender_player.set_position(x, y, z, sender_player.yaw, sender_player.pitch)
                
                teleport_packet = create_set_player_position_packet(
                    sender_player.id, x, y, z, sender_player.yaw, sender_player.pitch, 1
                )
                
                await safe_send(sender_player.ws, teleport_packet)
                await broadcast_func(teleport_packet, exclude_ws=sender_player.ws)
                
                await send_private_message(sender_player, f"Teleported to {x}, {y}, {z}.")
            except ValueError:
                await send_private_message(sender_player, "Invalid coordinates. Usage: /tp <x> <y> <z>")
        else:
            await send_private_message(sender_player, "Usage: /tp <player> OR /tp <x> <y> <z>")
        
    else:
        await send_private_message(sender_player, "Unknown command or invalid arguments.")

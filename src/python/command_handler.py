import logging
import asyncio
from admin_manager import is_admin
from player_manager import find_player_by_username
from ban_manager import add_ban, add_ip_ban
from packets import create_chat_message_packet

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

    # --- /broadcast <message> ---
    elif (command == "/broadcast") and len(args) >= 1:
        message_to_broadcast = " ".join(args)
        await broadcast_func(create_chat_message_packet(f"&e{message_to_broadcast}"))
        
    else:
        await send_private_message(sender_player, "Unknown command or invalid arguments.")

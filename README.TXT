## CrossCraft Public Server Alpha ##

Version 1.2

+ Added teleport command

Version 1.1

+ Added Player List on TAB (Client-side)
+ Added /setspawn command for admins
+ Added /op, /deop, /unban commands for admins
+ Added server-side name verification (verify-names)
+ Added logged-in.txt to show current players
+ The server now generates externalurl.txt with the direct join link
+ Added basic anti-packet-spam protection
+ Improved configuration file handling


## Requirements ##

- Python 3.10 or newer
- 'websockets' and 'requests' python libraries

Before the first run, make sure you have the libraries installed:
pip install websockets requests


## How to start ##

To start the server, run the following command in your terminal or command prompt:
python3 server.py


## Server Configuration ##

To change any settings, edit the `server.properties` file.

- public: Set to 'true' to list your server on crosscraftweb.ddns.net.
- verify-names: Set to 'true' to require players to have a valid CrossCraft account. If 'false', anyone can connect, and they will be assigned a Player<NUMBER> name if they don't provide one.


## Admin Commands ##

To become an administrator, add your in-game username to the `admins.txt` file (one name per line).

Ingame, admins can use the following commands by typing them in chat:

/tp <player1> <player2>
   - Teleport player1 to player2.

/tp <x> <y> <z>
   - Teleport player to x, y, z.

/op <username>
   - Grants admin permissions to the player. The player must be online.

/deop <username>
   - Revokes admin permissions from the player.

/kick <username> [reason]
   - Kicks a player from the server. A reason is optional.

/ban <username>
   - Bans and kicks a player. The ban is saved to `banned.txt`.

/unban <username>
   - Unbans a player.

/banip <username>
   - Bans a player's IP address and kicks them. The ban is saved to `banned-ip.txt` and cannot be undone ingame.

/setspawn
   - Sets the current location as the default spawn point for new players.

/say <message>
   - Broadcasts a message to the entire server as [Server].


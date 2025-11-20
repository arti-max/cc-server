## CrossCraft Public Server ##

This is the first public release of the CrossCraft server software. Thank you for helping host the CrossCraft community!

To change any settings, edit server.properties.

To run a public server that will be visible in the main server list, set 'public=true' in the properties.
To run a private server, set 'public=false'. The server will still report statistics, but it will not be listed for others to join.

To connect to your own private server, other players will need to get the direct URL from you:
http://crosscraftweb.ddns.net/play.jsp?server=<YOUR IP>&port=<SERVER PORT>


## Requirements ##

- Python 3.10 or newer
- 'websockets' and 'requests' python libraries

Before the first run, make sure you have the libraries installed:
pip install websockets requests


## How to start ##

To start the server, run the following command in your terminal or command prompt:
python3 server.py


## Admin Commands ##

To become an administrator, add your in-game username to the 'admins.txt' file (one name per line).

Ingame, admins can use the following commands by typing them in chat:

/kick <username> [reason]
   - Kicks a player from the server. A reason is optional.

/ban <username>
   - Bans and kicks a player. The ban is saved to 'banned.txt'.

/banip <username>
   - Bans a player's IP address and kicks them. The ban is saved to 'banned-ip.txt' and cannot be undone ingame.

/say <message>
   - Broadcasts a message to the entire server as [Server].


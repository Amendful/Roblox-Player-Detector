# **Roblox Player Detector**

[![Join Discord](https://img.shields.io/badge/Discord-Join-5865F2?style=for-the-badge&logo=discord)](https://discord.gg/wQaJJQQrXR)

**Note**: To ensure Windows toast notifications appear when in Windows fullscreen, follow the setup tutorial available in the Discord server.

For non-programmers, check the Discord server above for an API scraper to compile your `playerlist.txt` file with a list of players from a certain role in the Roblox group of your choice, as well as the prebuilt for the application.

## Overview
A C++ tool for monitoring player lists in Roblox games. It connects to the running Roblox process to check for specific users and sends a Windows toast notification when one from your list joins.

The application only reads memory and doesn't tamper/edit it

### How It Works
The tool connects to Roblox using helper files (`rbx.hpp` and `offsets.hpp`). It reads the Roblox client memory to detect if a username from your `playerlist.txt` is in the game, triggering a Windows toast notification. It checks for users already present at startup and stops cleanly on close.

<br>

# Features
- **Constant checks** – Reads the client memory for new player joins as well as pre-existing players in the game
- **Customizable list** – Use `playerlist.txt` for your targets; it will read through the list of usernames for the external to detect
- **Windows Notifications** – A windows toast notification to alert the user when a moderator/target player joins the game

<br>

# Install & Setup

### Building From Source:
1. **Clone the Repo**
   ```sh
   git clone <your-repo-url>
   cd Roblox-Player-Detector
   ```

2. **Add Files**
   - Include `rbx.hpp` and `offsets.hpp` in the main folder (get them from open Roblox tools online).

3. **Build It**
   - In Visual Studio: Open `main.cpp`, set to Release (x64), and click Build.
   - Or from command prompt:
     ```sh
     cl /EHsc /std:c++11 main.cpp PowrProf.lib
     ```
   - Compile to get the executable in the build folder.

<br>

# Usage
1. **Set Up Your List**  
   Make a `playerlist.txt` file next to the exe:  
   ```
   Player1
   Player2
   Player3
   ```

2. **Using the executable**  
   - Open Roblox and join a game.  
   - Run the compiled executable.  
   - It connects, shows the players, and starts watching.

3. **Stopping the executable**  
Closing the terminal will stop the program from operating 

## License
MIT License—details in [LICENSE](LICENSE).

## Legal
This tool is for educational and research purposes only. 
We don't support or take responsibility for any misuse.

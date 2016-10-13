#rinCheat
A multifunction plugin for PSVITA.

## Disclaimer

THIS TOOL HAS BEEN RELEASED JUST FOR TESTING PURPOSES. THIS IS NOT FINISHED AND THERE STILL ARE LOTS OF BUGS TO BE SOLVED. REPORT ON WOLOLO'S THREAD WHATEVER BUG YOU FIND IF YOU WANT TO CONTRIBUTE TO ITS DEVELOPMENT.

## Warning for developers (Read carefully)

This sourcecode has been released without a license.<br>
Since i'm the owner of this repository, whenever someone will release something using part or totality of this code without asking me for authorization, i'll INSTANTLY close source this project.<br>
I also expect no one bitching me if this happens cause, in that case, i can also consider to just drop the project too.

## Features

- Realtime cheats with cheats database support.
- Realtime memory scanner with (partial) heap scanner and main thread stack scanner.
- Decrypted savedata exporter/importer with multiple saveslots.
- Improved screenshot feature (no compression) in any game and any situation.
- FTP Server during gaming phase.
- Possibility to change console clockage with CPU, GPU, BUS, GPU Crossbar support.
- Possibility to disable Auto Suspend feature while in game.
- Possibility to stream PSVITA screen to PC while gaming.

## Credits

- Slade for his constant work pushing new cheats to the cheat database.
- Everyone who contributed to the cheat database.
- gnmmarechal, Slade and Red7s for testing stuffs.

## Controls

While in game press START+SELECT to open rinCheat menu.
<br><br>
In rinCheat menu:
- Cross = Select option / Increase selected number in the memory scanner
- Triangle = Return to previous menu
- Up/Down = Change selected option
- Left/Right = Change selected number in the memory scanner
- Square = Decrease selected number in the memory scanner
- Start = Return to the game

When screenshot feature is enabled (Game Hacks), press L+R+START to take a screenshot.
<br><br>
When starting a game, you can also:<br>
- Hold R to skip net module loading
- Hold L to force MMC mode

## MMC or RAM mode

These are the differences between MMC and RAM mode:<br>
<br>
Screenshots taking = Faster in RAM mode<br>
Cheats List support = Limited in MMC mode (due to low available memory)

## How to install and use

Place rinCheat.suprx to ux0:/plugins, then open ux0:/plugins/game.txt file and add this line to the file:
ux0:/plugins/rinCheat.suprx 1
To open rinCheat menu just press SELECT+START during your game session.

## How to use (CPU Clockage & AutoSuspend)

Pretty straightforward, navigate to Game Hacks menu and then enable such features.

## How to use (Screenshot feature)

In Game Hacks menu, enable Screenshot feature then close rinCheat.
During your game sessions you'll be able to take screenshots by pressing L+R+START.
Screenshots will be saved in ux0:/data/rinCheat/screenshots.
NOTE: If you see on right-bottom side of your screen the text "MMC Mode", taking a screenshot will take a lot of time (more than a minute).

## How to use (Value searcher/injector)

Navigate to Game Cheats -> Search Value.
In this screen you'll be able to search an arbitrary value of 1,2,4,8 bytes on stack or heap.
First of all set the value you want to search and its size, then press one of the two Start Absolute Search options, you'll be prompted with how many matches has been found.
Now you can directly inject a new value by pressing Inject value or you can unpause the game and, later, search between the matches how many changed their values by using Start Relative Search feature.
You can also use the Save offsets function (RAM Mode only at the moment) to save matched offsets on ux0:/data/rinCheat/db/TITLEID_offsets.txt. These offsets can be extremely useful to actually write automated cheats to insert to rinCheat database.

## How to use (Cheats List)

rinCheat actually has no cheats database cause cheats must by found by testers. When you find a good offset with the value searcher you can save the offset to actually write a cheat that will appear in the Cheats List.
To do so, after you saved the offsets, you have to create a file in ux0:/data/rinCheat/db named as TITLEID.txt. Here you can put how many cheats you want that will appear in the Cheats List.
Syntax for the file is:

\#CHEAT NAME<br>
@offset @value @size

(Remember to put a new line at the end of the file, just a minor bug that will be solved in next releases)

Example:

\#999 Max Health<br>
@0xB1A4A231 @0x3E7 @4

## How to use (Decrypted savedata dumper/restorer)

Such features are available on Manage Savedatas menu. Dumper will save the savedata to ux0:/data/rinCheat/TITLEID/SLOTX where X is the currently selected Slot. 
Edit your savedata as you wish and then you can re-inject it back by using the related feature.

## How to use (FTP Server)

FTP Server is available on Net Module menu. Just enable it and connect to the IP showed in the menu.
<br>NOTE: If you can't see any folder when connected just manually input the folder to show (ex: ux0:/).

## How to use (Screen Stream)

Screen Stream feature is available on Net Module menu. You can select the video quality of the stream (Lower = Best Framerate, Higher = Best Video Quality) and then you can start the stream by enabling the related feature. Then you only have to start the PC application and insert the IP shown on PSVITA to connect.
<br>NOTE: To change stream quality when you are already streaming, you must first disable stream, then change video quality of the stream and then re-enable the stream.

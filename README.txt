Explore the futuristic slums of Blade Runner, and listen in on the chaos the city has to offer.

Disclaimer: compilation supported on linux only with premake4 and gmake installed

Compilation:
1. run "premake4 gmake" from current folder (where this README is)
2. run "make"
3. run "./Project Assets/scene.lua"

Manual:
The Project always takes in a lua file "base" scene that the buildings are built on top of. My base scene contains a very largely scaled cube representing the ground (xz-plane at y=0).
It also contains texture mapped "roads" that are built from a for loop. Any file following the lua format defined in A3 will technically work.

When the program begins running, the user controls the helicopter with the following:
	Mouse movement within the window of the program controls the spotlight.
	WASD allows you to forward, backward, left, and right with respect to your current orientation.
	E and Q allows you to descend and ascend with respect to your current orientation.
	If you hold the shift key, WASD allows you to look up, down, left, and right.
	CTRL key being held down triggers infrared mode.
	
The program will only output to standard output if there was something wrong with loading the texture or audio files. Check your system's audio drives if an error comes up.
This project is best experienced with headphones. The irrKlang library supports 3D sounds and makes exploring the city more interesting.

Voice Acting Credits: Max Saar, Helena Ip, and Zhi Kai Lu
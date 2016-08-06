# iAIM

iAIM is a strategic battle puzzler. Two players play against each other and try to destroy the enemy ship.

![](https://raw.githubusercontent.com/MasterQ32/iAIM/master/screenshots/mainmenu.png)

## Gameplay

### Strategy Building
After selecting a level, the game starts. The first player starts building its attack strategy. He can place affectors by dragging them with the mouse from the toolbox to the
battleground. After placing an affector, it can be rotated by dragging the rectangular knob. Moving the affector works by simple dragging the affector. To remove the affector, it must be dragged to the left or the right toolbox. 

When the player thinks the base is ready, he can click the launch button.

![](https://raw.githubusercontent.com/MasterQ32/iAIM/master/screenshots/building.png)

### Launching
When launching, the projectile will move periodically around the players space ship. To launch the projectile, the player must hit either a mouse button or the space key.

The projectile will be launched in the direction it is currently facing. After the launch it will fly straight until it is beeing affected by an affector.

![](https://raw.githubusercontent.com/MasterQ32/iAIM/master/screenshots/help.png)

When the projectile hits a barricade, the barricade will be damaged. After 3 hits a barricade will be destroyed and allows hitting the force field.

The force field shields 4 hits to the space ship. The player whose space ship is destroyed first will lose the battle. Be careful, you can hit yourself!

### Resupplement
When the battle simulation is done, the other player is on turn.

Every turn, each player gets a set of supplements so he can build other strategies after using up the affectors.

## Technology
The game is built with SDL2 and its sibling libraries sdl2-mixer and sdl2-image.
The code is quite undocumented and messy as the game is the result of a game jam.

### Build Instructions (Linux)
Just clone the repository, then call make. Make sure sdl2, sdl2_image and sdl2_mixer is installed.

	git clone https://github.com/MasterQ32/iAIM
	make

An executable file named `aim` will emerge. This is the game executable which needs the folders `tex`, `sounds` and `levels`.

### Build Instructions (Windows)
Windows requires a bit more work to get iAIM to build. Also, visual studio must be
installed.

#### Download libraries
First, we need to download the required libraries.

For SDL2, go to https://www.libsdl.org/download-2.0.php and download the *Development Libraries* for Visual C++. Unzip the files into a folder of your choice.

For SDL2_image, go to https://www.libsdl.org/projects/SDL_image/ and download the *Development Libraries* for Visual C++. Unzip the files into a folder of your choice.

For SDL2_mixer, go to https://www.libsdl.org/projects/SDL_mixer/ and download the *Development Libraries* for Visual C++. Unzip the files into a folder of your choice.

#### Adjust the build file
After downloading and unzipping the libraries, open the build.bat in the root folder of iAIM.
Modify the three SET commands at the top of the file to fit your paths. Also set the ARCH variable to either `x86` or `x64` depending on your system.

#### Running the build
After the build setup we need to get the compiler. Go to the windows main menu and search for the "Visual Studio" folder. In this folder is a file link named "VS2015 x64 Native Tools". Depending on your installation, this can be similar.

Open the command line tools, navigate to the iAIM folder and run the command `build`. After the successfull execution of the batch file all required DLL files and `iAIM.exe` should emerge in the iAIM directory.

## Backstory
This is a remake of a project i once saw on the GamesCom but now seems dead. Strategic 1:1 gravity battles.

## Credits

### Space Ships
By MillionthVector (http://millionthvector.blogspot.de)
*found via opengameart.org*

### Sounds
SFXR (http://www.drpetter.se/project_sfxr.html)

### Textures
CGTextures (http://www.textures.com/)
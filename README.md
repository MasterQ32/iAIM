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
Just clone the repository, then call make:

	git clone https://github.com/MasterQ32/iAIM
	make

An executable file named `aim` will emerge. This is the game executable which needs the folders `tex`, `sounds` and `levels`.

### Build Instructions (Windows)
To be done.

## Backstory
This is a remake of a project i once saw on the GamesCom but now seems dead. Strategic 1:1 gravity battles.

## Credits
This game uses resources from opengameart.org:

### Space Ships
By MillionthVector (http://millionthvector.blogspot.de)

### Sounds
SFXR (http://www.drpetter.se/project_sfxr.html)

### Textures
CGTextures (http://www.textures.com/)
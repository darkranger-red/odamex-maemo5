odamex-maemo5
=============

Porting Odamex to Maemo 5.

Odamex(http://odamex.net/) is a Doom source port. 

This Maemo 5 port is aim to using external input device(keyboard/mouse/josytick)
and Qauke-style gameplay. If you just want classic Doom gameplay, you can use PrBoom port instead.
(PrBoom is already on Maemo 5)

BUILD
=============
cd odamex-maemo5
mkdir build
cd build
cmake ../
make

PLAY
=============
1)copy client/odamex and odamex.wad to N900.
2)chmod +x odamex
3)sudo apt-get install timidity (for music playback)
4)./odamex -iwad MyDocs/WADS/doom.wad

SUPPORTED GAMES
=============
Doom
Doom II
Ultimate Doom
Final Doom

*If you didn't own those games, the shareware version is free download. Go google some!

# Axe Runner

A game experiment in Raylib and C. Thinking about up/down only controls and an always scrolling 2d level -- an idea that if playing on an apple watch you could use the dial to contol (like an old school pong controller) or the crank on a Playdate. Thinking D&D meets Gradius meets Gauntlet. 

You are a Dwarf, kicked to Hel by Thor, trying to escape the dungeon and its denizens... Avoid the spikes and don't go splat.

## Credits
- Art by me in Aseprite.
- Code with a bit of AI assist.
- Engine: Raylib
- HTML Loadup Font: Alagard € by Hewett Tsoi


### Build
```bash
gcc main.c -I/opt/homebrew/include -L/opt/homebrew/lib -lraylib -framework OpenGL -framework Cocoa -framework IOKit -framework CoreVideo -o axe_runner
```

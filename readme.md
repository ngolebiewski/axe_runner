# Axe Runner

A game experiment in Raylib and C. Thinking about up/down only controls and an always scrolling 2d level -- an idea that if playing on an apple watch you could use the dial to contol (like an old school pong controller) or the crank on a Playdate. Thinking D&D meets Gradius meets Gauntlet. 

You are a Dwarf, kicked to Hel by Thor, trying to escape the dungeon and its denizens... Avoid the spikes and don't go splat.

## Credits
- Art by me in Aseprite.
- Code with a bit of AI assist.
- Engine: Raylib: https://www.raylib.com/
- HTML Loadup Font: Alagard € by Hewett Tsoi https://www.dafont.com/alagard.font


### Build
```bash
gcc main.c -I/opt/homebrew/include -L/opt/homebrew/lib -lraylib -framework OpenGL -framework Cocoa -framework IOKit -framework CoreVideo -o axe_runner
```

### Notes

### Art references and inspiration
- Viking coins with stylized crosses: https://upload.wikimedia.org/wikipedia/commons/a/a0/Thurcaston_Viking_mixed_coin_hoard_(FindID_106146).jpg
- Viking art essay from the Met Museum: https://www.metmuseum.org/essays/the-vikings-780-1100
- Cool earring for world snake: https://www.metmuseum.org/art/collection/search/468363

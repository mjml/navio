navio
=====

This is a small X11/Xlib client to help manage large numbers of desktop windows.

If you have 2+ monitors and work with many programs and terminals at once, alt-tabbing just doesn't cut it. Adding virtual desktops is a big help, but it isn't the whole solution. If you've often got more than four windows up, just cycling between them is slow since their order is arbitrary.

The spatial locations of the windows is unrelated to their tab-switching order and so having to perform a "human linear search" for the right window is too clunky and slow.

This little program `navio` lets you use the Super_L key (windows key) and the WASD arrow keys to move up,left,down,right between your windows. Usage is simple: you just run it in the background, probably from session autostart or a disowned terminal. It will grab only the Super-WASD keys, whether or not NumLock is turned on. It will not work with caps lock turned on.



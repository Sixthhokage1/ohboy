# Introduction #

This is a port for the China Chip A320 [Dingoo](http://en.wikipedia.org/wiki/Dingoo), a MIPS based device. The A320 has a "native" OS µC/OS-II (uCOS II) and users can install Dingux, a Linux port. This port is known to work for the native OS and should work for [Dingux](http://code.google.com/p/dingoo-linux/) ([also see](http://www.dingux.com/)) too. Built using the excellent [Native Dingoo SDK](http://code.google.com/p/dingoo-sdk/) created by Flatmush and Harteex. It uses the [Dingoo SDL port](http://code.google.com/p/dingoo-sdk/wiki/DingooSDL).
# Controls #

Oh Boy Controls for Dingoo A320

---



| ![http://wiki.ohboy.googlecode.com/hg/images/up.png](http://wiki.ohboy.googlecode.com/hg/images/up.png) | Gameboy D-Pad Up |
|:--------------------------------------------------------------------------------------------------------|:-----------------|
| ![http://wiki.ohboy.googlecode.com/hg/images/left.png](http://wiki.ohboy.googlecode.com/hg/images/left.png) ![http://wiki.ohboy.googlecode.com/hg/images/right.png](http://wiki.ohboy.googlecode.com/hg/images/right.png) | Gameboy D-Pad Left/Right |
| ![http://wiki.ohboy.googlecode.com/hg/images/down.png](http://wiki.ohboy.googlecode.com/hg/images/down.png) | Gameboy D-Pad Down |
| ![http://wiki.ohboy.googlecode.com/hg/images/start.png](http://wiki.ohboy.googlecode.com/hg/images/start.png) | Since August 2011 this is now Gameboy start rather than Menu action / Gameboy Start |
| ![http://wiki.ohboy.googlecode.com/hg/images/select.png](http://wiki.ohboy.googlecode.com/hg/images/select.png) | Since August 2011 this is now Gameboy Select rather than Menu |
| ![http://wiki.ohboy.googlecode.com/hg/images/a.png](http://wiki.ohboy.googlecode.com/hg/images/a.png) | Gameboy A |
| ![http://wiki.ohboy.googlecode.com/hg/images/b.png](http://wiki.ohboy.googlecode.com/hg/images/b.png) | Gameboy B |
| ![http://wiki.ohboy.googlecode.com/hg/images/x.png](http://wiki.ohboy.googlecode.com/hg/images/x.png) | Quit (when in emu) |
| ![http://wiki.ohboy.googlecode.com/hg/images/y.png](http://wiki.ohboy.googlecode.com/hg/images/y.png) | Gameboy Select |
| ![http://wiki.ohboy.googlecode.com/hg/images/r.png](http://wiki.ohboy.googlecode.com/hg/images/r.png) | Quit (when in emu) |
| ![http://wiki.ohboy.googlecode.com/hg/images/l.png](http://wiki.ohboy.googlecode.com/hg/images/l.png) | Quit (when in emu) |
| Power | Since August 2011 this is now the Menu key rather than take screenshot |

Edit the .rc file to change (note menu can't be configured in .rc file).

Oh Boy will load ohboy.rc (in the same directory as the .app file) first and then gnuboy.rc, ohboy.rc can be updated by the menu system. So if a customized .rc file is wanted, edit gnuboy.rc rather than ohboy.rc.

See http://code.google.com/p/dingoo-sdk/wiki/DingooSDL for the SDL key names, Dingoo SDL maps Dingoo buttons to keyboard presses. http://code.google.com/p/ohboy/source/browse/main.c shows the default bind commands.

See http://code.google.com/p/gnuboy/source/browse/trunk/docs/CONFIG for documentation on config .rc files.

These are the binds built into OhBoy for Dingoo (NOTE the comments are for demonstration purposes and should not be used in a .rc file!)
```
bind space +select		 /* X button */
bind shift +select		 /* Y button - LSHIFT*/
bind tab quit		 /* Left shoulder */
bind backspace quit		 /* Right shoulder */
bind up +up		 /*  */
bind down +down		 /*  */
bind left +left		 /*  */
bind right +right		 /*  */
bind ctrl +a		 /* A button - LEFTCTRL */
bind alt +b		 /* B button - LEFTALT */
bind enter +start		 /* START button */
unbind esc		 /* SELECT button */
/* do not bind esc as menu grabs it... */
```
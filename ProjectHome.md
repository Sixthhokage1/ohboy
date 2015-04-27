A gameboy emulator using the [gnuboy](http://code.google.com/p/gnuboy/) emulation core.

![http://wiki.ohboy.googlecode.com/hg/screenshots/ohboy_menu_20110820.png](http://wiki.ohboy.googlecode.com/hg/screenshots/ohboy_menu_20110820.png)

More ScreenShots.

Oh Boy is a port of the [gnuboy](http://code.google.com/p/gnuboy/) Gameboy emulator with a basic menu system. See [history](http://code.google.com/p/ohboy/wiki/History).

It runs on:

  * Linux x86
  * Windows (32 bit), e.g. Windows XP.
  * [Dingoo Native A320](DingooOhBoy.md)
  * [OpenDingux](http://www.treewalker.org/opendingux/) for GCW0 and Dingoo A320 - Also [Old Dingux](http://code.google.com/p/dingoo-linux/) [Dingux blog ](http://www.dingux.com/)
  * [Caanoo](OtherPorts.md)

There is code for Wiz and GP32x but this has not been tested recently (if you own one of these devices and are willing to do some test builds please contact me to get this updated). See the OtherPorts page.

Oh Boy is an [SDL](http://www.libsdl.org/) port of [gnuboy](http://code.google.com/p/gnuboy/) and offers:
  * frame skipping
  * GUI menu for loading roms
  * GUI menu for some settings and saving them to gnuboy .rc files
  * FPS indicator (from SDL gnuboy port)

NOTE Oh Boy does not make use of the existing gnuboy SDL port so there are some potential differences. For instance the original Oh Boy would hang in the audio code on some desktops (this particular sound problem has been fixed, see [source](http://code.google.com/p/ohboy/source/checkout)).

The current Oh Boy version here is now based on the latest version of [gnuboy](http://code.google.com/p/gnuboy/) and so the gnuboy code is now required for building. See BuildRequirements for more information.

The menu system has the potential  for breaking out into a standalone basic C and SDL project for a (game) menu system.

Hopefully the menu system can be added to the official gnuboy code base.

For a related projects see the base [gnuboy](http://code.google.com/p/gnuboy/) and also [Lemonboy](http://www.gp32x.com/board/index.php?/topic/48526-lemonboy-supercolor-gameboy-emu/) (has some Super Game Boy support).
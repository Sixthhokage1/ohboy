

# Requirements #

  * C-compiler, gcc is known to work
    * NOTE for Dingoo Native, see bug http://code.google.com/p/dingoo-sdk/issues/detail?id=64 for SDK suggestion
  * Make (e.g. GNU make)
  * SDL
  * [gnuboy](http://code.google.com/p/gnuboy/)
  * Freetype2 **optional**
  * libpng **optional**
  * libz **optional**

# Getting the code #

Pull code down from source code control, Google has some excellent copy/paste instructions for this. See http://code.google.com/p/ohboy/source/checkout and http://code.google.com/p/gnuboy/source/checkout quick instructions below:
```
hg clone https://ohboy.googlecode.com/hg/ ohboy
cd ohboy
svn checkout https://gnuboy.googlecode.com/svn/trunk/ gnuboy
```

# Building #

Unlike gnuboy which uses the Autoconf (configure/make) system, Oh Boy uses regular make which makes platform porting fairly easy, even to platforms where autoconf is not an option. Determine the make file for your platform an issue:
```
make -B -f Makefile.PICK_ONE
```


# Future #

Also see ToDo list.

Merge the SDL port, i.e. remove the duplicated functions (pcm\_submit, vid\_init, etc.).

Merge functionality into gnuboy.

By replacing Freetype2 using [SFont](http://www.linux-games.com/sfont/) and either using BMP files (SLD base BMP support built in) or adding built in support for TGA or png via code from http://www.nothings.org/ (thanks to http://www.philhassey.com/blog/2010/01/05/seahorse-adventures-loading-tgas-and-more/ for pointing me in that direction). There is always SDL\_Image too.
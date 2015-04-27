## Completed ##

  * Uses latest gnuboy code
  * Dingoo port: Sound control, volume in menu (there is an OSD sound option, that requires  keys this is different)
  * Dingoo: bug fix rom loader issues
  * Completed file selector code to deal with "../" more intelligently
  * Converted chose dir code to be like load file code (i.e. remove chdir/getcwd code)
  * Save config saves ROM dir with correct escapes for paths (for Dingoo)
  * Dingoo only bug - load/save state was not working
  * Dingoo only bug - load/save batteryand savestate/loading only works if "saves" directory (config setting "savedir") already exists ( and specified in .rc file). mkdir now works on Dingoo port.
  * Screenshot support (Pause key)
  * FPS indicator added
  * Freetype2 now optional - and will use a built in bitmap font (no need for etc/`*`.ttf for end users), via the [SFont library](http://bitbucket.org/clach04/sfont)
  * libpng now optional - if not used, attempted to load menu backgroud etc/launch.bmp (but works without)
  * libz now optional / not needed (was a dependency of png on some platforms)
  * Dingoo port has under/over clocking option
  * .zip file support
  * Created  bitmap transparent (e.g. png) SFont for use with menu, now uses  SDL\_image (if missing ugly BMP font without transparency is used). Considered using a mini png to SDL surface loader or TGA files, joyrider/hartex suggested tga, see Joyrider's sms port for Canoo http://www.gp32x.com/board/index.php?/topic/56974-sms-sdl-for-caanoo-by-joyrider-with-picklelauncher-by-pickle/ (limited tga file types). Also see stb\_image.c from http://www.nothings.org/. However the SDL\_image+libpng overhead was still less that using TTF.
  * allow no scaling (i.e. native res)
  * added full screen support (Thanks for Alya for his custom 160x144 -> 320x320 scaler)
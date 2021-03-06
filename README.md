_Colditz Escape!_ is a rewritten game engine for the classic "Escape From Colditz" Amiga game.
In this game, you control a set of four prisoners of war trying to escape from the infamous Colditz Castle WWII prison.

The original game, created by _Mike Halsall_ and _John Law_ (with intro music by [Bjørn Lynne](http://www.lynnemusic.com/)),
was published in 1991 by Digital Magic Software.
This new version, which allows you to play the game on the PSP and Windows platforms, has been reverse engineered from the
original Amiga game engine and is released under a GPL v3 license.

For more information, see http://sites.google.com/site/colditzescape.

#####FEATURES THAT MIGHT BE OF INTEREST TO YOU WITHIN THIS SOURCE:#####

* Code for a simple directShow movie player for Windows (`/win32/wmp.[cpp|h]`)
* Code for an [XAudio2](https://msdn.microsoft.com/en-us/library/windows/desktop/ee415813.aspx) sound player, with double  
  buffered streaming capabilities, for Windows (`/win32/winXAudio2.[cpp|h]`)
* Multiplatform wrapper for XML configuration files (`/conf.[c|h]` and `/eschew/`)
* Code for onscreen message printout on PSP (`/psp/psp-printf.h`)
* [IFF](http://en.wikipedia.org/wiki/Interchange_File_Format) image loader (`/graphics.c` &rarr; `load_iff()`)
* RAW texture loader, with or without Alpha (`/graphics.c` &rarr; `load_raw_rgb()`)
* OpenGL 2D rescale (`/graphics.c` &rarr; `rescale_buffer()`)
* line/bitplane interleaved interleaved to RGBA texture conversion (`graphics.c` &rarr; `line_interleaved_to_wGRAB()` and `bitplane_to_wGRAB()`)
* Bytekiller 1.3 decompression algorithm (`/low-level.c` &rarr; `uncompress()`)
* PowerPacker decompression (`/low-level.c` &rarr; `ppDecrunch()`, courtesy of 'amigadepacker' by _Heikki Orsila_)
* Generic HQ2X GLSL OpenGL Zoom shader 
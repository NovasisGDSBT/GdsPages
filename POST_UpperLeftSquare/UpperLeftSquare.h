#ifndef GREENSQUARE_H_INCLUDED
#define GREENSQUARE_H_INCLUDED

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <linux/fb.h>
#include <linux/kd.h>
#include <linux/vt.h>
#include <signal.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#if defined(__i386__) || defined(__alpha__)
#include <sys/io.h>
#endif
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <termios.h>
#include <unistd.h>
#include <getopt.h>

#include <SDL/SDL.h>
#include <SDL/SDL_gfxPrimitives.h>

#define     WIDTH       1920
#define     HEIGHT      1080
#define     BPP         32



#define     INTERVALS   12
#define     SQUARE_DIM  150
#define     VISIBLE_HEIGHT  380
#define     GREEN "GREEN"
#define     RED   "RED"
#define     YELLOW   "YELLOW"

extern  SDL_Surface *screen;

extern  SDL_Surface *initSDL(int scr_w, int scr_h, int bpp);
extern  void fill_screen(SDL_Surface *surface,int width , int height,int r, int g, int b,int offsetx,int offsety);




#endif // GREENSQUARE_H_INCLUDED

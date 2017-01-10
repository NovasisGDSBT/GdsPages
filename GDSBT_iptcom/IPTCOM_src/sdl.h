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




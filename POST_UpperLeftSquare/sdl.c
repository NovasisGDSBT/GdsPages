#include "UpperLeftSquare.h"
#include <signal.h>

SDL_Rect 	screen_rect = {0,0,WIDTH,HEIGHT};

SDL_Surface *initSDL(int scr_w, int scr_h, int bpp)
{
    SDL_Surface *scr = NULL;
    if (SDL_Init(SDL_INIT_VIDEO ) < 0)
    {
        printf("%s: Unable to init SDL: %s\n", __FUNCTION__, SDL_GetError());
        exit(-1);
    }
    atexit(SDL_Quit);
    scr = SDL_SetVideoMode(scr_w, scr_h, bpp, SDL_SWSURFACE|SDL_HWACCEL);

    if(scr == NULL)
    {
        printf("%s: Unable to set %dx%d video: %s\n", __FUNCTION__, scr_w, scr_h, SDL_GetError());
        exit(-1);
    }

    SDL_EnableUNICODE(1);
    SDL_ShowCursor(0);
    atexit(SDL_Quit);
    return scr;
}

void fill_screen(SDL_Surface *surface,int width , int height,int r, int g, int b,int offsetx,int offsety)
{
unsigned int colorkey;
SDL_Rect    rect;

    rect.x = offsetx;
    rect.y = offsety;
    rect.w = width-offsetx;
    rect.h =  height-offsety;
    colorkey = SDL_MapRGBA( surface->format, r, g, b, 0xff ) ;
    SDL_FillRect(surface, &rect, colorkey);
    SDL_UpdateRect(surface,rect.x,rect.y,rect.w,rect.h);
}


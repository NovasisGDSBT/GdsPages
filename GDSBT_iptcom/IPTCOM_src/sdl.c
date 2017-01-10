#include "sdl.h"
#include <signal.h>


extern  int iptcom_connected;

/*
State machine definitions :
1)  yellow square on for minimum YELLOW_SQUARE_TIME parameter
2)  if TCMS replies before TCMS timeout then
    {
        set green square for GREEN_SQUARE_TIME
        if page ! exists then
        {
            set red square for RED_SQUARE_TIME
            set default page
        }
    }
    else
        set default page
3) start x & chromium
*/

SDL_Surface *screen;
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

void draw_red(void)
{
    fill_screen(screen,13, 13,255,0,0,0,0);
}

void draw_green(void)
{
    fill_screen(screen,13, 13,0,255,0,0,0);
}

void draw_yellow(void)
{
    fill_screen(screen,13, 13,255,255,0,0,0);
}

void draw_black(int timeout)
{
    fill_screen(screen,13, 13,0,0,0,0,0);
}

void do_sdl(void)
{
    screen = initSDL(WIDTH,HEIGHT,BPP);
    draw_yellow();
}



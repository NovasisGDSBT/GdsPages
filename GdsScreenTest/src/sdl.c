#include "../include/GdsScreenTest.h"


SDL_Rect 	screen_rect = {0,0,WIDTH,HEIGHT};
TTF_Font 	    *STRINGFont;

SDL_Surface *initSDL(int scr_w, int scr_h, int bpp)
{
    SDL_Surface *scr = NULL;
    if (SDL_Init(SDL_INIT_VIDEO ) < 0)
    {
        printf("%s: Unable to init SDL: %s\n", __FUNCTION__, SDL_GetError());
		exit(-1);
    }
    scr = SDL_SetVideoMode(scr_w, scr_h, bpp, SDL_SWSURFACE|SDL_HWACCEL);
    if(scr == NULL)
    {
        printf("%s: Unable to set %dx%d video: %s\n", __FUNCTION__, scr_w, scr_h, SDL_GetError());
		exit(-1);
    }
    SDL_EnableUNICODE(1);
    SDL_ShowCursor(0);
	atexit(SDL_Quit);
	if(TTF_Init()==-1)
	{
		printf("%s : TTF_Init: %s\n",__FUNCTION__, TTF_GetError());
		exit(-1);
	}
    STRINGFont = TTF_OpenFont("/tmp/www/Bauhaus.ttf",FONT_PITCH);
    if ( STRINGFont == NULL )
    {
        printf("Failed to load font /tmp/www/Bauhaus.ttf\n");
        exit(-1);
    }
	return scr;
}

void fill_screen(SDL_Surface *surface,int width , int height,int r, int g, int b,int offsetx,int offsety)
{
unsigned int colorkey;
SDL_Rect    rect;

    rect.x = offsetx;
    rect.y = 0;
    rect.w = width-offsetx;
    rect.h = 384;
    colorkey = SDL_MapRGBA( surface->format, r, g, b, 0xff ) ;
    SDL_FillRect(surface, &rect, colorkey);
    SDL_UpdateRect(surface,rect.x,rect.y,rect.w,rect.h);
}

SDL_Surface *load_image(char *fname)
{
SDL_Surface *ld;
    if ( fname == NULL )
    {
        printf("%s called with NULL filename\n",__FUNCTION__);
        exit(-1);
    }
    ld = ((SDL_Surface *)IMG_Load(fname));
    if ( ld == NULL)
    {
        printf("%s : File %s not found\n",__FUNCTION__,fname);
        exit(-1);
    }
    return ld;
}

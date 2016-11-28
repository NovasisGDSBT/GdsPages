#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <SDL/SDL.h>
#include <SDL/SDL_image.h>
#include <SDL/SDL_ttf.h>

SDL_Surface     *surface;
SDL_Rect 	    ScreenRect;

SDL_Surface     *FlasherDoneImage,*FlashingImage;
SDL_Rect 	    FlashDoneRect,FlashingImageRect;
char            Text[32];
TTF_Font 	    *STRINGFont;
SDL_Color       STRINGFontColor = { 0x3f,0xaa,0xcd } ,STRINGFontColorRed = { 0xff,0,0 } , STRINGFontBgColor = { 0xec,0xec,0xe1 } ;  //376093
unsigned int    BlackColorKey;

SDL_Surface *initSDL(void)
{
SDL_Surface     *scr;

    if (SDL_Init(SDL_INIT_VIDEO) < 0)
    {
        printf("%s: Unable to init SDL: %s\n", __FUNCTION__, SDL_GetError());
		exit(-1);
    }
    scr = SDL_SetVideoMode(1920, 1080, 32, SDL_SWSURFACE);
    if(scr == NULL)
    {
        printf("%s: Unable to set 1920x1080 video: %s\n", __FUNCTION__, SDL_GetError());
		exit(-1);
    }
    printf(" SDL_ShowCursor \n");
    //SDL_EnableUNICODE(1);
    SDL_ShowCursor(SDL_DISABLE);

    printf(" TTF_Init \n");
	if(TTF_Init()==-1)
	{
		printf("%s : TTF_Init: %s\n",__FUNCTION__, TTF_GetError());
		exit(-1);
	}
	atexit(SDL_Quit);

    printf(" STRINGFont \n");
    STRINGFont = TTF_OpenFont("/tmp/application_storage/Bauhaus.ttf",96);
    if ( STRINGFont == NULL )
    {
        printf("Failed to load font/tmp/application_storage/Bauhaus.ttf\n");
        exit(-1);
    }

    sprintf(Text," System Flashed successfully ");
    printf(" System Flashed successfully \n");
    FlasherDoneImage  = TTF_RenderText_Shaded(STRINGFont ,Text,STRINGFontColor,STRINGFontBgColor);
    if ( FlasherDoneImage == NULL )
    {
        printf("Failed to allocate ram for FlasherDoneImage\n");
        exit(-1);
    }
    FlashDoneRect.x=960 - (FlasherDoneImage->w/2);
    FlashDoneRect.y=190 - (FlasherDoneImage->h/2);
    FlashDoneRect.w=FlasherDoneImage->w;
    FlashDoneRect.h=FlasherDoneImage->h;

    sprintf(Text," Flashing Image ");
    printf(" Flashing Image \n");
    FlashingImage  = TTF_RenderText_Shaded(STRINGFont ,Text,STRINGFontColorRed,STRINGFontBgColor);
    if ( FlashingImage == NULL )
    {
        printf("Failed to allocate ram for FlashingImage\n");
        exit(-1);
    }
    FlashingImageRect.x=960 - (FlashingImage->w/2);
    FlashingImageRect.y=190 - (FlashingImage->h/2);
    FlashingImageRect.w=FlashingImage->w;
    FlashingImageRect.h=FlashingImage->h;

    ScreenRect.x = 0;
    ScreenRect.y = 0;
    ScreenRect.w = 1920;
    ScreenRect.h = 400;
    printf(" BlackColorKey \n");

    printf(" END \n");
    return scr;
}

void ClearScreen(void)
{
    SDL_FillRect(surface, &ScreenRect, BlackColorKey);
    SDL_UpdateRect(surface,ScreenRect.x,ScreenRect.y,ScreenRect.w,ScreenRect.h);
}

void blink_FlashingImage(void)
{
FILE    *fp;
    while(1)
    {
        SDL_BlitSurface(FlashingImage,NULL,surface,&FlashingImageRect);
        SDL_UpdateRect(surface,FlashingImageRect.x,FlashingImageRect.y,FlashingImageRect.w,FlashingImageRect.h);
        sleep(1);
        SDL_FillRect(surface, &FlashingImageRect, BlackColorKey);
        SDL_UpdateRect(surface,FlashingImageRect.x,FlashingImageRect.y,FlashingImageRect.w,FlashingImageRect.h);
        sleep(1);
        fp = fopen("/tmp/system_flashed","r");
        if ( fp != NULL)
        {
            fclose(fp);
            return;
        }
    }
}

int main()
{
    printf("Enter\n");
    surface = initSDL();
    BlackColorKey = SDL_MapRGBA( surface->format, 0, 0, 0, 0xff ) ;
    printf("ClearScreen\n");
    ClearScreen();
    printf("blink_FlashingImage\n");
    blink_FlashingImage();
    printf("final blit\n");
    SDL_BlitSurface(FlasherDoneImage,NULL,surface,&FlashDoneRect);
    SDL_UpdateRect(surface,FlashDoneRect.x,FlashDoneRect.y,FlashDoneRect.w,FlashDoneRect.h);
    sleep(3);
    return 0;
}

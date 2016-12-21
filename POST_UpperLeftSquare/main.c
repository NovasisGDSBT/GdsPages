#include "UpperLeftSquare.h"
SDL_Surface *screen;

int main (int argc, char** argv)
{
int timeout;


    timeout = atoi(argv[1]) * 1000;
    screen = initSDL(WIDTH,HEIGHT,BPP);

     if ( strcmp(argv[2],(GREEN)) == 0 )
     {
        fill_screen(screen,13, 13,0,255,0,0,0);
     }
     else if( strcmp(argv[2],(RED)) == 0 )
     {
       fill_screen(screen,13, 13,255,0,0,0,0);
     }
     else if( strcmp(argv[2],(YELLOW)) == 0 )
     {
       fill_screen(screen,13, 13,255,255,0,0,0);
     }
     else
     {
        fill_screen(screen,13, 13,0,0,0,0,0);
     }



    SDL_Delay( timeout );
    SDL_Quit();
    return 0;
}

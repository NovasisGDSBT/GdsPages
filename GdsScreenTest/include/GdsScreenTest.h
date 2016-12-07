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
#include <SDL/SDL.h>
#include <SDL/SDL_image.h>
#include <SDL/SDL_ttf.h>

#define FONT_PITCH              24

#define     WIDTH       1920
//#define     HEIGHT      358
#define     HEIGHT      1080
#define     BPP         32
#define     NAMED_FIFO  "GdsScreenTest_fifo"
#define     KILL        1
#define     STOP        2

#define     INTERVALS   12
#define     SQUARE_DIM  150
#define     VISIBLE_HEIGHT  380

typedef struct _CommandsStruct
{
	char		command[32];
	int         (* fun_ptr)(SDL_Surface *surface,char *command, char *parameter1, char *parameter2, char *parameter3,int ret_val);
	int         retval;
}  CommandsStruct;

extern  SDL_Surface *screen;
extern  int         fifo_fd;
extern  TTF_Font 	*STRINGFont;
extern  int system_started;

extern  SDL_Surface *initSDL(int scr_w, int scr_h, int bpp);
extern  void fill_screen(SDL_Surface *surface,int width , int height,int r, int g, int b,int offsetx,int offsety);

extern  int decodecmds(SDL_Surface *surface,char  *command , int len);
extern  int stop_cmd(SDL_Surface *surface,char *command, char *parameter1, char *parameter2, char *parameter3, int ret_val);
extern  int start_cmd(SDL_Surface *surface,char *command, char *parameter1, char *parameter2, char *parameter3,int ret_val);
extern  int fill_red(SDL_Surface *surface,char *command, char *parameter1, char *parameter2, char *parameter3,int ret_val);
extern  int fill_gray(SDL_Surface *surface,char *command, char *parameter1, char *parameter2, char *parameter3,int ret_val);
extern  int fill_green(SDL_Surface *surface,char *command, char *parameter1, char *parameter2, char *parameter3,int ret_val);
extern  int fill_blue(SDL_Surface *surface,char *command, char *parameter1, char *parameter2, char *parameter3,int ret_val);
extern  int fill_black(SDL_Surface *surface,char *command, char *parameter1, char *parameter2, char *parameter3,int ret_val);
extern  int fill_white(SDL_Surface *surface,char *command, char *parameter1, char *parameter2, char *parameter3,int ret_val);
extern  int fill_borders(SDL_Surface *surface,char *command, char *parameter1, char *parameter2, char *parameter3,int ret_val);
extern  int fill_grayscale12(SDL_Surface *surface,char *command, char *parameter1, char *parameter2, char *parameter3,int ret_val);
extern  int fill_grayscale11(SDL_Surface *surface,char *command, char *parameter1, char *parameter2, char *parameter3,int ret_val);
extern  int fill_squareleft(SDL_Surface *surface,char *command, char *parameter1, char *parameter2, char *parameter3,int ret_val);
extern  int fill_squarecenter(SDL_Surface *surface,char *command, char *parameter1, char *parameter2, char *parameter3,int ret_val);
extern  int fill_squareright(SDL_Surface *surface,char *command, char *parameter1, char *parameter2, char *parameter3,int ret_val);
extern  int fill_squarecenter_reverse(SDL_Surface *surface,char *command, char *parameter1, char *parameter2, char *parameter3,int ret_val);
extern  int loop_test(SDL_Surface *surface,char *command, char *parameter1, char *parameter2, char *parameter3,int ret_val);
extern  int btloop_test(SDL_Surface *surface,char *command, char *parameter1, char *parameter2, char *parameter3,int ret_val);
extern  int diag_page(SDL_Surface *surface,char *command, char *parameter1, char *parameter2, char *parameter3,int ret_val);
extern  int image_test(SDL_Surface *surface,char *command, char *parameter1, char *parameter2, char *parameter3,int ret_val);
extern  SDL_Surface *load_image(char *fname);

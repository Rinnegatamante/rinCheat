/*
	PSP VSH 24bpp text bliter
*/
#include <psp2/types.h>
#include <psp2/display.h>

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

#include "blit.h"

#define ALPHA_BLEND 1

extern unsigned char msx[];

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
static int pwidth, pheight, pixelformat;
unsigned int* vram32;
int bufferwidth;

static uint32_t fcolor = 0x00ffffff;
static uint32_t bcolor = 0xff000000;

#if ALPHA_BLEND
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
uint32_t adjust_alpha(uint32_t col)
{
	uint32_t alpha = col>>24;
	uint8_t mul;
	uint32_t c1,c2;

	if(alpha==0)    return col;
	if(alpha==0xff) return col;

	c1 = col & 0x00ff00ff;
	c2 = col & 0x0000ff00;
	mul = (uint8_t)(255-alpha);
	c1 = ((c1*mul)>>8)&0x00ff00ff;
	c2 = ((c2*mul)>>8)&0x0000ff00;
	return (alpha<<24)|c1|c2;
}
#endif

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
//int blit_setup(int sx,int sy,const char *msg,int fg_col,int bg_col)
int blit_setup(void)
{
	SceDisplayFrameBuf param;
	param.size = sizeof(SceDisplayFrameBuf);
	sceDisplayGetFrameBuf(&param, SCE_DISPLAY_SETBUF_IMMEDIATE);

	pwidth = param.width;
	pheight = param.height;
	vram32 = param.base;
	bufferwidth = param.pitch;
	pixelformat = param.pixelformat;

	if( (bufferwidth==0) || (pixelformat!=0)) return -1;

	fcolor = 0x00ffffff;
	bcolor = 0x00000000;

	return 0;
}

/////////////////////////////////////////////////////////////////////////////
// blit text
/////////////////////////////////////////////////////////////////////////////
void blit_set_color(int fg_col)
{
	fcolor = fg_col;
}

/////////////////////////////////////////////////////////////////////////////
// blit text
/////////////////////////////////////////////////////////////////////////////
int blit_string(int sx,int sy,const char *msg)
{
	int x,y,p;
	int offset;
	char code;
	unsigned char font;
	uint32_t fg_col,bg_col;

#if ALPHA_BLEND
	uint32_t col,c1,c2;
	uint32_t alpha;

	fg_col = adjust_alpha(fcolor);
	bg_col = adjust_alpha(bcolor);
#else
	fg_col = fcolor;
	bg_col = bcolor;
#endif

//Kprintf("MODE %d WIDTH %d\n",pixelformat,bufferwidth);
	if( (bufferwidth==0) || (pixelformat!=0)) return -1;

	for(x=0;msg[x] && x<(pwidth/16);x++)
	{
		code = msg[x] & 0x7f; // 7bit ANK
		for(y=0;y<8;y++)
		{
			offset = (sy+(y*2))*bufferwidth + sx+x*16;
			font = y>=7 ? 0x00 : msx[ code*8 + y ];
			for(p=0;p<8;p++)
			{
#if ALPHA_BLEND
				col = (font & 0x80) ? fg_col : bg_col;
				alpha = col>>24;
				if(alpha==0)
				{
					vram32[offset] = col;
					vram32[offset + 1] = col;
					vram32[offset + bufferwidth] = col;
					vram32[offset + bufferwidth + 1] = col;
				}
				else if(alpha!=0xff)
				{
					c2 = vram32[offset];
					c1 = c2 & 0x00ff00ff;
					c2 = c2 & 0x0000ff00;
					c1 = ((c1*alpha)>>8)&0x00ff00ff;
					c2 = ((c2*alpha)>>8)&0x0000ff00;
					uint32_t color = (col&0xffffff) + c1 + c2;
					vram32[offset] = color;
					vram32[offset + 1] = color;
					vram32[offset + bufferwidth] = color;
					vram32[offset + bufferwidth + 1] = color;
				}
#else
				uint32_t color = (font & 0x80) ? fg_col : bg_col;
				vram32[offset] = color;
				vram32[offset + 1] = color;
				vram32[offset + bufferwidth] = color;
				vram32[offset + bufferwidth + 1] = color;
#endif
				font <<= 1;
				offset+=2;
			}
		}
	}
	return x;
}

int blit_string_ctr(int sy,const char *msg)
{
	int sx = 960/2-strlen(msg)*(16/2);
	return blit_string(sx,sy,msg);
}

int blit_stringf(int sx, int sy, const char *msg, ...)
{
	va_list list;
	char string[512];

	va_start(list, msg);
	vsprintf(string, msg, list);
	va_end(list);

	return blit_string(sx, sy, string);
}

void blit_clearscreen(){
	memset(vram32, bcolor, (bufferwidth*pheight+pwidth)<<2);
}
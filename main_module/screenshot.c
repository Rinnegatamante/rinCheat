/*
 * This File is Part Of : 
 *      ___                       ___           ___           ___           ___           ___                 
 *     /  /\        ___          /__/\         /  /\         /__/\         /  /\         /  /\          ___   
 *    /  /::\      /  /\         \  \:\       /  /:/         \  \:\       /  /:/_       /  /::\        /  /\  
 *   /  /:/\:\    /  /:/          \  \:\     /  /:/           \__\:\     /  /:/ /\     /  /:/\:\      /  /:/  
 *  /  /:/~/:/   /__/::\      _____\__\:\   /  /:/  ___   ___ /  /::\   /  /:/ /:/_   /  /:/~/::\    /  /:/   
 * /__/:/ /:/___ \__\/\:\__  /__/::::::::\ /__/:/  /  /\ /__/\  /:/\:\ /__/:/ /:/ /\ /__/:/ /:/\:\  /  /::\   
 * \  \:\/:::::/    \  \:\/\ \  \:\~~\~~\/ \  \:\ /  /:/ \  \:\/:/__\/ \  \:\/:/ /:/ \  \:\/:/__\/ /__/:/\:\  
 *  \  \::/~~~~      \__\::/  \  \:\  ~~~   \  \:\  /:/   \  \::/       \  \::/ /:/   \  \::/      \__\/  \:\ 
 *   \  \:\          /__/:/    \  \:\        \  \:\/:/     \  \:\        \  \:\/:/     \  \:\           \  \:\
 *    \  \:\         \__\/      \  \:\        \  \::/       \  \:\        \  \::/       \  \:\           \__\/
 *     \__\/                     \__\/         \__\/         \__\/         \__\/         \__\/                
 *
 * Copyright (c) Rinnegatamante <rinnegatamante@gmail.com>
 *
 */
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <psp2/io/fcntl.h>
#include <psp2/io/stat.h>
#include <psp2/rtc.h>
#include <psp2/display.h>
#include <psp2/kernel/processmgr.h>
#include "memory.h"

void takeScreenshot(char* titleid){
	SceDisplayFrameBuf param;
	param.size = sizeof(SceDisplayFrameBuf);
	sceDisplayWaitVblankStart();
	sceDisplayGetFrameBuf(&param, SCE_DISPLAY_SETBUF_NEXTFRAME);
	SceDateTime time;
	sceRtcGetCurrentClockLocalTime(&time);
	char filename[256], temp[0x1000];
	sprintf(filename,"ux0:/data/rinCheat/screenshots/%s_%d%d_%d%d%d.bmp",titleid,time.month,time.day,time.hour,time.minute,time.second);
	int fd = sceIoOpen(filename, SCE_O_CREAT|SCE_O_WRONLY, 0777);
	uint8_t* bmp_content;
	if (ram_mode) bmp_content = (uint8_t*)malloc(((param.pitch*param.height)<<2)+0x36);
	else bmp_content = (uint8_t*)temp;
	if (bmp_content == NULL) bmp_content = temp; // Not enough heap, temporary using MMC mode
	memset(bmp_content, 0, 0x36);
	*(uint16_t*)&bmp_content[0x0] = 0x4D42;
	*(uint32_t*)&bmp_content[0x2] = ((param.pitch*param.height)<<2)+0x36;
	*(uint32_t*)&bmp_content[0xA] = 0x36;
	*(uint32_t*)&bmp_content[0xE] = 0x28;
	*(uint32_t*)&bmp_content[0x12] = param.pitch;
	*(uint32_t*)&bmp_content[0x16] = param.height;
	*(uint32_t*)&bmp_content[0x1A] = 0x00200001;
	*(uint32_t*)&bmp_content[0x22] = ((param.pitch*param.height)<<2);
	if (bmp_content == temp){
		sceIoWrite(fd, bmp_content, 0x36);
		int x, y, i;
		i = 0;
		uint32_t* buffer = (uint32_t*)bmp_content;
		uint32_t* framebuf = (uint32_t*)param.base;
		for (y = 0; y<param.height; y++){
			for (x = 0; x<param.pitch; x++){
				buffer[i] = framebuf[x+(param.height-y)*param.pitch];
				uint8_t* clr = (uint8_t*)&buffer[i];
				uint8_t g = clr[1];
				uint8_t r = clr[2];
				uint8_t a = clr[3];
				uint8_t b = clr[0];
				buffer[i] = (a<<24) | (b<<16) | (g<<8) | r;
				i++;
				if (i == 1024){
					i = 0;
					sceIoWrite(fd, bmp_content, 0x1000);
					sceKernelPowerTick(1); // Since MMC screenshot is pretty slow
				}
			}
		}
		if (i != 0) sceIoWrite(fd, bmp_content, i<<2);
	}else{
		int x, y;
		uint32_t* buffer = (uint32_t*)bmp_content;
		uint32_t* framebuf = (uint32_t*)param.base;
		for (y = 0; y<param.height; y++){
			for (x = 0; x<param.pitch; x++){
				buffer[x+y*param.pitch+0x36] = framebuf[x+(param.height-y)*param.pitch];
				uint8_t* clr = (uint8_t*)&buffer[x+y*param.pitch+0x36];
				uint8_t r = clr[1];
				clr[1] = clr[3];
				clr[3] = r;
			}
		}
		sceIoWrite(fd, bmp_content, ((param.pitch*param.height)<<2)+0x36);
		free(bmp_content);
	}
	sceIoClose(fd);
}
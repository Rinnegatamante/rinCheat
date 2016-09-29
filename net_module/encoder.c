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
#include <stdlib.h>
#include <psp2/io/fcntl.h>
#include <psp2/kernel/sysmem.h>
#include "encoder.h"

#define ALIGN(x, a)	(((x) + ((a) - 1)) & ~((a) - 1))
 
void encoderInit(int width, int height, int pitch, encoder* enc){
	enc->in_size = ALIGN((width*height)<<1, 256);
	enc->out_size = ALIGN(width*height, 256);
	uint32_t tempbuf_size = ALIGN(enc->in_size + enc->out_size,0x40000);
	enc->memblock = sceKernelAllocMemBlock("encoderBuffer", SCE_KERNEL_MEMBLOCK_TYPE_USER_CDRAM_RW, tempbuf_size, NULL);
	if (enc->memblock < 0){
		int fd = sceIoOpen("ux0:/data/rinCheat/enc_log.txt", SCE_O_CREAT | SCE_O_WRONLY | SCE_O_TRUNC, 0777);
		char errcode[64];
		sprintf(errcode, "sceKernelAllocMemBlock %lX", enc->memblock);
		sceIoWrite(fd, errcode, 64);
		sceIoClose(fd);
		return;
	}
	int ret = sceKernelGetMemBlockBase(enc->memblock, &enc->tempbuf_addr);
	if (ret < 0){
		int fd = sceIoOpen("ux0:/data/rinCheat/enc_log.txt", SCE_O_CREAT | SCE_O_WRONLY | SCE_O_TRUNC, 0777);
		char errcode[64];
		sprintf(errcode, "sceKernelGetMemblockBase %lX", ret);
		sceIoWrite(fd, errcode, 64);
		sceIoClose(fd);
		return;
	}
	enc->context = malloc(sceJpegEncoderGetContextSize());
	if (enc->context == NULL){
		int fd = sceIoOpen("ux0:/data/rinCheat/enc_log.txt", SCE_O_CREAT | SCE_O_WRONLY | SCE_O_TRUNC, 0777);
		char errcode[64];
		sprintf(errcode, "%s", "Not enough memory");
		sceIoWrite(fd, errcode, 64);
		sceIoClose(fd);
		return;
	}
	ret = sceJpegEncoderInit(enc->context, width, height, PIXELFORMAT_YCBCR422 | ABGR2YCBCR, enc->tempbuf_addr + enc->in_size, enc->out_size);
	if (ret < 0){
		int fd = sceIoOpen("ux0:/data/rinCheat/enc_log.txt", SCE_O_CREAT | SCE_O_WRONLY | SCE_O_TRUNC, 0777);
		char errcode[64];
		sprintf(errcode, "sceJpegEncoderInit: %lX", ret);
		sceIoWrite(fd, errcode, 64);
		sceIoClose(fd);
		return;
	}
}

void encoderTerm(encoder* enc){
	sceKernelFreeMemBlock(enc->memblock);
	sceJpegEncoderEnd(enc->context);
	free(enc->context);
}

void* encodeABGR(encoder* enc, void* buffer, int width, int height, int pitch, int* outSize){
	int ret = sceJpegEncoderCsc(enc->context, enc->tempbuf_addr, buffer, pitch, PIXELFORMAT_ABGR8888);
	if (ret < 0){
		int fd = sceIoOpen("ux0:/data/rinCheat/enc_log2.txt", SCE_O_CREAT | SCE_O_WRONLY | SCE_O_TRUNC, 0777);
		char errcode[64];
		sprintf(errcode, "sceJpegEncoderCsc: %lX", ret);
		sceIoWrite(fd, errcode, 64);
		sceIoClose(fd);
		return NULL;
	}
	ret = sceJpegEncoderSetValidRegion(enc->context, width, height);
	if (ret < 0){
		int fd = sceIoOpen("ux0:/data/rinCheat/enc_log2.txt", SCE_O_CREAT | SCE_O_WRONLY | SCE_O_TRUNC, 0777);
		char errcode[64];
		sprintf(errcode, "sceJpegEncoderSetValidRegion: %lX", ret);
		sceIoWrite(fd, errcode, 64);
		sceIoClose(fd);
		return NULL;
	}
	sceJpegEncoderSetCompressionRatio(enc->context, 255);
	if (ret < 0){
		int fd = sceIoOpen("ux0:/data/rinCheat/enc_log2.txt", SCE_O_CREAT | SCE_O_WRONLY | SCE_O_TRUNC, 0777);
		char errcode[64];
		sprintf(errcode, "sceJpegEncoderSetCompressionRatio: %lX", ret);
		sceIoWrite(fd, errcode, 64);
		sceIoClose(fd);
		return NULL;
	}
	sceJpegEncoderSetOutputAddr(enc->context, enc->tempbuf_addr + enc->in_size, enc->out_size);
	if (ret < 0){
		int fd = sceIoOpen("ux0:/data/rinCheat/enc_log2.txt", SCE_O_CREAT | SCE_O_WRONLY | SCE_O_TRUNC, 0777);
		char errcode[64];
		sprintf(errcode, "sceJpegEncoderSetOutputAddr: %lX", ret);
		sceIoWrite(fd, errcode, 64);
		sceIoClose(fd);
		return NULL;
	}
	*outSize = sceJpegEncoderEncode(enc->context, enc->tempbuf_addr);
	if (*outSize < 0){
		int fd = sceIoOpen("ux0:/data/rinCheat/enc_log2.txt", SCE_O_CREAT | SCE_O_WRONLY | SCE_O_TRUNC, 0777);
		char errcode[64];
		sprintf(errcode, "sceJpegEncoderEncode: %lX", *outSize);
		sceIoWrite(fd, errcode, 64);
		sceIoClose(fd);
		return NULL;
	}
	return enc->tempbuf_addr + enc->in_size;
}

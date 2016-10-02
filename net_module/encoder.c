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
 
void encoderInit(int width, int height, int pitch, encoder* enc, uint8_t video_quality){
	enc->in_size = ALIGN((width*height)<<1, 256);
	enc->out_size = ALIGN(width*height, 256);
	uint32_t tempbuf_size = ALIGN(enc->in_size + enc->out_size,0x40000);
	enc->memblock = sceKernelAllocMemBlock("encoderBuffer", SCE_KERNEL_MEMBLOCK_TYPE_USER_CDRAM_RW, tempbuf_size, NULL);
	sceKernelGetMemBlockBase(enc->memblock, &enc->tempbuf_addr);
	enc->context = malloc(sceJpegEncoderGetContextSize());
	sceJpegEncoderInit(enc->context, width, height, PIXELFORMAT_YCBCR420 | PIXELFORMAT_CSC_ARGB_YCBCR, enc->tempbuf_addr + enc->in_size, enc->out_size);
	sceJpegEncoderSetValidRegion(enc->context, width, height);
	sceJpegEncoderSetCompressionRatio(enc->context, video_quality);
	sceJpegEncoderSetOutputAddr(enc->context, enc->tempbuf_addr + enc->in_size, enc->out_size);
}

void encoderTerm(encoder* enc){
	sceKernelFreeMemBlock(enc->memblock);
	sceJpegEncoderEnd(enc->context);
	free(enc->context);
}

void* encodeARGB(encoder* enc, void* buffer, int width, int height, int pitch, int* outSize){
	sceJpegEncoderCsc(enc->context, enc->tempbuf_addr, buffer, pitch, PIXELFORMAT_ARGB8888);
	*outSize = sceJpegEncoderEncode(enc->context, enc->tempbuf_addr);
	return enc->tempbuf_addr + enc->in_size;
}

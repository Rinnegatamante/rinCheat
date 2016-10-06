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
#include <jpeglib.h>
#include <setjmp.h>

#define ALIGN(x, a)	(((x) + ((a) - 1)) & ~((a) - 1))

// Incompatibility between sceLibc and newlib fix
int _free_vita_newlib() {
	return 0;
}
int _fini(){
	return 0;
}

// libjpeg stuffs
struct jpeg_error_mgr jerr;
struct jpeg_compress_struct cinfo;

void encoderInit(int width, int height, int pitch, encoder* enc, uint8_t video_quality){
	enc->in_size = ALIGN((width*height)<<1, 256);
	enc->out_size = ALIGN(width*height, 256);
	uint32_t tempbuf_size = ALIGN(enc->in_size + enc->out_size,0x40000);
	enc->memblock = sceKernelAllocMemBlock("encoderBuffer", SCE_KERNEL_MEMBLOCK_TYPE_USER_CDRAM_RW, tempbuf_size, NULL);
	if (enc->memblock < 0){ // Not enough vram, will use libjpeg
		enc->isHwAccelerated = 0;
		enc->memblock = sceKernelAllocMemBlock("encoderBuffer", SCE_KERNEL_MEMBLOCK_TYPE_USER_RW, tempbuf_size, NULL);
		sceKernelGetMemBlockBase(enc->memblock, &enc->tempbuf_addr);
		enc->out_size = tempbuf_size;
		cinfo.err = jpeg_std_error(&jerr);
		jpeg_create_compress(&cinfo);
		cinfo.image_width = pitch;
		cinfo.image_height = height;
		cinfo.input_components = 4;
		cinfo.in_color_space = JCS_EXT_RGBA;
		jpeg_set_defaults(&cinfo);
		cinfo.dct_method = JDCT_FASTEST;
		jpeg_set_colorspace(&cinfo, JCS_YCbCr);
		jpeg_set_quality(&cinfo, 100 - ((100*video_quality) / 255), TRUE);
	}else{ // Will use sceJpegEnc
		enc->isHwAccelerated = 1;
		sceKernelGetMemBlockBase(enc->memblock, &enc->tempbuf_addr);
		enc->context = malloc(sceJpegEncoderGetContextSize());
		sceJpegEncoderInit(enc->context, width, height, PIXELFORMAT_YCBCR420 | PIXELFORMAT_CSC_ARGB_YCBCR, enc->tempbuf_addr + enc->in_size, enc->out_size);
		sceJpegEncoderSetValidRegion(enc->context, width, height);
		sceJpegEncoderSetCompressionRatio(enc->context, video_quality);
		sceJpegEncoderSetOutputAddr(enc->context, enc->tempbuf_addr + enc->in_size, enc->out_size);
	}
}

void encoderTerm(encoder* enc){
	if (enc->isHwAccelerated){
		sceJpegEncoderEnd(enc->context);
		free(enc->context);
	}else jpeg_destroy_compress(&cinfo);
	sceKernelFreeMemBlock(enc->memblock);
}

void* encodeARGB(encoder* enc, void* buffer, int width, int height, int pitch, int* outSize){
	if (enc->isHwAccelerated){
		sceJpegEncoderCsc(enc->context, enc->tempbuf_addr, buffer, pitch, PIXELFORMAT_ARGB8888);
		*outSize = sceJpegEncoderEncode(enc->context, enc->tempbuf_addr);
		return enc->tempbuf_addr + enc->in_size;
	}else{
		JSAMPROW row_pointer[1];
		unsigned char* outBuffer = (unsigned char*)enc->tempbuf_addr;
		long unsigned int out_size = enc->out_size;
		jpeg_mem_dest(&cinfo, &outBuffer, &out_size);		
		JSAMPLE* buf = (JSAMPLE*)buffer;
		jpeg_start_compress(&cinfo, TRUE);
		int row_stride = cinfo.image_width * cinfo.input_components;
		while (cinfo.next_scanline < cinfo.image_height){
			row_pointer[0] = &buf[cinfo.next_scanline * row_stride];
			jpeg_write_scanlines(&cinfo, row_pointer, 1);
		}
		jpeg_finish_compress(&cinfo);
		*outSize = out_size;		
		return enc->tempbuf_addr;
	}
}
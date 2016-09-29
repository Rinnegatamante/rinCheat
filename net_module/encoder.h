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
 
#ifndef _ENCODER_H_
#define _ENCODER_H_

#define SceJpegEncoderContext void*

typedef struct encoder{
	SceUID memblock;
	void* tempbuf_addr;
	uint32_t in_size;
	uint32_t out_size;
	SceJpegEncoderContext context;
}encoder;

enum {
	PIXELFORMAT_ABGR8888 = 0,
	PIXELFORMAT_YCBCR422 = 9
};

#define ABGR2YCBCR 16

SceUID sceJpegEncoderInit(SceJpegEncoderContext context,SceUID iFrameWidth,SceUID iFrameHeight,SceUID pixelFormat,void* pJpeg, SceUID oJpegbufSize);
SceUID sceJpegEncoderEncode(SceJpegEncoderContext context, void* pYCbCr);
SceUID sceJpegEncoderSetCompressionRatio(SceJpegEncoderContext context,SceUID compratio);
SceUID sceJpegEncoderSetOutputAddr(SceJpegEncoderContext context,void* pJpeg,SceUID oJpegbufSize);
SceUID sceJpegEncoderCsc(SceJpegEncoderContext context,void* pYCbCr,void* pRGBA,SceUID iFrameWidth,SceUID inputPixelFormat);
SceUID sceJpegEncoderGetContextSize();
SceUID sceJpegEncoderSetValidRegion(SceJpegEncoderContext context, SceUID iFrameWidth, SceUID iFrameHeight);
SceUID sceJpegEncoderEnd(SceJpegEncoderContext context);

void encoderInit(int width, int height, int pitch, encoder* enc);
void encoderTerm(encoder* enc);
void* encodeABGR(encoder* enc, void* buffer, int width, int height, int pitch, int* outSize);

#endif
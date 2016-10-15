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
#include <psp2/display.h>
#include <psp2/sysmodule.h>
#include <psp2/kernel/threadmgr.h>
#include <psp2/net/net.h>
#include <psp2/net/netctl.h>
#include <psp2/ctrl.h>
#include <psp2/io/fcntl.h>
#include "encoder.h"
#include "ftpvita.h"

#define STREAM_PORT 5000 // Port used for screen streaming
#define STREAM_BUFSIZE 0x20000 // Size of stream buffer

// Requests type
enum{
	NONE = 0,
	FTP_SWITCH,
	SCREEN_STREAM,
	SET_LOWEST_QUALITY = 251,
	SET_LOW_QUALITY = 252,
	SET_NORMAL_QUALITY = 253,
	SET_HIGH_QUALITY = 254,
	SET_BEST_QUALITY = 255
};

volatile uint8_t request = NONE;
volatile uint8_t stream_state = 0;
uint8_t video_quality = 255;
unsigned long vita_addr;

int stream_thread(SceSize args, void* argp){

	// Internal stuffs
	int stream_skt = -1;
	int sndbuf_size = STREAM_BUFSIZE;
	encoder jpeg_encoder;
	char unused[256];
	SceNetSockaddrIn addrTo, addrFrom;
	unsigned int fromLen = sizeof(addrFrom);
	
	// Initializing JPG encoder
	SceDisplayFrameBuf param;
	param.size = sizeof(SceDisplayFrameBuf);
	sceDisplayGetFrameBuf(&param, SCE_DISPLAY_SETBUF_NEXTFRAME);
	encoderInit(param.width, param.height, param.pitch, &jpeg_encoder, video_quality);
	
	// Streaming loop
	for (;;){
		if (stream_state){
		
			// Starting a new broadcast server socket
			if (stream_skt < 0){
				stream_skt = sceNetSocket("Stream Socket", SCE_NET_AF_INET, SCE_NET_SOCK_DGRAM, SCE_NET_IPPROTO_UDP);
				addrTo.sin_family = SCE_NET_AF_INET;
				addrTo.sin_port = sceNetHtons(STREAM_PORT);
				addrTo.sin_addr.s_addr = vita_addr;
				sceNetBind(stream_skt, (SceNetSockaddr*)&addrTo, sizeof(addrTo));			
				sceNetSetsockopt(stream_skt, SCE_NET_SOL_SOCKET, SCE_NET_SO_SNDBUF, &sndbuf_size, sizeof(sndbuf_size));
				
				// Waiting until a client is connected
				sceNetRecvfrom(stream_skt, unused, 8, 0, (SceNetSockaddr*)&addrFrom, &fromLen);
				
				// Sending resolution settings
				char txt[32];
				sprintf(txt, "%d;%d;%hhu", param.pitch, param.height, jpeg_encoder.isHwAccelerated);
				sceNetSendto(stream_skt, txt, 32, 0, (SceNetSockaddr*)&addrFrom, sizeof(addrFrom));
				
			}
			
			// Broadcasting framebuffer frames
			if (stream_skt >= 0){
				int bytes = 0;
				int mem_size = 0;
				SceDisplayFrameBuf param;
				param.size = sizeof(SceDisplayFrameBuf);
				sceDisplayGetFrameBuf(&param, SCE_DISPLAY_SETBUF_NEXTFRAME);
				uint8_t* mem = encodeARGB(&jpeg_encoder, param.base, param.width, param.height, param.pitch, &mem_size);
				sceNetSendto(stream_skt, mem, mem_size, 0, (SceNetSockaddr*)&addrFrom, sizeof(addrFrom));
			}
			
		}else{
			if (stream_skt > 0){
				sceNetSocketClose(stream_skt);
				stream_skt = -1;
				encoderTerm(&jpeg_encoder);
				sceKernelExitDeleteThread(0);
			}
		}
	}
	return 0;
}

static uint8_t quality_list[] = {255, 200, 128, 64, 0};
int main_thread(SceSize args, void *argp){

	// Internal states
	uint8_t ftp_state = 0;
	uint16_t vita_port;
	
	// Loading net module
	sceSysmoduleLoadModule(SCE_SYSMODULE_NET);
	
	// Initializing SceNet and SceNetCtl
	char vita_ip[32];
	int ret = sceNetShowNetstat();
	if (ret == SCE_NET_ERROR_ENOTINIT) {
		SceNetInitParam initparam;
		initparam.memory = malloc(0x100000);
		if (initparam.memory == NULL) sceKernelExitDeleteThread(0);
		initparam.size = 0x100000;
		initparam.flags = 0;
		sceNetInit(&initparam);
	}
	
	sceNetCtlInit();
	SceNetCtlInfo info;
	sceNetCtlInetGetInfo(SCE_NETCTL_INFO_GET_IP_ADDRESS, &info);
	sprintf(vita_ip,"%s",info.ip_address);
	sceNetInetPton(SCE_NET_AF_INET, info.ip_address, &vita_addr);
	
	// Writing on a temp file request address in order to let main module to know about it
	uint32_t addr = (uint32_t)&request;
	int tmp = sceIoOpen("ux0:/data/rinCheat/addr.bin", SCE_O_WRONLY|SCE_O_CREAT, 0777);
	sceIoWrite(tmp, &addr, 4);
	sceIoClose(tmp);
	tmp = sceIoOpen("ux0:/data/rinCheat/ip.txt", SCE_O_WRONLY|SCE_O_CREAT|SCE_O_TRUNC, 0777);
	sceIoWrite(tmp, vita_ip, strlen(vita_ip));
	sceIoClose(tmp);
	
	for (;;){
		switch (request){
			case FTP_SWITCH:
				request = NONE; // Resetting request field
				ftp_state = !ftp_state;
				if (ftp_state){
					ftpvita_add_device("app0:");
					ftpvita_add_device("ux0:");
					ftpvita_add_device("savedata0:");
					ftpvita_init(vita_ip, &vita_port);
				}else ftpvita_fini();
				break;
			case SCREEN_STREAM:
				request = NONE; // Resetting request field
				stream_state = !stream_state;
				if (stream_state){
					SceUID stream_thid = sceKernelCreateThread("rinCheat Stream", stream_thread,0x10000100, 0x10000, 0, 0, NULL);
					sceKernelStartThread(stream_thid, 0, NULL);
				}
				break;
			default:
				if (request >= SET_LOWEST_QUALITY){
					request -= SET_LOWEST_QUALITY;
					video_quality = quality_list[request];
					request = NONE; // Resetting request field
				}
				break;
		}
		sceKernelDelayThread(1000);
	}
	
	return 0;
}

int _start(SceSize args, void *argp) {
	SceUID thid = sceKernelCreateThread("rinCheat_net", main_thread, 0x40, 0x400000, 0, 0, NULL);
	if (thid >= 0)
		sceKernelStartThread(thid, 0, NULL);

	return 0;
}
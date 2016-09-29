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
#include <jpeglib.h>
#include <setjmp.h>
#include "ftpvita.h"

#define STREAM_PORT 5000 // Port used for screen streaming

// Requests type
#define NONE 0
#define FTP_SWITCH 1
#define SCREEN_STREAM 2

volatile uint8_t request = NONE;
volatile uint8_t stream_state = 0;

int _free_vita_newlib() {
	return 0;
}

int _fini(){
	return 0;
}

/*	
 * REVIEW:
 * 1) Tons of games don't have enough free memory to let libjpeg encoder to work (in that cases, an error is sent to the client and can be read with Wireshark)
 * 2) Code uses TCP which is slow, should be ported to UDP
 * 3) Since libjpeg encoder seems to be unreliable on most games, maybe switching to another compression could be the right choice
 */
int stream_thread(SceSize args, void* argp){
	int stream_skt = 0xDEADBEEF;
	int client_skt = -1;
	for (;;){
		if (stream_state){
			struct jpeg_error_mgr jerr;
			struct jpeg_compress_struct cinfo;
			JSAMPROW row_pointer[1];
			if (stream_skt == 0xDEADBEEF){
				stream_skt = sceNetSocket("Stream Socket", SCE_NET_AF_INET, SCE_NET_SOCK_STREAM, 0);
				SceNetSockaddrIn addrTo;
				addrTo.sin_family = SCE_NET_AF_INET;
				addrTo.sin_port = sceNetHtons(STREAM_PORT);
				addrTo.sin_addr.s_addr = sceNetHtonl(SCE_NET_INADDR_ANY);
				sceNetBind(stream_skt, (SceNetSockaddr*)&addrTo, sizeof(addrTo));
				sceNetListen(stream_skt, 128);
				cinfo.err = jpeg_std_error(&jerr);
				jpeg_create_compress(&cinfo);
			}
			if (client_skt < 0){
				SceNetSockaddrIn addrAccept;
				unsigned int cbAddrAccept = sizeof(addrAccept);
				client_skt = sceNetAccept(stream_skt, (SceNetSockaddr*)&addrAccept, &cbAddrAccept);
				if (client_skt >= 0){
					SceDisplayFrameBuf param;
					param.size = sizeof(SceDisplayFrameBuf);
					sceDisplayGetFrameBuf(&param, SCE_DISPLAY_SETBUF_NEXTFRAME);
					char txt[32];
					sprintf(txt, "%d;%d", param.pitch, param.height);
					sceNetSend(client_skt, txt, 32, 0);
				}
			}
			if (client_skt >= 0){
				int bytes = 0;
				char unused[256];
				sceNetRecv(client_skt,unused,256,0);
				unsigned char* mem = NULL;
				unsigned long mem_size = 0;
				SceDisplayFrameBuf param;
				param.size = sizeof(SceDisplayFrameBuf);
				sceDisplayGetFrameBuf(&param, SCE_DISPLAY_SETBUF_NEXTFRAME);
				jpeg_mem_dest(&cinfo, &mem, &mem_size);
				cinfo.image_width = param.pitch;
				cinfo.image_height = param.height;
				cinfo.input_components = 4;
				cinfo.in_color_space = JCS_EXT_ABGR;
				jpeg_set_defaults(&cinfo);
				cinfo.num_components = 3;
				cinfo.dct_method = JDCT_FASTEST;
				jpeg_set_quality(&cinfo, 100, TRUE);
				JSAMPLE* buffer = (JSAMPLE*)param.base;
				jpeg_start_compress(&cinfo, TRUE);
				int row_stride = cinfo.image_width * 3;
				while( cinfo.next_scanline < cinfo.image_height ){
					row_pointer[0] = &buffer[cinfo.next_scanline * row_stride];
					jpeg_write_scanlines( &cinfo, row_pointer, 1 );
				}
				jpeg_finish_compress( &cinfo );
				char txt[32];
				sprintf(txt, "%ld;", mem_size);
				sceNetSend(client_skt, txt, 32, 0);
				sceNetRecv(client_skt,unused,256,0);
				while (bytes < mem_size){
					sceNetSend(client_skt, &((uint8_t*)mem)[bytes], 4096, 0);
					bytes += 4096;
				}
			}
		}else{
			if (stream_skt != 0xDEADBEEF){
				sceNetSocketClose(client_skt);
				sceNetSocketClose(stream_skt);
				stream_skt = 0xDEADBEEF;
				client_skt = 0;
				sceKernelExitDeleteThread(0);
			}
		}
	}
	return 0;
}

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
		ret=sceNetInit(&initparam);
	}
	ret = sceNetCtlInit();
	SceNetCtlInfo info;
	sceNetCtlInetGetInfo(SCE_NETCTL_INFO_GET_IP_ADDRESS, &info);
	sprintf(vita_ip,"%s",info.ip_address);
	SceNetInAddr vita_addr;
	sceNetInetPton(SCE_NET_AF_INET, info.ip_address, &vita_addr);
	
	// Writing on a temp file request address in order to let main module to know about it
	uint32_t addr = (uint32_t)&request;
	int tmp = sceIoOpen("ux0:/data/rinCheat/addr.bin", SCE_O_WRONLY|SCE_O_CREAT, 0777);
	sceIoWrite(tmp, &addr, 4);
	sceIoClose(tmp);
	tmp = sceIoOpen("ux0:/data/rinCheat/ip.txt", SCE_O_WRONLY|SCE_O_CREAT|SCE_O_TRUNC, 0777);
	sceIoWrite(tmp, vita_ip, strlen(vita_ip));
	sceIoClose(tmp);
	
	SceCtrlData pad, oldpad;
	
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
			default:
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
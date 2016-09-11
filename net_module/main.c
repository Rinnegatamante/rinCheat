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
#include <psp2/sysmodule.h>
#include <psp2/kernel/threadmgr.h>
#include <psp2/net/net.h>
#include <psp2/net/netctl.h>
#include <psp2/ctrl.h>
#include <psp2/io/fcntl.h>
#include "ftpvita.h"

// Requests type
#define NONE 0
#define FTP_SWITCH 1

volatile uint8_t request = NONE;

int main_thread(SceSize args, void *argp) {

	// Internal states
	uint8_t ftp_state = 0;
	uint16_t vita_port;
	uint8_t paused = 0;
	
	// Loading net module
	sceSysmoduleLoadModule(SCE_SYSMODULE_NET);
	
	// Initializing SceNet and SceNetCtl
	char vita_ip[32];
	int ret = sceNetShowNetstat();
	if (ret == SCE_NET_ERROR_ENOTINIT) {
		SceNetInitParam initparam;
		initparam.memory = malloc(1048576);
		initparam.size = 1048576;
		initparam.flags = 0;
		ret=sceNetInit(&initparam);
	}
	if (ret < 0) sceKernelExitDeleteThread(0);
	ret = sceNetCtlInit();
	if (ret < 0) sceKernelExitDeleteThread(0);
	SceNetCtlInfo info;
	sceNetCtlInetGetInfo(SCE_NETCTL_INFO_GET_IP_ADDRESS, &info);
	sprintf(vita_ip,"%s",info.ip_address);
	SceNetInAddr vita_addr;
	sceNetInetPton(SCE_NET_AF_INET, info.ip_address, &vita_addr);
	
	// Writing on a temp file reuqest address in order to let main module to know about it
	uint32_t addr = (uint32_t)&request;
	int tmp = sceIoOpen("ux0:/data/rinCheat/addr.bin", SCE_O_WRONLY|SCE_O_CREAT, 0777);
	sceIoWrite(tmp, &addr, 4);
	sceIoClose(tmp);
	tmp = sceIoOpen("ux0:/data/rinCheat/ip.txt", SCE_O_WRONLY|SCE_O_CREAT|SCE_O_TRUNC, 0777);
	sceIoWrite(tmp, vita_ip, strlen(vita_ip));
	sceIoClose(tmp);
	
	SceCtrlData pad, oldpad;
	
	for (;;){
		sceCtrlPeekBufferPositive(0, &pad, 1);
		if (!paused){
			if ((pad.buttons & SCE_CTRL_SELECT) && (pad.buttons & SCE_CTRL_START)){
				paused = 1;
				sceKernelChangeThreadPriority(0, 0x0);	
			}else sceKernelDelayThread(1000); // Just let VITA scheduler do its work
		}else{
			if ((pad.buttons & SCE_CTRL_START) && (!(oldpad.buttons & SCE_CTRL_START))){
				paused = 0;
				sceKernelChangeThreadPriority(0, 0x40);
			}
		}
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
			default:
				break;
		}
		oldpad = pad;
	}
	
	return 0;
}

int _start(SceSize args, void *argp) {
	SceUID thid = sceKernelCreateThread("rinCheat_net", main_thread, 0x40, 0x400000, 0, 0, NULL);
	if (thid >= 0)
		sceKernelStartThread(thid, 0, NULL);

	return 0;
}
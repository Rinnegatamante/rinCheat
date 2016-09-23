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
#include <string.h>
#include <psp2/types.h>
#include <psp2/kernel/threadmgr.h>
#include <psp2/io/dirent.h>
#include <psp2/io/fcntl.h>
#include <psp2/io/stat.h>
#include "threads.h"

#define THREADS_RANGE 0x100000 // How many thread to scan starting from main thread

extern int net_thread;
extern uint8_t* net_request;

// Generic thread scanner by name
uint32_t searchThreadByName(const char* name){
	int i = 1;
	while (i <= THREADS_RANGE){
		SceKernelThreadInfo status;
		status.size = sizeof(SceKernelThreadInfo);
		int ret = sceKernelGetThreadInfo(main_thread_thid + i, &status);
		if (ret >= 0){
			if (strcmp(status.name, name) == 0) break;
		}
		i++;
	}
	if (i > THREADS_RANGE) return 0; // Not found
	return main_thread_thid + i;
}

// Check if net module is running
uint32_t checkNetModule(){
	uint32_t thid = searchThreadByName("rinCheat_net");
	if (thid != 0){
		uint32_t addr;
		int tmp = sceIoOpen("ux0:/data/rinCheat/addr.bin", SCE_O_RDONLY|SCE_O_CREAT, 0777);
		sceIoRead(tmp, &addr, 4);
		sceIoClose(tmp);
		net_request = (uint8_t*)addr; // Address of the volatile variable used by net module to check requests
	
	}
	return thid;
}
	
/*
 * Tricky way to freeze main thread, we set our plugin priority to 0 (max)
 * and we start two threads with 0 priority in order to get VITA scheduler
 * to always reschedule our threads instead of main one
 */
volatile uint8_t term_dummies = 0;
int dummy_thread(SceSize args, void *argp){
	for (;;){
		if (term_dummies) sceKernelExitDeleteThread(0);
	}
}
void pauseMainThread(){
	sceKernelChangeThreadPriority(0, 0x0);
	if (net_thread != 0) sceKernelChangeThreadPriority(net_thread, 0x0);
	sceKernelChangeThreadPriority(main_thread_thid, 0x7F);
	int i;
	term_dummies = 0;
	for (i=0;i<2;i++){
		SceUID thid = sceKernelCreateThread("dummy thread", dummy_thread, 0x0, 0x10000, 0, 0, NULL);
		if (thid >= 0)
			sceKernelStartThread(thid, 0, NULL);
	}
	SceKernelThreadInfo main_thread_info;
	for(;;){
		main_thread_info.size = sizeof(SceKernelThreadInfo);
		sceKernelGetThreadInfo(main_thread_thid, &main_thread_info);
		if (main_thread_info.status == SCE_THREAD_RUNNING){
			sceKernelDelayThread(1000); // Rescheduling until main thread is in WAITING status
		}else break;
	}
}
void resumeMainThread(){
	term_dummies = 1;
	sceKernelChangeThreadPriority(0, 0x40);
	SceKernelThreadInfo main_thread_info;
	main_thread_info.size = sizeof(SceKernelThreadInfo);
	sceKernelGetThreadInfo(main_thread_thid, &main_thread_info);
	sceKernelChangeThreadPriority(main_thread_thid, main_thread_info.initPriority);
	if (net_thread != 0) sceKernelChangeThreadPriority(net_thread, 0x40);
}
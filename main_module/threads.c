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

#define THREADS_RANGE 0x1000 // How many thread to scan starting from main thread

extern int net_thread;

int searchThreadByName(char* name){
	int i = 1;
	SceKernelThreadInfo status;
	status.size = sizeof(SceKernelThreadInfo);
	while (i <= THREADS_RANGE){
		int ret = sceKernelGetThreadInfo(0x40010003 + i, &status);
		if (ret >= 0 && strcmp(status.name, name) == 0) break;
		i++;
	}
	if (i > THREADS_RANGE) return 0; // Not found
	return 0x40010003 + i;
}

/*
 * Tricky way to freeze main thread, we set our plugin priority to 0 (max)
 * and we start two threads with 0 priority in order to get VITA scheduler
 * to always reschedule our threads instead of main one
 */
volatile int term_stubs = 0;
int dummy_thread(SceSize args, void *argp){
	for (;;){
		if (term_stubs) sceKernelExitDeleteThread(0);
	}
}
void pauseMainThread(){
	sceKernelChangeThreadPriority(0, 0x0);
	int i;
	term_stubs = 0;
	for (i=0;i<((net_thread != 0) ? 1 : 2);i++){
		SceUID thid = sceKernelCreateThread("dummy thread", dummy_thread, 0x0, 0x10000, 0, 0, NULL);
		if (thid >= 0)
			sceKernelStartThread(thid, 0, NULL);
	}
	SceKernelThreadInfo main_thread;
	for(;;){
		main_thread.size = sizeof(SceKernelThreadInfo);
		sceKernelGetThreadInfo(0x40010003, &main_thread);
		sceKernelChangeThreadPriority(0x40010003, 0x7F);
		if (main_thread.status == SCE_THREAD_RUNNING){
			sceKernelDelayThread(1000); // Rescheduling until main thread is in WAITING status
		}else break;
	}
}
void resumeMainThread(){
	term_stubs = 1;
	sceKernelChangeThreadPriority(0, 0x40);
	SceKernelThreadInfo main_thread;
	main_thread.size = sizeof(SceKernelThreadInfo);
	sceKernelGetThreadInfo(0x40010003, &main_thread);
	sceKernelChangeThreadPriority(0x40010003, main_thread.initPriority);
}
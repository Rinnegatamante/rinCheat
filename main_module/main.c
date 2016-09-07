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
#include <psp2/ctrl.h>
#include <psp2/power.h>
#include <psp2/io/dirent.h>
#include <psp2/io/fcntl.h>
#include <psp2/io/stat.h>
#include <psp2/appmgr.h>
#include <psp2/net/net.h>
#include <psp2/net/netctl.h>
#include <psp2/display.h>
#include <psp2/kernel/modulemgr.h>
#include <psp2/kernel/processmgr.h>
#include "blit.h"
#include "memory.h"
#include "threads.h"
#include "savedata.h"
#include "cheats.h"
#include "screenshot.h"

// Menu states
#define MAIN_MENU 0
#define CHEATS_MENU 1
#define HACKS_MENU 2
#define SEARCH_MENU 3
#define CHEATS_LIST 4

// Internal states
#define MENU 0
#define STACK_DUMP 1
#define DO_ABS_SEARCH 2
#define INJECT_STACK 3
#define DO_REL_SEARCH 4
#define SAVE_OFFSETS 5
#define APPLY_CHEAT 6
#define EXPORT_SAVEDATA 7
#define IMPORT_SAVEDATA 8
#define STACK_RESTORE 9
#define DO_ABS_SEARCH_EXT 10
#define FTP_COMMUNICATION 11

// Requests type for net module
#define NONE 0
#define FTP_SWITCH 1

static int freq_list[] = { 111, 166, 222, 266, 333, 366, 444 };
static int search_type[] = {1, 2, 4, 8};

int main_thread(SceSize args, void *argp) {
	
	sceKernelDelayThread(5 * 1000 * 1000);
	sceIoMkdir("ux0:/data/rinCheat", 0777);
	sceIoMkdir("ux0:/data/rinCheat/db", 0777);
	sceIoMkdir("ux0:/data/rinCheat/screenshots", 0777);
	int started = 0;
	int ftp = 0;
	int menu_state = MAIN_MENU;
	int int_state = MENU;
	int old_int_state = int_state;
	int old_menu_state = menu_state;
	int menu_idx = 0;
	int screenshot = 0;
	int auto_suspend = 1;
	int search_id = 2;
	char search_val[] = "0000000000000000";
	int search_idx = 7;
	char temp[128];
	uint64_t dval = 0;
	
	// Loading net module
	sceKernelLoadStartModule("ux0:/data/rinCheat/net_module.suprx", 0, NULL, 0, NULL, NULL);
	sceKernelDelayThread(1 * 1000 * 1000); // Wait till net module did its stuffs
	uint32_t addr;
	int tmp = sceIoOpen("ux0:/data/rinCheat/addr.bin", SCE_O_RDONLY|SCE_O_CREAT, 0777);
	sceIoRead(tmp, &addr, 4);
	sceIoClose(tmp);
	uint8_t* net_request = (uint8_t*)addr; // Address of the volatile variable used by net module to check requests
	
	// Attaching game main thread
	SceKernelThreadInfo status;
	status.size = sizeof(SceKernelThreadInfo);
	sceKernelGetThreadInfo(0x40010003, &status);
	
	// Check if we'll use RAM or MMC storage
	SceCtrlData pad, oldpad;
	sceCtrlPeekBufferPositive(0, &oldpad, 1);
	if (!(oldpad.buttons & SCE_CTRL_LTRIGGER)){
		uint8_t* test = malloc(status.stackSize);
		if (test != NULL){
			ram_mode = 1;
			free(test);
		}
	}
	
	// Getting title info
	char titleid[16], title[256];
	sceAppMgrAppParamGetString(0, 9, title , 256);
	sceAppMgrAppParamGetString(0, 12, titleid , 256);
	
	// Opening db file for target game
	cheatDB* db = NULL;
	cheatDB* cur = NULL;
	cheatDB* sel = NULL;
	char db_file[128];
	sprintf(db_file,"ux0:/data/rinCheat/db/%s.txt", titleid);
	db = loadCheatsDatabase(db_file, db);
	
	// Menus setup
	char* menus[] = {"Main Menu", "Cheats Menu", "Hacks Menu", "Search Value", "Cheats List"};
	char* opt_main[] = {"Game Cheats","Game Hacks","Export decrypted savedata","Import decrypted savedata","FTP State: "};
	char* opt_cheats[] = {"Cheats List","Search for value", "Dump stack to ux0:/stack.bin", "Inject stack from ux0:/stack.bin", "Return Main Menu"};
	char* opt_hacks[] = {"CPU Clock: ","BUS Clock: ","GPU Clock: ","Auto-Suspend: ", "Screenshot Feature: ","Return Main Menu"};
	char* opt_search[] = {"Value: ","Type: ","Start Absolute Search on Stack","Start Absolute Search on Stack and Heap (Experimental)","Start Relative Search","Inject Value","Save offsets","Return Cheats Menu"};
	char** opt[] = {opt_main, opt_cheats, opt_hacks, opt_search};
	int num_opt[] = {5, 5, 6, 8};
	
	// Main loop
	for (;;){
		
		// Auto Suspend Hack
		if (!auto_suspend) sceKernelPowerTick(1);
		
		sceCtrlPeekBufferPositive(0, &pad, 1);
		
		// Screenshot feature
		if (screenshot){
			if ((pad.buttons & SCE_CTRL_START) && (pad.buttons & SCE_CTRL_LTRIGGER) && (pad.buttons & SCE_CTRL_RTRIGGER)){
				if (!started) pauseMainThread();
				takeScreenshot(titleid);
				if (!started) resumeMainThread();
			}
		}
		
		if (started){
			sceDisplayWaitVblankStart();
			blit_setup();
			uint64_t i;
			if (menu_state != CHEATS_LIST) blit_stringf(5, 5, "rinCheat v.0.1 - %s", menus[menu_state]);
			else blit_stringf(5, 5, "rinCheat v.0.1 - %s (%d available)", menus[menu_state],numCheats);
			int m_idx = 0;
			int y = 35;
			if (int_state != old_int_state){ 
				blit_clearscreen();
				old_int_state = int_state;
			}else if (menu_state != old_menu_state){
				blit_clearscreen();
				old_menu_state = menu_state;
			}
			switch (int_state){
			
				case MENU:
			
					// Drawing Menu
					if (menu_state != CHEATS_LIST){ // Static menus
						while (m_idx < num_opt[menu_state]){
							uint32_t clr = 0x00ffffff;
							if (m_idx == menu_idx) clr = 0x0000ffff;
							blit_set_color(clr);
							if (menu_state == HACKS_MENU){
								switch (m_idx){
									case 0:
										blit_stringf(5, y, "%s%d", opt[menu_state][m_idx], scePowerGetArmClockFrequency());
										break;
									case 1:
										blit_stringf(5, y, "%s%d", opt[menu_state][m_idx], scePowerGetBusClockFrequency());
										break;
									case 2:
										blit_stringf(5, y, "%s%d", opt[menu_state][m_idx], scePowerGetGpuClockFrequency());
										break;
									case 3:
										blit_stringf(5, y, "%s%s", opt[menu_state][m_idx], auto_suspend ? "On" : "Off");
										break;
									case 4:
										blit_stringf(5, y, "%s%s", opt[menu_state][m_idx], screenshot ? "On" : "Off");
										break;
									default:
										blit_stringf(5, y, opt[menu_state][m_idx]);
										break;
								}
							}else if (menu_state == MAIN_MENU){
								switch (m_idx){
									case 4:
										blit_stringf(5, y, "%s%s", opt[menu_state][m_idx], ftp ? "On" : "Off");
										break;
									default:
										blit_stringf(5, y, opt[menu_state][m_idx]);
										break;
								}
							}else if (menu_state == SEARCH_MENU){
								switch (m_idx){
									case 0:
										blit_set_color(0x00ffffff);
										blit_stringf(5, y, "%s0x%s (%llu)", opt[menu_state][m_idx], &search_val[14-((search_type[search_id]-1)<<1)], dval);
										blit_set_color(clr);
										blit_stringf(5, y, "%s", opt[menu_state][m_idx]);
										blit_stringf(149+16*search_idx, y, "%c", search_val[14-((search_type[search_id]-1)<<1)+search_idx]);
										break;
									case 1:
										blit_stringf(5, y, "%s%d Bytes", opt[menu_state][m_idx], search_type[search_id]);
										break;
									default:
										blit_stringf(5, y, opt[menu_state][m_idx]);
										break;
								}
							}else blit_stringf(5, y, opt[menu_state][m_idx]);
							m_idx++;
							y += 20;
						}
					}else{ // Dynamic menus
						cur = db;
						while (y <= 460 && cur != NULL){
							if (m_idx >= menu_idx){
								uint32_t clr = 0x00ffffff;
								if (m_idx == menu_idx){
									clr = 0x0000ffff;
									sel = cur;
								}
								blit_set_color(clr);
								blit_stringf(5, y, "%s [%s]", cur->name, cur->state ? "Enabled" : "Disabled");
								y+=20;
							}
							m_idx++;
							cur = cur->next;
						}
					}
					blit_set_color(0x00ffffff);
					
					// Search menu results
					if (menu_state == SEARCH_MENU && results_num != -2){
						if (results_num == 0) blit_stringf(5, 225, "No matches");
						else blit_stringf(5, 225, "Found %d matches", results_num);
					}
					
					// Functions
					int i, freq;
					uint64_t val;
					if ((pad.buttons & SCE_CTRL_CROSS) && (!(oldpad.buttons & SCE_CTRL_CROSS))){
						switch (menu_state){
							case MAIN_MENU:
								switch (menu_idx){
									case 0:
										menu_idx = 0;
										menu_state = CHEATS_MENU;
										break;
									case 1:
										menu_idx = 0;
										menu_state = HACKS_MENU;
										break;
									case 2:
										int_state = EXPORT_SAVEDATA;
										break;
									case 3:
										int_state = IMPORT_SAVEDATA;
										break;
									case 4:
										int_state = FTP_COMMUNICATION;
										break;
								}								
								break;
							case CHEATS_MENU:
								switch (menu_idx){
									case 0:
										menu_idx = 0;
										menu_state = CHEATS_LIST;
										break;
									case 1:
										menu_idx = 0;
										menu_state = SEARCH_MENU;
										break;
									case 2:
										int_state = STACK_DUMP;
										break;
									case 3:
										int_state = STACK_RESTORE;
										break;
									case 4:
										menu_idx = 0;
										menu_state = MAIN_MENU;
										break;
								}								
								break;
							case HACKS_MENU:
								switch (menu_idx){
									case 0:
										i = 0;
										freq = scePowerGetArmClockFrequency();
										while(freq_list[i] != freq){i++;}
										if (freq < 444) scePowerSetArmClockFrequency(freq_list[i+1]);
										else scePowerSetArmClockFrequency(111);
										break;
									case 1:
										i = 0;
										freq = scePowerGetBusClockFrequency();
										while(freq_list[i] != freq){i++;}
										if (freq < 222) scePowerSetBusClockFrequency(freq_list[i+1]);
										else scePowerSetBusClockFrequency(111);
										break;
									case 2:
										i = 0;
										freq = scePowerGetGpuClockFrequency();
										while(freq_list[i] != freq){i++;}
										if (freq < 222) scePowerSetGpuClockFrequency(freq_list[i+1]);
										else scePowerSetGpuClockFrequency(111);
										break;
									case 3:
										blit_clearscreen();
										auto_suspend = !auto_suspend;
										break;
									case 4:
										blit_clearscreen();
										screenshot = !screenshot;
										break;
									case 5:
										menu_idx = 1;
										menu_state = MAIN_MENU;
										break;
								}								
								break;
							case SEARCH_MENU:
								switch (menu_idx){
									case 0:
										i = search_val[14-((search_type[search_id]-1)<<1)+search_idx]-0x30;
										if (i < 0x16){
											if (i != 0x09) search_val[14-((search_type[search_id]-1)<<1)+search_idx]++;
											else search_val[14-((search_type[search_id]-1)<<1)+search_idx]+=8; // Jumping from 9 to A on ASCII table
											sscanf(search_val,"%llX",&dval);
											switch (search_type[search_id]){
												case 1:
													dval = (dval<<56)>>56;
													break;
												case 2:
													dval = (dval<<48)>>48;
													break;
												case 4:
													dval = (dval<<32)>>32;
												case 8:
													break;
											}
										}
										break;
									case 1:
										search_id++;
										if (search_id > 3) search_id = 0;
										search_idx = (search_type[search_id]<<1)-1;
										sscanf(search_val,"%llX",&dval);
										switch (search_type[search_id]){
											case 1:
												dval = (dval<<56)>>56;
												break;
											case 2:
												dval = (dval<<48)>>48;
												break;
											case 4:
												dval = (dval<<32)>>32;
											case 8:
												break;
										}
										break;
									case 2:
										int_state = DO_ABS_SEARCH;
										break;
									case 3:
										int_state = DO_ABS_SEARCH_EXT;
										break;
									case 4:
										if (results_num != -2){
											int_state = DO_REL_SEARCH;
										}
										break;
									case 5:
										if (results_num != -2){
											int_state = INJECT_STACK;
										}
										break;
									case 6:
										if (results_num != -2){
											int_state = SAVE_OFFSETS;
										}
										break;
									case 7:
										menu_idx = 1;
										menu_state = CHEATS_MENU;
										break;
								}
								break;
							case CHEATS_LIST:
								if (numCheats > 0) int_state = APPLY_CHEAT;
								break;
						}
					}else if ((pad.buttons & SCE_CTRL_TRIANGLE) && (!(oldpad.buttons & SCE_CTRL_TRIANGLE))){
						switch (menu_state){
							case MAIN_MENU:
								break;
							case CHEATS_MENU:
								menu_idx = 0;
								menu_state = MAIN_MENU;
								break;
							case HACKS_MENU:
								menu_idx = 1;
								menu_state = MAIN_MENU;
								break;
							case SEARCH_MENU:
								menu_idx = 1;
								menu_state = CHEATS_MENU;
								break;
							case CHEATS_LIST:
								menu_idx = 0;
								menu_state = CHEATS_MENU;
								break;
						}
					}else if ((pad.buttons & SCE_CTRL_SQUARE) && (!(oldpad.buttons & SCE_CTRL_SQUARE))){
						switch (menu_state){
							case SEARCH_MENU:
								blit_clearscreen();
								if (menu_idx == 0){
									i = search_val[14-((search_type[search_id]-1)<<1)+search_idx]-0x30;
									if (i > 0){
										if (i != 0x11) search_val[14-((search_type[search_id]-1)<<1)+search_idx]--;
										else search_val[14-((search_type[search_id]-1)<<1)+search_idx]-=8; // Jumping from A to 9 on ASCII table
										sscanf(search_val,"%llX",&dval);
										switch (search_type[search_id]){
											case 1:
												dval = (dval<<56)>>56;
												break;
											case 2:
												dval = (dval<<48)>>48;
												break;
											case 4:
												dval = (dval<<32)>>32;
											case 8:
												break;
										}
									}
									break;
								}
							default:
								break;
						}
					}else if ((pad.buttons & SCE_CTRL_LTRIGGER) && (!(oldpad.buttons & SCE_CTRL_LTRIGGER))){
						switch (menu_state){
							case SEARCH_MENU:
								if (menu_idx == 0){
									search_idx--;
									if (search_idx < 0) search_idx++;
								}
							default:
								break;
						}
					}else if ((pad.buttons & SCE_CTRL_RTRIGGER) && (!(oldpad.buttons & SCE_CTRL_RTRIGGER))){
						switch (menu_state){
							case SEARCH_MENU:
								if (menu_idx == 0){
									search_idx++;
									if (search_idx > (search_type[search_id]<<1)-1) search_idx--;
								}
							default:
								break;
						}
					}else if ((pad.buttons & SCE_CTRL_DOWN) && (!(oldpad.buttons & SCE_CTRL_DOWN))){
						menu_idx++;
						if (menu_state != CHEATS_LIST){	if (menu_idx >= num_opt[menu_state]) menu_idx = 0;
						}else if (menu_idx >= numCheats) menu_idx = 0;
					}else if ((pad.buttons & SCE_CTRL_UP) && (!(oldpad.buttons & SCE_CTRL_UP))){
						menu_idx--;
						if (menu_idx < 0){
							if (menu_state != CHEATS_LIST) menu_idx = num_opt[menu_state]-1;
							else menu_idx = numCheats-1;
						}
					}
					
					blit_stringf(5, 480, "Target info: ");
					blit_stringf(5, 500, "Title: %s", title);
					blit_stringf(5, 520, "TitleID: %s", titleid);
					blit_stringf(830, 520, ram_mode ? "RAM Mode" : "MMC Mode");
					if ((pad.buttons & SCE_CTRL_START) && (!(oldpad.buttons & SCE_CTRL_START))){
						started = 0;
						resumeMainThread();
					}
					break;
					
				case STACK_DUMP:	
				
					blit_stringf(5, 35, "Dumping stack, please wait");
					int fd = sceIoOpen("ux0:/data/rinCheat/stack.bin", SCE_O_WRONLY | SCE_O_CREAT,0777);
					sceIoWrite(fd, status.stack, status.stackSize);
					sceIoClose(fd);
					int_state = MENU;
					break;
					
				case DO_ABS_SEARCH:	
				
					blit_stringf(5, 35, "Scanning stack, please wait");
					results_num = scanStack(status.stack,status.stackSize,dval,search_type[search_id]);
					int_state = MENU;
					break;
					
				case DO_REL_SEARCH:	
				
					blit_stringf(5, 35, "Scanning stack, please wait");
					scanResults(dval,search_type[search_id]);
					int_state = MENU;
					break;
					
				case INJECT_STACK:
				
					blit_stringf(5, 35, "Injecting stack, please wait");
					sscanf(search_val,"%llX",&val);
					switch (search_type[search_id]){
						case 1:
							val = (val<<56)>>56;
							break;
						case 2:
							val = (val<<48)>>48;
							break;
						case 4:
							val = (val<<32)>>32;
						case 8:
							break;
					}
					injectMemory(val, search_type[search_id]);
					int_state = MENU;
					break;
					
				case SAVE_OFFSETS:
				
					blit_stringf(5, 35, "Saving offsets, please wait");
					char offs_file[128];
					sprintf(offs_file,"ux0:/data/rinCheat/db/%s_offsets.txt", titleid);
					saveOffsets(offs_file);
					int_state = MENU;
					break;
					
				case APPLY_CHEAT:
					
					sel->state = !sel->state;
					if (sel->state){
						blit_stringf(5, 35, "Applying cheat, please wait");
						injectValue((uint8_t*)sel->offset, sel->val, sel->size);
					}
					int_state = MENU;
					break;
					
				case EXPORT_SAVEDATA:
					
					blit_stringf(5, 35, "Exporting savedata, please wait");
					sprintf(temp,"ux0:/data/rinCheat/%s_SAVEDATA", titleid);
					dumpSavedataDir("savedata0:",temp);	
					int_state = MENU;
					break;
					
				case IMPORT_SAVEDATA:
					
					blit_stringf(5, 35, "Importing savedata, please wait");
					sprintf(temp,"ux0:/data/rinCheat/%s_SAVEDATA", titleid);
					restoreSavedataDir(temp, NULL);
					int_state = MENU;
					break;
					
				case STACK_RESTORE:
					
					blit_stringf(5, 35, "Importing stack file, please wait");
					injectStackFile(status.stack,status.stackSize,"ux0:/data/rinCheat/stack.bin");
					break;
					
				case DO_ABS_SEARCH_EXT:
				
					blit_stringf(5, 35, "Scanning memory, please wait");
					results_num = scanStack(status.stack,status.stackSize,dval,search_type[search_id]);
					results_num += scanHeap(dval, search_type[search_id]);
					int_state = MENU;
					break;
					
				case FTP_COMMUNICATION:
					
					blit_stringf(5, 35, "Sending request to net module, please wait");
					ftp = !ftp;
					net_request[0] = FTP_SWITCH;
					int_state = MENU;
					break;
					
			}
		}else{
			if ((pad.buttons & SCE_CTRL_SELECT) && (pad.buttons & SCE_CTRL_START)){
				pauseMainThread();
				started = 1;
				menu_state = MAIN_MENU;
				menu_idx = 0;
				blit_clearscreen();
			}else sceKernelDelayThread(1000); // Invoking scheduler to not slowdown games
		}
		oldpad = pad;
	}

	return 0;
}

int _start(SceSize args, void *argp) {
	SceUID thid = sceKernelCreateThread("rinCheat", main_thread, 0x40, 0x600000, 0, 0, NULL);
	if (thid >= 0)
		sceKernelStartThread(thid, 0, NULL);

	return 0;
}
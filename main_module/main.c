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
#include "filesystem.h"
#include "screenshot.h"

// Menu states
enum{
	MAIN_MENU = 0,
	CHEATS_MENU,
	HACKS_MENU,
	SEARCH_MENU,
	NET_MENU,
	SAVEDATA_MENU,
	CHEATS_LIST
};

// Internal states
enum{
	MENU = 0,
	STACK_DUMP,
	DO_ABS_SEARCH,
	INJECT_MEMORY,
	DO_REL_SEARCH,
	SAVE_OFFSETS,
	APPLY_CHEAT,
	EXPORT_SAVEDATA,
	IMPORT_SAVEDATA,
	STACK_RESTORE,
	DO_ABS_SEARCH_EXT,
	FTP_COMMUNICATION,
	STREAM_COMMUNICATION,
	CHANGE_STREAM_QUALITY
};

// Requests type for net module
enum{
	NONE = 0,
	FTP_SWITCH,
	STREAM_SWITCH,
	SET_LOWEST_QUALITY = 251,
	SET_LOW_QUALITY = 252,
	SET_NORMAL_QUALITY = 253,
	SET_HIGH_QUALITY = 254,
	SET_BEST_QUALITY = 255
};

// Color values
#define WHITE 0x00ffffff
#define YELLOW 0x0000ffff
#define RED 0x000000ff
#define GREEN 0x0000ff00

static int freq_list[] = { 111, 166, 222, 266, 333, 366, 444 };
uint8_t quality_list[] = {255, 200, 128, 64, 0};
static int search_type[] = {1, 2, 4, 8};
int net_thread = 0;

int main_thread(SceSize args, void *argp) {
	
	// Waiting a bit to let the game inits its stuffs
	sceKernelDelayThread(5 * 1000 * 1000);
	
	// Creating required folders for rinCheat if they don't exist
	sceIoMkdir("ux0:/data/rinCheat", 0777);
	sceIoMkdir("ux0:/data/rinCheat/db", 0777);
	sceIoMkdir("ux0:/data/rinCheat/screenshots", 0777);
	sceIoMkdir("ux0:/data/rinCheat/settings", 0777);
	sceIoMkdir("ux0:/data/savegames", 0777);
	
	// Internal stuffs
	uint8_t saveslot = 0;
	int hmax, pwidth, pheight;
	int started = 0;
	int ftp = 0;
	int pc_stream = 0;
	int menu_state = MAIN_MENU;
	int int_state = MENU;
	int old_int_state = int_state;
	int old_menu_state = menu_state;
	int menu_idx = 0;
	int tmp, size;
	int heap_scanner;
	int search_id = 2;
	char search_val[] = "0000000000000000";
	uint8_t slotstate[] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
	int search_idx = 7;
	char temp[128];
	char vita_ip[32];
	uint8_t quality_idx = 0;
	uint8_t setup = 0;
	uint64_t dval = 0;
	
	// Getting title info
	char titleid[16], title[256];
	sceAppMgrAppParamGetString(0, 9, title , 256);
	sceAppMgrAppParamGetString(0, 12, titleid , 256);
	
	// Creating saveslots directories if they don't exist
	sprintf(temp, "ux0:/data/savegames/%s", titleid);
	sceIoMkdir(temp, 0777);
	for (tmp=0; tmp<=9; tmp++){
		sprintf(temp, "ux0:/data/savegames/%s/SLOT%d", titleid, tmp);
		sceIoMkdir(temp, 0777);
		slotstate[tmp] = isDirectoryEmpty(temp);
	}
	
	// Loading default settings file if it exists
	settings cfg;
	if (loadTitleSettings(titleid, &cfg) == 0){
		scePowerSetArmClockFrequency(cfg.cpu_clock);
		scePowerSetGpuClockFrequency(cfg.gpu_clock);
		scePowerSetBusClockFrequency(cfg.bus_clock);
		scePowerSetGpuXbarClockFrequency(cfg.gpu_xbar_clock);
	}
	
	// Loading net module
	SceCtrlData pad, oldpad;
	sceCtrlPeekBufferPositive(0, &oldpad, 1);
	if ((!(oldpad.buttons & SCE_CTRL_RTRIGGER)) && cfg.net) sceKernelLoadStartModule("ux0:/data/rinCheat/net_module.suprx", 0, NULL, 0, NULL, NULL);
	
	// Attaching game main thread
	SceKernelThreadInfo status;
	status.size = sizeof(SceKernelThreadInfo);
	main_thread_thid = 0x40010003;
	int ret = sceKernelGetThreadInfo(main_thread_thid, &status);
	
	// Oreshika apparently uses this thid, maybe even other games uses it?
	if (ret < 0){
		main_thread_thid = 0x40010005;
		sceKernelGetThreadInfo(main_thread_thid, &status);
	}
	
	// Check if we'll use RAM or MMC mode
	if (!(oldpad.buttons & SCE_CTRL_LTRIGGER)){
		uint8_t* test = malloc(status.stackSize);
		if (test != NULL){
			ram_mode = 1;
			free(test);
		}
	}
	
	// Opening db file for target game
	cheatDB* db = NULL;
	cheatDB* cur = NULL;
	cheatDB* sel = NULL;
	char db_file[128];
	sprintf(db_file,"ux0:/data/rinCheat/db/%s.txt", titleid);
	db = loadCheatsDatabase(db_file, db);
	
	// Menus setup
	char* menus[] = {"Main Menu", "Cheats Menu", "Hacks Menu", "Search Value", "Net Module Menu", "Savedata Menu", "Cheats List"};
	char* opt_main[] = {"Game Cheats","Game Hacks","Net Module","Manage Savedatas"};
	char* opt_cheats[] = {"Cheats List","Search for value", "Dump stack to ux0:/data/rinCheat/stack.bin", "Inject stack from ux0:/data/rinCheat/stack.bin", "Return Main Menu"};
	char* opt_hacks[] = {"CPU Clock: ","BUS Clock: ","GPU Clock: ", "GPU Crossbar Clock: ", "Auto-Suspend: ", "Screenshot Feature: ","Return Main Menu"};
	char* opt_search[] = {"Value: ","Type: ","Start Absolute Search on Stack","Start Absolute Search on Stack and Heap (Experimental)","Start Relative Search","Inject Value","Save offsets","Return Cheats Menu"};
	char* opt_net[] = {"FTP State: ", "Stream Screen to PC: ", "Screen Stream Video Quality: ", "Return Main Menu"};
	char* opt_qualities[] = {"Lowest", "Low", "Normal", "High", "Best"};
	char* opt_savedata[] = {"Current Slot: ", "State: ", "Import decrypted savedata", "Export decrypted savedata", "Return Main Menu"};
	char** opt[] = {opt_main, opt_cheats, opt_hacks, opt_search, opt_net, opt_savedata};
	int num_opt[] = {4, 5, 7, 8, 4, 5};
	
	// Main loop
	for (;;){
		
		// Auto Suspend Hack
		if (!cfg.suspend) sceKernelPowerTick(0);
		
		sceCtrlPeekBufferPositive(0, &pad, 1);
		
		// Screenshot feature
		if (cfg.screenshot){
			if ((pad.buttons & SCE_CTRL_START) && (pad.buttons & SCE_CTRL_LTRIGGER) && (pad.buttons & SCE_CTRL_RTRIGGER)){
				if (!started) pauseMainThread();
				takeScreenshot(titleid);
				if (!started) resumeMainThread();
			}
		}
		
		if (started){
			sceDisplayWaitVblankStart();
			blit_setup();
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
							uint32_t clr = WHITE;
							if (m_idx == menu_idx) clr = YELLOW;
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
										blit_stringf(5, y, "%s%d", opt[menu_state][m_idx], scePowerGetGpuXbarClockFrequency());
										break;
									case 4:
										blit_stringf(5, y, "%s%s", opt[menu_state][m_idx], cfg.suspend ? "On" : "Off");
										break;
									case 5:
										blit_stringf(5, y, "%s%s", opt[menu_state][m_idx], cfg.screenshot ? "On" : "Off");
										break;
									default:
										blit_stringf(5, y, opt[menu_state][m_idx]);
										break;
								}
							}else if (menu_state == NET_MENU){
								switch (m_idx){
									case 0:
										blit_stringf(5, y, "%s%s%s", opt[menu_state][m_idx], ftp ? "On - Listening on " : "Off", ftp ? vita_ip : "");
										break;
									case 1:
										blit_stringf(5, y, "%s%s%s", opt[menu_state][m_idx], pc_stream ? "On - Listening on " : "Off", pc_stream ? vita_ip : "");
										break;
									case 2:
										blit_stringf(5, y, "%s%s", opt[menu_state][m_idx], opt_qualities[quality_idx]);
										break;
									default:
										blit_stringf(5, y, opt[menu_state][m_idx]);
										break;
								}
							}else if (menu_state == SAVEDATA_MENU){
								switch (m_idx){
									case 0:
										blit_stringf(5, y, "%s%hhu", opt[menu_state][m_idx], saveslot);
										break;
									case 1:
										blit_set_color(slotstate[saveslot] ? GREEN : RED);
										blit_stringf(5, y, "%s%s", opt[menu_state][m_idx], slotstate[saveslot] ? "Full" : "Empty");
										blit_set_color(WHITE);
										break;
									default:
										blit_stringf(5, y, opt[menu_state][m_idx]);
										break;
								}
							}else if (menu_state == SEARCH_MENU){
								switch (m_idx){
									case 0:
										blit_set_color(WHITE);
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
						while (y <= hmax && cur != NULL){
							if (m_idx >= menu_idx){
								uint32_t clr = WHITE;
								if (m_idx == menu_idx){
									clr = YELLOW;
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
					blit_set_color(WHITE);
					
					// Search menu results
					if (menu_state == SEARCH_MENU && results_num != -2){
						if (results_num == 0) blit_stringf(5, 225, "No matches");
						else blit_stringf(5, 225, "Found %d matches", results_num);
					}
					
					// Extra features status
					if (menu_state == MAIN_MENU){
						if (net_thread != 0){
							blit_set_color(GREEN);
							blit_stringf(5, 225, "NET MODULE: running");
						}else{
							blit_set_color(RED);
							blit_stringf(5, 225, "NET MODULE: exited");
						}
						if (heap_scanner != -1){
							blit_set_color(GREEN);
							blit_stringf(5, 245, "HEAP SCANNER: ready");
						}else{
							blit_set_color(RED);
							blit_stringf(5, 245, "HEAP SCANNER: unavailable");
						}
						blit_set_color(WHITE);
					}
					
					// Functions
					int i, freq;
					uint64_t val;
					if ((pad.buttons & SCE_CTRL_CROSS) && (!(oldpad.buttons & SCE_CTRL_CROSS))){
						switch (menu_state){
							case MAIN_MENU:
								switch (menu_idx){
									case 0: // Open Game Cheats Menu
										menu_idx = 0;
										menu_state = CHEATS_MENU;
										break;
									case 1: // Open Game Hacks Menu
										menu_idx = 0;
										menu_state = HACKS_MENU;
										break;
									case 2: // Open Net Module Menu
										if (net_thread != 0){
											menu_idx = 0;
											menu_state = NET_MENU;
										}
										break;
									case 3: // Open Manage Savedata 
										menu_idx = 0;
										menu_state = SAVEDATA_MENU;
								}								
								break;
							case CHEATS_MENU:
								switch (menu_idx){
									case 0: // Open Cheats List Menu
										menu_idx = 0;
										menu_state = CHEATS_LIST;
										break;
									case 1:
										menu_idx = 0; // Open Search Menu
										menu_state = SEARCH_MENU;
										break;
									case 2: // Execute a main thread stack dump
										int_state = STACK_DUMP;
										break;
									case 3: // Execute a main thread stack restore
										int_state = STACK_RESTORE;
										break;
									case 4: // Return Main Menu
										menu_idx = 0;
										menu_state = MAIN_MENU;
										break;
								}								
								break;
							case HACKS_MENU:
								switch (menu_idx){
									case 0: // Change CPU Clock
										i = 0;
										freq = scePowerGetArmClockFrequency();
										while(freq_list[i] != freq){i++;}
										if (freq < 444) scePowerSetArmClockFrequency(freq_list[i+1]);
										else scePowerSetArmClockFrequency(111);
										if (freq == scePowerGetArmClockFrequency()) scePowerSetArmClockFrequency(111);
										cfg.cpu_clock = scePowerGetArmClockFrequency();
										break;
									case 1: // Change BUS Clock
										i = 0;
										freq = scePowerGetBusClockFrequency();
										while(freq_list[i] != freq){i++;}
										if (freq < 222) scePowerSetBusClockFrequency(freq_list[i+1]);
										else scePowerSetBusClockFrequency(111);
										if (freq == scePowerGetBusClockFrequency()) scePowerSetBusClockFrequency(111);
										cfg.bus_clock = scePowerGetArmClockFrequency();
										break;
									case 2: // Change GPU Clock
										i = 0;
										freq = scePowerGetGpuClockFrequency();
										while(freq_list[i] != freq){i++;}
										if (freq < 222) scePowerSetGpuClockFrequency(freq_list[i+1]);
										else scePowerSetGpuClockFrequency(111);
										if (freq == scePowerGetGpuClockFrequency()) scePowerSetGpuClockFrequency(111);
										cfg.gpu_clock = scePowerGetArmClockFrequency();
										break;
									case 3: // Change GPU Crossbar Clock
										i = 0;
										freq = scePowerGetGpuXbarClockFrequency();
										while(freq_list[i] != freq){i++;}
										if (freq < 222) scePowerSetGpuXbarClockFrequency(freq_list[i+1]);
										else scePowerSetGpuXbarClockFrequency(111);
										if (freq == scePowerGetGpuXbarClockFrequency()) scePowerSetGpuXbarClockFrequency(111);
										cfg.gpu_xbar_clock = scePowerGetGpuXbarClockFrequency();
										break;
									case 4: // Enable/Disable AutoSuspend Hack
										blit_clearscreen();
										cfg.suspend = !cfg.suspend;
										break;
									case 5: // Enable/Disable Screenshots Feature
										blit_clearscreen();
										cfg.screenshot = !cfg.screenshot;
										break;
									case 6: // Return Main Menu
										menu_idx = 1;
										menu_state = MAIN_MENU;
										break;
								}								
								break;
							case SEARCH_MENU:
								switch (menu_idx){
									case 0: // Change Selected Byte Value
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
									case 1: // Change Value Size
										blit_clearscreen();
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
									case 2: // Execute Absolute Search on Stack
										int_state = DO_ABS_SEARCH;
										break;
									case 3: // Execute Absolute Search on Stack+Heap
										int_state = DO_ABS_SEARCH_EXT;
										break;
									case 4: // Execute Relative Search
										if (results_num != -2){
											int_state = DO_REL_SEARCH;
										}
										break;
									case 5: // Inject new value
										if (results_num != -2){
											int_state = INJECT_MEMORY;
										}
										break;
									case 6: // Save found offsets
										if (results_num != -2){
											int_state = SAVE_OFFSETS;
										}
										break;
									case 7: // Return Cheats Menu
										menu_idx = 1;
										menu_state = CHEATS_MENU;
										break;
								}
								break;
							case NET_MENU:
								switch (menu_idx){
									case 0: // Enable/Disable FTP Server
										if (net_thread != 0) int_state = FTP_COMMUNICATION;
										break;
									case 1: // Enable/Disable Screen Streaming
										if (net_thread != 0) int_state = STREAM_COMMUNICATION;
										break;
									case 2: // Change Stream Video Quality
										quality_idx++;
										if (quality_idx > 4) quality_idx = 0;
										int_state = CHANGE_STREAM_QUALITY;
										break;
									case 3: // Return Main Menu
										menu_idx = 2;
										menu_state = MAIN_MENU;
										break;
								}
								break;
							case SAVEDATA_MENU:
								switch (menu_idx){
									case 0: // Change Selected Saveslot
										blit_clearscreen();
										saveslot++;
										if (saveslot == 10) saveslot = 0;
										break;
									case 2: // Import a Savedata
										if (slotstate[saveslot]) int_state = IMPORT_SAVEDATA;
										break;
									case 3: // Export a Savedata
										int_state = EXPORT_SAVEDATA;
										break;
									case 4:
										menu_idx = 3; // Return Main Menu
										menu_state = MAIN_MENU;
										break;
								}
								break;
							case CHEATS_LIST: // Apply a cheat
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
							case NET_MENU:
								menu_idx = 2;
								menu_state = MAIN_MENU;
								break;
						}
					}else if ((pad.buttons & SCE_CTRL_SQUARE) && (!(oldpad.buttons & SCE_CTRL_SQUARE))){
						switch (menu_state){
							case SEARCH_MENU:
								blit_clearscreen();
								if (menu_idx == 0){ // Change Selected Byte Value
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
								if (menu_idx == 0){ // Change Selected Byte
									search_idx--;
									if (search_idx < 0) search_idx++;
								}
							default:
								break;
						}
					}else if ((pad.buttons & SCE_CTRL_RTRIGGER) && (!(oldpad.buttons & SCE_CTRL_RTRIGGER))){
						switch (menu_state){
							case SEARCH_MENU:
								if (menu_idx == 0){ // Change Selected Byte
									search_idx++;
									if (search_idx > (search_type[search_id]<<1)-1) search_idx--;
								}
							default:
								break;
						}
					}else if ((pad.buttons & SCE_CTRL_DOWN) && (!(oldpad.buttons & SCE_CTRL_DOWN))){
						menu_idx++;
						if (menu_state == CHEATS_LIST && menu_idx >= numCheats) menu_idx = 0;
						else if (menu_state == SAVEDATA_MENU && menu_idx == 1) menu_idx++;
						else if (menu_state != CHEATS_LIST && menu_idx >= num_opt[menu_state]) menu_idx = 0;
						if (menu_state == CHEATS_LIST) blit_clearscreen();
					}else if ((pad.buttons & SCE_CTRL_UP) && (!(oldpad.buttons & SCE_CTRL_UP))){
						menu_idx--;
						if (menu_idx < 0){
							if (menu_state == CHEATS_LIST) menu_idx = numCheats-1;
							else menu_idx = num_opt[menu_state]-1;
						}else if (menu_state == SAVEDATA_MENU && menu_idx == 1) menu_idx--;
						if (menu_state == CHEATS_LIST) blit_clearscreen();
					}else if ((pad.buttons & SCE_CTRL_START) && (!(oldpad.buttons & SCE_CTRL_START))){
						started = 0;
						saveTitleSettings(titleid, &cfg);
						resumeMainThread();
					}
					
					// Plugin State and Target Info Showing
					blit_stringf(5, hmax-64, "Target info: ");
					blit_stringf(5, hmax-44, "Title: %s", title);
					blit_stringf(5, hmax-24, "TitleID: %s", titleid);
					blit_stringf(pwidth-130, hmax-24, ram_mode ? "RAM Mode" : "MMC Mode");
					
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
					
				case INJECT_MEMORY:
				
					blit_stringf(5, 35, "Injecting memory, please wait");
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
					sprintf(temp,"ux0:/data/savegames/%s/SLOT%hhu", titleid, saveslot);
					dumpSavedataDir("savedata0:",temp);
					slotstate[saveslot] = isDirectoryEmpty(temp);
					int_state = MENU;
					break;
					
				case IMPORT_SAVEDATA:
					
					blit_stringf(5, 35, "Importing savedata, please wait");
					sprintf(temp,"ux0:/data/savegames/%s/SLOT%hhu", titleid, saveslot);
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
					if (heap_scanner == 0) results_num += scanHeap(dval, search_type[search_id]);
					int_state = MENU;
					break;
					
				case FTP_COMMUNICATION:
					
					blit_stringf(5, 35, "Sending request to net module, please wait");
					ftp = !ftp;
					sendNetRequest(FTP_SWITCH);
					tmp = sceIoOpen("ux0:/data/rinCheat/ip.txt", SCE_O_RDONLY, 0777);
					size = sceIoRead(tmp, vita_ip, 32);
					vita_ip[size] = 0;
					sceIoClose(tmp);
					int_state = MENU;
					break;
					
				case STREAM_COMMUNICATION:
				
					blit_stringf(5, 35, "Sending request to net module, please wait");
					pc_stream = !pc_stream;
					sendNetRequest(STREAM_SWITCH);
					tmp = sceIoOpen("ux0:/data/rinCheat/ip.txt", SCE_O_RDONLY, 0777);
					size = sceIoRead(tmp, vita_ip, 32);
					vita_ip[size] = 0;
					sceIoClose(tmp);
					int_state = MENU;
					break;
					
				case CHANGE_STREAM_QUALITY:
					
					blit_stringf(5, 35, "Sending request to net module, please wait");
					sendNetRequest(SET_LOWEST_QUALITY + quality_idx);
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
				heap_scanner = checkHeap();
				net_thread = checkNetModule();
				if (!setup){
					setup = 1;
					
					// Setting saved stream video quality
					if (net_thread != 0){
						uint8_t req_id;
						switch (cfg.video_quality){
							case 255:
								quality_idx = 0;
								req_id = SET_LOWEST_QUALITY;
								break;
							case 200:
								quality_idx = 1;
								req_id = SET_LOW_QUALITY;
								break;
							case 128:
								quality_idx = 2;
								req_id = SET_NORMAL_QUALITY;
								break;
							case 64:
								quality_idx = 3;
								req_id = SET_HIGH_QUALITY;
								break;
							case 0:
								quality_idx = 4;
								req_id = SET_BEST_QUALITY;
								break;
						}
						sendNetRequest(req_id);
					}
					
					// Grabbing game resolution
					SceDisplayFrameBuf param;
					param.size = sizeof(SceDisplayFrameBuf);
					sceDisplayWaitVblankStart();
					sceDisplayGetFrameBuf(&param, SCE_DISPLAY_SETBUF_NEXTFRAME);
					pheight = param.height;
					pwidth = param.width;
					
					// Patch for games that use resolution different from native one (Like Minecraft)
					hmax = pheight - 84;
					
				}
			}else sceKernelDelayThread(1000); // Invoking scheduler to not slowdown games
		}
		oldpad = pad;
	}

	return 0;
}

int _start(SceSize args, void *argp) {
	SceUID thid = sceKernelCreateThread("rinCheat", main_thread, 0x40, 0x400000, 0, 0, NULL);
	if (thid >= 0)
		sceKernelStartThread(thid, 0, NULL);

	return 0;
}
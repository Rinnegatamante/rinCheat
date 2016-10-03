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
#include "memory.h"
#include <psp2/apputil.h>
#include <psp2/io/dirent.h>
#include <psp2/io/fcntl.h>
#include <psp2/io/stat.h>

// Dump/Restore decrypted savedata
void dumpSavedataDir(char* folder, char* target){
	
	// Check if game is listed to force MMC mode
	uint8_t force_mmc = 0;
	if (strstr(target, "PCSE00934") != NULL) force_mmc = 1;
	
	sceIoMkdir(target, 0777);
	SceIoDirent dirent;
	SceUID fd = sceIoDopen(folder);
	while (sceIoDread(fd, &dirent) > 0) {
		if SCE_S_ISDIR(dirent.d_stat.st_mode){
			char path[256], target_p[256];
			sprintf(path,"%s/%s",folder,dirent.d_name);
			sprintf(target_p,"%s/%s",target,dirent.d_name);
			dumpSavedataDir(path, target_p);
		}else{
			char path[256], target_p[256];
			sprintf(path,"%s/%s",folder,dirent.d_name);
			sprintf(target_p,"%s/%s",target,dirent.d_name);
			int fd = sceIoOpen(path, SCE_O_RDONLY, 0777);
			int size = sceIoLseek(fd, 0, SEEK_END);
			sceIoLseek(fd, 0, SEEK_SET);
			if (ram_mode && !force_mmc){
				char* data = (char*)malloc(size);
				sceIoRead(fd, data, size);
				sceIoClose(fd);
				fd = sceIoOpen(target_p, SCE_O_WRONLY | SCE_O_CREAT, 0777);
				sceIoWrite(fd, data, size);
				sceIoClose(fd);
				free(data);
			}else{
				int fd2 = sceIoOpen(target_p, SCE_O_WRONLY | SCE_O_CREAT, 0777);
				char data[CHUNK_SIZE];
				int chunk_size;
				while ((chunk_size=sceIoRead(fd,data,CHUNK_SIZE)) > 0){
					sceIoWrite(fd2, data, chunk_size);
				}
				sceIoClose(fd2);
				sceIoClose(fd);
			}
		}
	}
}

void restoreSavedataDir(char* folder, char* target){

	// Check if game is listed to force MMC mode
	uint8_t force_mmc = 0;
	if (strstr(folder, "PCSE00934") != NULL) force_mmc = 1;
	
	SceUID fd = sceIoDopen(folder);
	SceIoDirent dirent;
	while (sceIoDread(fd, &dirent) > 0) {
		if SCE_S_ISDIR(dirent.d_stat.st_mode){
			char path[256], target_p[256];
			sprintf(path,"%s/%s",folder,dirent.d_name);
			if (target != NULL) sprintf(target_p,"%s/%s",target,dirent.d_name);
			else sprintf(target_p,"%s",dirent.d_name);
			restoreSavedataDir(path, target_p);
		}else{
			char path[256], target_p[256];
			sprintf(path,"%s/%s",folder,dirent.d_name);
			if (target != NULL) sprintf(target_p,"%s/%s",target,dirent.d_name);
			else sprintf(target_p,"%s",dirent.d_name);
			int fd = sceIoOpen(path, SCE_O_RDONLY, 0777);
			int size = sceIoLseek(fd, 0, SEEK_END);
			sceIoLseek(fd, 0, SEEK_SET);
			
			// Initializing SceAppUtil if not yet booted
			SceAppUtilInitParam init_param;
			SceAppUtilBootParam boot_param;
			memset(&init_param, 0, sizeof(SceAppUtilInitParam));
			memset(&boot_param, 0, sizeof(SceAppUtilBootParam));
			sceAppUtilInit(&init_param, &boot_param);
			
			// Removing old file if it exists
			SceAppUtilSaveDataRemoveItem removeData;
			memset(&removeData, 0, sizeof(removeData));
			removeData.dataPath = target_p;
			sceAppUtilSaveDataDataRemove(NULL, &removeData, 1, NULL);
			
			// Getting save slot info
			SceAppUtilSaveDataFileSlot saveSlot;
			SceAppUtilSaveDataSlotParam slotParam;
			sceAppUtilSaveDataSlotGetParam(0, &slotParam, NULL);
			memset(&saveSlot, 0 ,sizeof(SceAppUtilSaveDataFileSlot));
			saveSlot.slotParam = &slotParam;
			
			// Injecting new file
			SceSize requiredSize=0;
			SceAppUtilSaveDataFile file;
			memset(&file, 0 ,sizeof(SceAppUtilSaveDataFile));
			file.filePath = target_p;
			if (ram_mode && !force_mmc){
				char* data = (char*)malloc(size);
				sceIoRead(fd, data, size);
				sceIoClose(fd);
				file.buf = data;
				file.bufSize = size;
				file.offset = 0;
				sceAppUtilSaveDataDataSave(&saveSlot, &file, 1, NULL, &requiredSize);
				free(data);
			}else{
				char data[0x80000];
				int chunk_size;
				uint32_t offset = 0;
				while ((chunk_size=sceIoRead(fd,data,0x80000)) > 0){
					file.buf = data;
					file.bufSize = chunk_size;
					file.offset = offset;
					sceAppUtilSaveDataDataSave(&saveSlot, &file, 1, NULL, &requiredSize);
					offset+=chunk_size;
				}
				sceIoClose(fd);
			}
			
		}
	}
	
}
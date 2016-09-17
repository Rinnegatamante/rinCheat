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
#include <psp2/io/fcntl.h>
#include <psp2/io/stat.h>
#include "cheats.h"

int numCheats = 0;

cheatDB* loadCheatsDatabase(char* db_file, cheatDB* db){
	char chunk[1025];
	cheatDB* cur = NULL;
	int fd = sceIoOpen(db_file, SCE_O_RDWR|SCE_O_CREAT, 0777);
	for (;;){
	
		// Read next chunk
		int chunk_size = 0;
		if ((chunk_size=sceIoRead(fd, chunk, 1024)) < 2) break;
		
		// Scan chunk
		char* last_ptr = NULL;
		char* cur_ptr = chunk;
		for (;;){
			char* name_init = strstr(cur_ptr, "#");
			if (name_init == NULL) break;
			char* name_end = strstr(name_init, "\n");
			if (name_end == NULL) break;
			char* addr_init = strstr(name_end,"@");
			if (addr_init == NULL) break;
			char* val_init = strstr(addr_init+1,"@");
			if (val_init == NULL) break;
			char* size_init = strstr(val_init+1,"@");
			if (size_init == NULL) break;
			if (size_init-chunk >= 1024) break;
			cur_ptr = last_ptr = size_init+1;
			if (db == NULL){
				db = malloc(sizeof(cheatDB));
				cur = db;
			}else{
				cur->next = malloc(sizeof(cheatDB));
				cur = cur->next;
			}
			strncpy(cur->name,name_init+1,name_end-(name_init+1));
			cur->name[name_end-(name_init+1)] = 0;
			sscanf(addr_init,"@0x%lX @0x%llX @%hhu",&cur->offset, &cur->val, &cur->size);
			cur->state = 0;
			numCheats++;
			cur->next = NULL;
		}
		uint32_t rewind_val = 0-(1024 - (last_ptr - chunk));
		sceIoLseek(fd, rewind_val, SEEK_CUR);
		
		// We reached EOF
		if (chunk_size < 1024) break;
		
	}
	sceIoClose(fd);
	return db;
}
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
 
#include "SDL/SDL.h"
#include "SDL/SDL_image.h"
#include "SDL/SDL_mixer.h"
#include "SDL/SDL_opengl.h"
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#ifdef __WIN32__
# include <winsock2.h>
#else
# include <sys/socket.h>
# include <netinet/in.h>
# include <netdb.h>
# include <arpa/inet.h>
#endif

#define STREAM_PORT 5000 // Port to use

typedef struct{
	uint32_t sock;
	struct sockaddr_in addrTo;
} Socket;

int width, height, size;
SDL_Surface* frame = NULL;
SDL_Surface* new_frame = NULL;
char* buffer;
GLint nofcolors = 3;
GLenum texture_format=GL_RGB;
GLuint texture=NULL;
void updateFrame(){

	// Loading frame
	SDL_RWops* rw = SDL_RWFromMem(buffer,size);
	new_frame = IMG_Load_RW(rw, 1);
	if (new_frame != NULL){
		SDL_FreeSurface(frame);
		frame = new_frame;
	}else printf("\nMalformed packet,frame will be skipped...");	
	if (frame == NULL) return;
	
	fflush(stdout);
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
	glTexImage2D( GL_TEXTURE_2D, 0, nofcolors, frame->w, frame->h, 0, texture_format, GL_UNSIGNED_BYTE, frame->pixels );
	
}

// Drawing function using openGL
void drawFrame(){
	if (texture == NULL) return;	
	glClear( GL_COLOR_BUFFER_BIT );
	glBindTexture( GL_TEXTURE_2D, texture );
	glBegin( GL_QUADS );
	glTexCoord2i( 0, 0 );
	glVertex3f( 0, 0, 0 );
	glTexCoord2i( 1, 0 );
	glVertex3f( width, 0, 0 );
	glTexCoord2i( 1, 1 );
	glVertex3f( width, height, 0 );
	glTexCoord2i( 0, 1 );
	glVertex3f( 0, height, 0 );
	glEnd();
	glLoadIdentity();
	SDL_GL_SwapBuffers();
}

int main(int argc, char* argv[]){

	#ifdef __WIN32__
	WORD versionWanted = MAKEWORD(1, 1);
	WSADATA wsaData;
	WSAStartup(versionWanted, &wsaData);
	#endif
	
	char host[32];
	if (argc > 1){
		char* ip = (char*)(argv[1]);
		sprintf(host,"%s",ip);
	}else{
		printf("Insert Vita IP: ");
		scanf("%s",host);
	}
	
	// Writing info on the screen
	printf("IP: %s\nPort: %d\n\n",host, STREAM_PORT);
	
	
	// Creating client socket
	Socket* my_socket = (Socket*) malloc(sizeof(Socket));
	memset(&my_socket->addrTo, '0', sizeof(my_socket->addrTo));
	my_socket->addrTo.sin_family = AF_INET;
	my_socket->addrTo.sin_port = htons(STREAM_PORT);
	my_socket->addrTo.sin_addr.s_addr = inet_addr(host);
	int addrLen = sizeof(my_socket->addrTo);
	my_socket->sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (my_socket->sock < 0){
		printf("\nFailed creating socket.");
		return -1;
	}else printf("\nClient socket created on port %d", STREAM_PORT);
	
	// Connecting to rinCheat
	int err = connect(my_socket->sock, (struct sockaddr*)&my_socket->addrTo, sizeof(my_socket->addrTo));
	if (err < 0 ){
		printf("\nFailed connecting server.");
		close(my_socket->sock);
		return -1;
	}else printf("\nConnection established!");
	fflush(stdout);
	u_long _true = 1;
	char sizes[32];
	send(my_socket->sock, "request", 8, 0);
	recv(my_socket->sock, sizes, 32, 0);
	sscanf(sizes, "%d;%d", &width, &height);
	printf("\nSetting window resolution to %d x %d", width, height);
	fflush(stdout);
	ioctlsocket(my_socket->sock, FIONBIO, &_true);
	int rcvbuf = 1*1024* 1024;
	setsockopt(my_socket->sock, SOL_SOCKET, SO_RCVBUF, (char*)&rcvbuf, sizeof(rcvbuf));
	
	// Initializing SDL and openGL stuffs
	uint8_t quit = 0;
	SDL_Event event;
	SDL_Surface* screen = NULL;
	SDL_Init( SDL_INIT_EVERYTHING );
	SDL_GL_SetAttribute( SDL_GL_DOUBLEBUFFER, 1 );
	screen = SDL_SetVideoMode( width, height, 32, SDL_OPENGL );
	glClearColor( 0, 0, 0, 0 );
	glEnable( GL_TEXTURE_2D );
	glViewport( 0, 0, width, height );
	glMatrixMode( GL_PROJECTION );
	glLoadIdentity();
	glOrtho( 0, width, height, 0, -1, 1 );
	glMatrixMode( GL_MODELVIEW );
	glLoadIdentity();
	SDL_WM_SetCaption("rinCheat Streamer", NULL);
	
	// Framebuffer & texture stuffs
	glGenTextures( 1, &texture );
	glBindTexture( GL_TEXTURE_2D, texture );
	buffer = (uint8_t*)malloc((width*height)<<2);
	
	for (;;){

		// Receiving a new frame
		int rbytes = 0;
		while (rbytes <= 0){
			rbytes = recv(my_socket->sock, &buffer[count], 65536, 0);
			while( SDL_PollEvent( &event ) ) {
				if( event.type == SDL_QUIT ) {
					quit = 1;
				} 
			}
			if (quit) break;
			size = rbytes;
		}
		if (quit) break;
		
		updateFrame();
		drawFrame();
		
	}
	
	free(buffer);
	return 0;
}
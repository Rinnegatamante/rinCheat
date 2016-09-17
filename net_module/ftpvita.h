/*
 * Copyright (c) 2015-2016 Sergi Granell (xerpi)
 */

#ifndef FTPVITA_H
#define FTPVITA_H

#include <psp2/types.h>
#include <sys/syslimits.h>
#include <psp2/net/net.h>
#include <psp2/net/netctl.h>

typedef void (*ftpvita_log_cb_t)(const char *);

/* Returns PSVita's IP and FTP port. 0 on success */
int ftpvita_init(char *vita_ip, unsigned short int *vita_port);
void ftpvita_fini();
int ftpvita_is_initialized();
int ftpvita_add_device(const char *devname);
int ftpvita_del_device(const char *devname);
void ftpvita_set_info_log_cb(ftpvita_log_cb_t cb);
void ftpvita_set_debug_log_cb(ftpvita_log_cb_t cb);
void ftpvita_set_file_buf_size(unsigned int size);

/* Extended functionality */

#define FTPVITA_EOL "\r\n"

typedef enum {
	FTP_DATA_CONNECTION_NONE,
	FTP_DATA_CONNECTION_ACTIVE,
	FTP_DATA_CONNECTION_PASSIVE,
} DataConnectionType;

typedef struct ftpvita_client_info {
	/* Client number */
	int num;
	/* Thread UID */
	SceUID thid;
	/* Control connection socket FD */
	int ctrl_sockfd;
	/* Data connection attributes */
	int data_sockfd;
	DataConnectionType data_con_type;
	SceNetSockaddrIn data_sockaddr;
	/* PASV mode client socket */
	SceNetSockaddrIn pasv_sockaddr;
	int pasv_sockfd;
	/* Remote client net info */
	SceNetSockaddrIn addr;
	/* Receive buffer attributes */
	int n_recv;
	char recv_buffer[512];
	/* Current working directory */
	char cur_path[PATH_MAX];
	/* Rename path */
	char rename_path[PATH_MAX];
	/* Client list */
	struct ftpvita_client_info *next;
	struct ftpvita_client_info *prev;
	/* Offset for transfer resume */
	unsigned int restore_point;
} ftpvita_client_info_t;


typedef void (*cmd_dispatch_func)(ftpvita_client_info_t *client); // Command handler

int ftpvita_ext_add_custom_command(const char *cmd, cmd_dispatch_func func);
int ftpvita_ext_del_custom_command(const char *cmd);
void ftpvita_ext_client_send_ctrl_msg(ftpvita_client_info_t *client, const char *msg);
void ftpvita_ext_client_send_data_msg(ftpvita_client_info_t *client, const char *str);

#endif

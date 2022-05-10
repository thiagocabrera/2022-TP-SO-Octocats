#ifndef KERNEL_UTILS_H_
#define KERNEL_UTILS_H_

#include<stdio.h>
#include<stdlib.h>
#include<pthread.h>
#include<commons/log.h>
#include<commons/string.h>
#include<commons/config.h>
#include<commons/collections/queue.h>
#include<readline/readline.h>

#include "../../shared/src/sockets.h"
#include "../../shared/src/protocol.h"
#include "../../shared/src/structures.h"

typedef struct{
	t_log* log;
	int cliente;
	char* server_name;
	t_queue* colaNew;
} procesar_consola_args;

typedef struct{
	t_log* log;
	t_queue* colaNew;
} pasaje_new_ready_args;

void procesar_consola(procesar_consola_args* argumentos);
int escuchar_servidor(t_log* logger, char* name, int server_socket, t_queue* colaNew);

#endif /* KERNEL_UTILS_H_ */

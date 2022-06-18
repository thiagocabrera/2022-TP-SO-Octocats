#ifndef MEMORIA_UTILS_H_
#define MEMORIA_UTILS_H_

#include<stdio.h>
#include<stdlib.h>
//#include<stdbool.h>
//#include<pthread.h>
#include<commons/log.h>
#include<commons/config.h>
//#include<commons/string.h>
//#include<readline/readline.h>
//#include<semaphore.h>
//#include<commons/collections/queue.h>
#include<commons/collections/list.h>
//#include<commons/collections/dictionary.h>
#include <sockets.h>
#include <protocol.h>
//#include <structures.h>
//#include <pcb.h>
#include <commons/bitarray.h>

typedef uint32_t fila_1er_nivel;

typedef struct {
	uint32_t nro_marco;
	unsigned char modificado;
	unsigned char presencia;
} fila_2do_nivel;

t_log* logger;
t_config* config;
int cliente_cpu, cliente_kernel;

int server_memoria;

void apagar_memoria();
void inicializar_memoria();
uint32_t crear_tablas();
void asignar_marcos(t_list* tabla_2do_nivel);
int buscar_marco_libre();
int marcos_actuales(int entrada_1er_nivel, int entrada_2do_nivel);

#endif /* MEMORIA_UTILS_H_ */

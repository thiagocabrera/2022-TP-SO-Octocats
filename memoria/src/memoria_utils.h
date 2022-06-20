#ifndef MEMORIA_UTILS_H_
#define MEMORIA_UTILS_H_

#include<stdio.h>
#include<stdlib.h>
//#include<stdbool.h>
#include<pthread.h>
#include "memoria_swap.h"
#include<commons/log.h>
#include<commons/config.h>
//#include<commons/string.h>
//#include<readline/readline.h>
#include<semaphore.h>
//#include<commons/collections/queue.h>
#include<commons/collections/list.h>
//#include<commons/collections/dictionary.h>
#include <sockets.h>
#include <protocol.h>
//#include <structures.h>
#include <commons/bitarray.h>

typedef int32_t fila_1er_nivel;

typedef struct {
	int32_t nro_marco;
	unsigned char modificado;
	unsigned char presencia;
	unsigned char uso;
} fila_2do_nivel;

typedef struct {
	uint32_t entrada_1er_nivel;
	uint32_t entrada_2do_nivel;
} puntero_clock;

int cliente_cpu, cliente_kernel;

int server_memoria;

void apagar_memoria();
void escuchar_kernel();
void recibir_kernel();
void escuchar_cpu();
void recibir_cpu();
void inicializar_memoria();
uint32_t crear_tablas(uint16_t pid, uint32_t tamanio);
void eliminar_estructuras(uint32_t tabla_paginas, uint16_t pid);
void escribir_marco(uint32_t nro_marco, void* marco);
int buscar_marco_libre();
int marcos_actuales(int entrada_1er_nivel, int entrada_2do_nivel);
void inicializar_tabla_1er_nivel(fila_1er_nivel* tabla_1er_nivel);
void inicializar_tabla_2do_nivel(fila_2do_nivel* tabla_2do_nivel);
fila_1er_nivel obtener_entrada_1er_nivel(int32_t nro_tabla, uint32_t index, uint32_t pid);
uint32_t obtener_entrada_2do_nivel(uint32_t nro_tabla, uint32_t index);
uint8_t obtener_entrada_marco(uint32_t nro_marco, uint32_t desplazamiento);
void* obtener_marco(uint32_t nro_marco);
uint32_t clock_simple(FILE* swap);
uint32_t clock_modificado();
uint32_t marcos_en_memoria();

#endif /* MEMORIA_UTILS_H_ */

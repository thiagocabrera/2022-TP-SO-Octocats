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
	uint16_t pid;
	t_list* marcos_en_memoria;
	uint16_t puntero;
} estructura_clock;

typedef struct {
	int32_t nro_marco_en_memoria;
	fila_2do_nivel* pagina;
	uint32_t nro_marco_en_swap;
} fila_estructura_clock;

int cliente_cpu, cliente_kernel;

int server_memoria;

void inicializar_memoria();
void apagar_memoria();

// HILO KERNEL
void escuchar_kernel();
void recibir_kernel();

fila_2do_nivel crear_fila(int marco, int mod, int pres);
uint32_t crear_tablas(uint16_t pid, uint32_t tamanio);
void inicializar_tabla_1er_nivel(fila_1er_nivel* tabla_1er_nivel);
void inicializar_tabla_2do_nivel(fila_2do_nivel* tabla_2do_nivel);
void crear_estructura_clock(uint16_t pid);

void suspender_proceso(uint16_t pid, uint32_t tabla_paginas);

void eliminar_estructuras(uint32_t tabla_paginas, uint16_t pid);

// HILO CPU
void escuchar_cpu();
void recibir_cpu();
int buscar_marco_libre();
uint32_t usar_algoritmo(FILE* swap);
uint32_t calcular_cant_marcos(uint32_t tamanio);
int marcos_actuales(int entrada_1er_nivel, int entrada_2do_nivel);
void printear_bitmap();
fila_1er_nivel obtener_nro_tabla_2do_nivel(int32_t nro_tabla, uint32_t index, uint32_t pid);
uint32_t obtener_nro_marco_memoria(uint32_t nro_tabla, uint32_t index);
void reemplazo_por_clock(uint32_t nro_marco_en_swap, fila_2do_nivel* entrada_2do_nivel, FILE* swap)
void agregar_pagina_a_estructura_clock(int32_t nro_marco, fila_2do_nivel* pagina);
estructura_clock* buscar_estructura_clock();
uint16_t avanzar_puntero(uint16_t puntero_clock);
uint32_t clock_simple(FILE* swap);
uint32_t clock_modificado(FILE* swap);
void* obtener_marco(uint32_t nro_marco);
void eliminar_marco_estructura_clock(uint32_t nro_marco_en_swap);
uint32_t marcos_en_memoria();
void escribir_marco_en_memoria(uint32_t nro_marco, void* marco);

uint32_t read_en_memoria(uint32_t nro_marco, uint16_t desplazamiento);
void write_en_memoria(uint32_t nro_marco, uint16_t desplazamiento, uint32_t dato);

#endif /* MEMORIA_UTILS_H_ */

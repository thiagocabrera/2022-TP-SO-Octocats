#include "memoria_utils.h"

t_list* lista_tablas_1er_nivel;
t_list* lista_tablas_2do_nivel;
uint16_t tam_memoria;
uint16_t tam_pagina;
uint16_t entradas_por_tabla;
uint16_t retardo_memoria;
uint16_t marcos_por_proceso;
uint16_t retardo_swap;
uint8_t* marcos_ocupados;
void* memoria;

void inicializar_memoria(){
	logger = log_create("memoria.log", "memoria", 1, LOG_LEVEL_INFO);
	config = config_create("memoria.config");
//	char* ip = config_get_string_value(config,"IP");
//	char* puerto_escucha = config_get_string_value(config,"PUERTO_ESCUCHA");
//	server_memoria = iniciar_servidor(logger, "MEMORIA", ip, puerto_escucha);

	tam_memoria = config_get_int_value(config,"TAM_MEMORIA");
	tam_pagina = config_get_int_value(config,"TAM_PAGINA");
	entradas_por_tabla = config_get_int_value(config,"ENTRADAS_POR_TABLA");
	retardo_memoria = config_get_double_value(config,"RETARDO_MEMORIA");
	marcos_por_proceso = config_get_int_value(config,"MARCOS_POR_PROCESO");
	retardo_swap = config_get_int_value(config,"RETARDO_SWAP");

	memoria = malloc(tam_memoria);
	lista_tablas_1er_nivel = list_create();
	lista_tablas_2do_nivel = list_create();
	marcos_ocupados = malloc(tam_memoria / tam_pagina);
	for (int i = 0; i < tam_memoria / tam_pagina; i++)
		marcos_ocupados[i] = 0;

}

// buscar el primer marco libre
int buscar_marco_libre(){
	for (int i = 0; i < tam_memoria / tam_pagina; i++){
		if (marcos_ocupados[i] == 0)
			return i;
	}
	return -1;
}

// devuelve la cantidad de marcos que requiere un proceso del tamanio especificado
uint32_t calcular_cant_marcos(uint32_t tamanio){
	int  cant_marcos = tamanio / tam_pagina;
	if (tamanio % tam_pagina != 0)
		cant_marcos++;
	return cant_marcos;
}

// devuelve el nro de marcos que se crearon hasta el momento
int marcos_actuales(int entrada_1er_nivel, int entrada_2do_nivel){
	return entrada_1er_nivel * entradas_por_tabla + entrada_2do_nivel;
}

void printear_bitmap(){
	for (int i = 0; i < tam_memoria / tam_pagina; i++){
		printf("%d : %d\n", i, marcos_ocupados[i]);
	}
}

fila_2do_nivel crear_fila(int marco, int mod, int pres){
	fila_2do_nivel f = {marco, mod, pres};
	return f;
}

// creas los marcos y las tablas necesarias para el proceso
uint32_t crear_tablas(uint32_t tamanio){

	int cont = 0;
	int marcos_req = calcular_cant_marcos(tamanio);
	printf("CANT DE MARCOS REQUERIDOS: %d\n", marcos_req);
	fila_1er_nivel tabla_1er_nivel [entradas_por_tabla];
	for (int i = 0; i < entradas_por_tabla && marcos_req > marcos_actuales(i, 0); i++){
		fila_2do_nivel* tabla_2do_nivel = malloc(entradas_por_tabla * sizeof(fila_2do_nivel));
		for (int j = 0 ; j < entradas_por_tabla && marcos_req > marcos_actuales(i, j); j++){
			fila_2do_nivel fila = {0,0,0};
			// chequea si el proceso ya ocupo la cantidad maxima (parametro marcos x proceso)
			if (cont  < marcos_por_proceso){
				int marco_libre = buscar_marco_libre();
				// si no hay marcos libres, hay que swappear el marco
				if (marco_libre == -1){
					printf("Swappeame loco :(\n");
					// que marco le mandas, si lo estas mandando a swapp??
					fila = crear_fila(marco_libre,0,1);
				}
				else{
					cont++;
					fila = crear_fila(marco_libre,0,0);
					marcos_ocupados[marco_libre] = 1;
				}
			}
			else{
				printf("Swapeame los marcos wachin\n");
				// que marco le mandas, si lo estas mandando a swapp??
				fila = crear_fila(0,0,0);
			}
			tabla_2do_nivel[j] = fila;
		}
		uint32_t nro_tabla_2do_nivel = list_add(lista_tablas_2do_nivel,tabla_2do_nivel);
		tabla_1er_nivel[i] = nro_tabla_2do_nivel;
	}
	return list_add(lista_tablas_1er_nivel,tabla_1er_nivel);
}

// dada un nro de tabla y un index devuelve la entrada correspondiente
// quizas convenga devolver directamente el valor y no su puntero
fila_1er_nivel obtener_entrada_1er_nivel(uint32_t nro_tabla, uint32_t index){
	fila_1er_nivel* tabla = list_get(lista_tablas_1er_nivel,nro_tabla);
	return tabla[index];
}

// dada un nro de tabla y un index devuelve la entrada correspondiente
// quizas convenga devolver directamente el valor y no su puntero
fila_2do_nivel obtener_entrada_2do_nivel(uint32_t nro_tabla, uint32_t index){
	fila_2do_nivel* tabla = list_get(lista_tablas_2do_nivel,nro_tabla);
	return tabla[index];
}

// dado un nro de marco y un desplazamiento, devuelve el dato en concreto
// la posicion que nos da el puntero a la memoria + tam_pagina * nro_marco nos da -->
// --> la posicion de inicio del marco especificado, luego eso + desplazamiento -->
// --> nos da la direccion que buscamos
// (chequear implementacion)
// quizas convenga devolver directamente el valor y no su puntero
uint32_t obtener_entrada_marco(uint32_t nro_marco, uint32_t desplazamiento){
	return (uint32_t) memoria + tam_pagina * nro_marco + desplazamiento;
}

void asignar_marcos(t_list* tabla_2do_nivel){

}

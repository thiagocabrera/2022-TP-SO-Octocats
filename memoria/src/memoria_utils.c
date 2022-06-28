#include "memoria_utils.h"


pthread_mutex_t mx_log = PTHREAD_MUTEX_INITIALIZER;

int cliente_kernel;
uint16_t pid_actual;
uint16_t entrada_1er_nivel_actual;
fila_2do_nivel* entrada_2do_nivel_actual;
uint32_t index_tabla_1er_nivel;
t_list* lista_tablas_1er_nivel;
t_list* lista_tablas_2do_nivel;
t_list* lista_estructuras_clock;
uint16_t tam_memoria;
uint16_t tam_pagina;
uint16_t entradas_por_tabla;
uint16_t retardo_memoria;
uint16_t marcos_por_proceso;
uint8_t* marcos_ocupados;
char* algoritmo;
void* memoria;

void imprimir_tablas_1(){
	printf("TABLAS 1ER NIVEL:\n");
	for (int i = 0; i < list_size(lista_tablas_1er_nivel); i++){
		printf("TABLA %d:\n", i);
		fila_1er_nivel* tabla = list_get(lista_tablas_1er_nivel, i);
		for (int j = 0; j < entradas_por_tabla; j ++){
			printf("%d - %d\n", j, tabla[j]);
		}
	}
}

void imprimir_tablas_2(){
	printf("TABLAS 2DO NIVEL:\n");
	for (int i = 0; i < list_size(lista_tablas_2do_nivel); i++){
		printf("TABLA %d:\n", i);
		fila_2do_nivel* tabla = list_get(lista_tablas_2do_nivel, i);
		for (int j = 0; j < entradas_por_tabla; j ++){
			printf("%d - marco %d uso %d presencia %d modificado %d\n", j, tabla[j].nro_marco,tabla[j].uso, tabla[j].presencia, tabla[j].modificado);
		}
	}
}

void inicializar_memoria(){
	logger = log_create("memoria.log", "memoria", 1, LOG_LEVEL_INFO);
	config = config_create("memoria.config");
	char* ip = config_get_string_value(config,"IP");
	char* puerto_escucha = config_get_string_value(config,"PUERTO_ESCUCHA");
	server_memoria = iniciar_servidor(logger, "MEMORIA", ip, puerto_escucha);
//	cliente_cpu = esperar_cliente(logger, "MEMORIA", server_memoria);
//	cliente_kernel = esperar_cliente(logger, "MEMORIA", server_memoria);

	tam_memoria = config_get_int_value(config,"TAM_MEMORIA");
	tam_pagina = config_get_int_value(config,"TAM_PAGINA");
	entradas_por_tabla = config_get_int_value(config,"ENTRADAS_POR_TABLA");

	retardo_memoria = config_get_double_value(config,"RETARDO_MEMORIA");

	marcos_por_proceso = config_get_int_value(config,"MARCOS_POR_PROCESO");
	algoritmo = config_get_string_value(config,"ALGORITMO_REEMPLAZO");

	memoria = malloc(tam_memoria);
	lista_tablas_1er_nivel = list_create();
	lista_tablas_2do_nivel = list_create();
	lista_estructuras_clock = list_create();
	marcos_ocupados = malloc(tam_memoria / tam_pagina);
	for (int i = 0; i < tam_memoria / tam_pagina; i++)
		marcos_ocupados[i] = 0;

	inicializar_swap();

}

/***************************** HILO KERNEL *****************************/

void escuchar_kernel() {
	while(1) {
		recibir_kernel();
	}
}

void recibir_kernel() {
	op_code cop;
	while (cliente_kernel != -1) {
		if (recv(cliente_kernel, &cop, sizeof(op_code), 0) <= 0) {
			pthread_mutex_lock(&mx_log);
			log_error(logger,"DISCONNECT FAILURE!");
			pthread_mutex_unlock(&mx_log);
			return;
		}
		switch (cop) {
			case CREAR_TABLA:
			{
				uint32_t tamanio = 0;
				uint16_t pid = 0;

				if (!recv_crear_tabla(cliente_kernel, &tamanio, &pid)) {
					pthread_mutex_lock(&mx_log);
					log_error(logger,"Fallo recibiendo ELIMINAR ESTRUCTURAS");
					pthread_mutex_unlock(&mx_log);
					break;
				}

				uint32_t tabla_paginas = crear_tablas(pid, tamanio);
				// usleep(retardo_memoria * 1000); //este usleep no va xq es para kernel, no cpu:)
				send(cliente_kernel, &tabla_paginas, sizeof(uint32_t), 0);

				return;
			}
			case ELIMINAR_ESTRUCTURAS:
			{
				uint32_t tabla_paginas = 0;
				uint16_t pid = 0;

				if (!recv_eliminar_estructuras(cliente_kernel, &pid, &tabla_paginas)) {
					pthread_mutex_lock(&mx_log);
					log_error(logger,"Fallo recibiendo ELIMINAR ESTRUCTURAS");
					pthread_mutex_unlock(&mx_log);
					break;
				}

				eliminar_estructuras(tabla_paginas, pid);

				return;
			}
			case SUSPENDER_PROCESO:
			{
				uint32_t tabla_paginas = 0;
				uint16_t pid = 0;

				if (!recv_suspender_proceso(cliente_kernel, &pid, &tabla_paginas)) {
					pthread_mutex_lock(&mx_log);
					log_error(logger,"Fallo recibiendo ELIMINAR ESTRUCTURAS");
					pthread_mutex_unlock(&mx_log);
					break;
				}

				suspender_proceso(pid, tabla_paginas);

				uint16_t resultado = 1;
				send(cliente_kernel, &resultado, sizeof(uint16_t), 0);


				return;
			}
			default:
				pthread_mutex_lock(&mx_log);
				log_error(logger, "Algo anduvo mal en el server de memoria\n Cop: %d",cop);
				pthread_mutex_unlock(&mx_log);
		}
	}
}

// INICIALIZACIÓN DE PROCESO
fila_2do_nivel crear_fila(int marco, int mod, int pres){
	fila_2do_nivel f = {marco, mod, pres};
	return f;
}

// creas los marcos y las tablas necesarias para el proceso
uint32_t crear_tablas(uint16_t pid, uint32_t tamanio){

	int marcos_req = calcular_cant_marcos(tamanio);
	fila_1er_nivel* tabla_1er_nivel = malloc(sizeof(uint32_t) * entradas_por_tabla);
	FILE* swap = crear_archivo_swap(pid);
	inicializar_tabla_1er_nivel(tabla_1er_nivel);

	for (int i = 0; i < entradas_por_tabla && marcos_req > marcos_actuales(i, 0); i++){
		fila_2do_nivel* tabla_2do_nivel = malloc(entradas_por_tabla * sizeof(fila_2do_nivel));
		inicializar_tabla_2do_nivel(tabla_2do_nivel);
		for (int j = 0 ; j < entradas_por_tabla && marcos_req > marcos_actuales(i, j); j++){
			uint32_t nro_marco = agregar_marco_en_swap(swap, tam_pagina);
			fila_2do_nivel fila = {nro_marco,0,0,0};
			tabla_2do_nivel[j] = fila;
		}
		uint32_t nro_tabla_2do_nivel = list_add(lista_tablas_2do_nivel,tabla_2do_nivel);
		tabla_1er_nivel[i] = nro_tabla_2do_nivel;
	}
	cerrar_archivo_swap(swap);
	crear_estructura_clock(pid);
	return list_add(lista_tablas_1er_nivel,tabla_1er_nivel);
}

void inicializar_tabla_1er_nivel(fila_1er_nivel* tabla_1er_nivel){
	for (int i = 0; i < entradas_por_tabla; i++){
		tabla_1er_nivel[i] = -1;
	}
}

void inicializar_tabla_2do_nivel(fila_2do_nivel* tabla_2do_nivel){
	for (int i = 0; i < entradas_por_tabla; i++){
		tabla_2do_nivel[i].nro_marco = -1;
	}
}

// SUSPENSIÓN DE PROCESO

void suspender_proceso(uint16_t pid, uint32_t tabla_paginas) {
	FILE* swap = abrir_archivo_swap(pid);
	fila_1er_nivel* tabla_1er_nivel = list_get(lista_tablas_1er_nivel, tabla_paginas);
	for(int i=0; i < entradas_por_tabla; i++) {
		fila_2do_nivel* tabla_2do_nivel = list_get(lista_tablas_2do_nivel,tabla_1er_nivel[i]);
		for(int j=0; j < entradas_por_tabla; j++) {
			if (tabla_2do_nivel->nro_marco == -1) continue;
			if (tabla_2do_nivel->presencia == 1) {
				marcos_ocupados[tabla_2do_nivel->nro_marco] = 0;
				tabla_2do_nivel->presencia = 0;
				if (tabla_2do_nivel->modificado == 1) {
					int nro_marco_en_swap = i * entradas_por_tabla + j;
					actualizar_marco_en_swap(swap, nro_marco_en_swap, obtener_marco(tabla_2do_nivel->nro_marco), tam_pagina);
					tabla_2do_nivel->modificado = 0;
				}
			}
		}
	}
	cerrar_archivo_swap(swap);
}

// FINALIZACIÓN DE PROCESO

void eliminar_estructuras(uint32_t tabla_paginas, uint16_t pid) {
	// Acá se tendrían que eliminar el archivo swap correspondiente...
	// sería --> por cada marco --> bit de presencia --> pres == 1 --> bitarray a 0
	// yyyyyyyyy eliminamos el .swap
	fila_1er_nivel* tabla_1er_nivel = list_get(lista_tablas_1er_nivel, tabla_paginas);
	for(int i=0; i < entradas_por_tabla; i++) {
		fila_1er_nivel nro_tabla_2do_nivel = tabla_1er_nivel[i];
		if (nro_tabla_2do_nivel == -1) break; // si no hay mas salite
		fila_2do_nivel* tabla_2do_nivel = list_get(lista_tablas_2do_nivel,nro_tabla_2do_nivel);
		for(int j=0; j < entradas_por_tabla; j++){
			if (tabla_2do_nivel->nro_marco == -1) break; // si no hay mas salite
			if (tabla_2do_nivel->presencia == 1)
				marcos_ocupados[tabla_2do_nivel->nro_marco] = 0;
		}
	}
	borrar_archivo_swap(pid);
}

/***************************** HILO CPU *****************************/

void escuchar_cpu() {
	while(1) {
		recibir_cpu();
	}
}

void recibir_cpu() {
	op_code cop;
	while (cliente_cpu != -1) {
		if (recv(cliente_cpu, &cop, sizeof(op_code), 0) <= 0) {
			pthread_mutex_lock(&mx_log);
			log_error(logger,"DISCONNECT FAILURE!");
			pthread_mutex_unlock(&mx_log);
			return;
		}
		switch (cop) {
			case SOLICITUD_NRO_TABLA_2DO_NIVEL:
			{
				uint16_t pid = 0;
				uint32_t nro_tabla_1er_nivel = 0;
				uint32_t entrada_tabla = 0;

				if (!recv_solicitud_nro_tabla_2do_nivel(cliente_cpu, &pid, &nro_tabla_1er_nivel, &entrada_tabla)) {
					pthread_mutex_lock(&mx_log);
					log_error(logger,"Fallo recibiendo SOLICITUD NRO TABLA 2DO NIVEL");
					pthread_mutex_unlock(&mx_log);
					break;
				}

				fila_1er_nivel nro_tabla_2do_nivel = obtener_nro_tabla_2do_nivel(nro_tabla_1er_nivel, entrada_tabla, pid);

				usleep(retardo_memoria * 1000);
				send(cliente_cpu, &nro_tabla_2do_nivel, sizeof(fila_1er_nivel), 0);

				return;
			}
			case SOLICITUD_NRO_MARCO:
			{
				uint16_t pid = 0; // No se usa por ahora
				uint32_t nro_tabla_2do_nivel = 0;
				uint32_t entrada_tabla = 0;

				if (!recv_solicitud_nro_marco(cliente_cpu, &pid, &nro_tabla_2do_nivel, &entrada_tabla)) {
					pthread_mutex_lock(&mx_log);
					log_error(logger,"Fallo recibiendo SOLICITUD NRO TABLA 2DO NIVEL");
					pthread_mutex_unlock(&mx_log);
					break;
				}

				uint32_t nro_marco = obtener_nro_marco_memoria(nro_tabla_2do_nivel, entrada_tabla);

				usleep(retardo_memoria * 1000);
				send(cliente_cpu, &nro_marco, sizeof(uint32_t), 0);

				return;
			}
			case READ:
			{
				uint32_t nro_marco;
				uint16_t desplazamiento;

				if (!recv_read(cliente_cpu, &nro_marco, &desplazamiento)) {
					pthread_mutex_lock(&mx_log);
					log_error(logger,"Fallo recibiendo SOLICITUD NRO TABLA 2DO NIVEL");
					pthread_mutex_unlock(&mx_log);
					break;
				}

				uint32_t dato = read_en_memoria(nro_marco, desplazamiento);

				usleep(retardo_memoria * 1000);
				send(cliente_cpu, &dato, sizeof(uint32_t), 0);

				return;
			}
			case WRITE:
			{
				uint32_t nro_marco;
				uint16_t desplazamiento;
				uint32_t dato;

				if (!recv_write(cliente_cpu, &nro_marco, &desplazamiento, &dato)) {
					pthread_mutex_lock(&mx_log);
					log_error(logger,"Fallo recibiendo SOLICITUD NRO TABLA 2DO NIVEL");
					pthread_mutex_unlock(&mx_log);
					break;
				}

				write_en_memoria(nro_marco, desplazamiento, dato);

				op_code resultado = ESCRITURA_OK;
				usleep(retardo_memoria * 1000);
				send(cliente_cpu, &resultado, sizeof(op_code), 0);

				return;
			}
			default:
				pthread_mutex_lock(&mx_log);
				log_error(logger, "Algo anduvo mal en el server de memoria\n Cop: %d",cop);
				pthread_mutex_unlock(&mx_log);
		}
	}
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

// dada un nro de tabla y un index devuelve el nro de tabla de 2do nivel
fila_1er_nivel obtener_nro_tabla_2do_nivel(int32_t nro_tabla, uint32_t index, uint32_t pid){
	if (pid != pid_actual || nro_tabla != entrada_1er_nivel_actual){
		pid_actual = pid;
		entrada_1er_nivel_actual = nro_tabla;
	}
	fila_1er_nivel* tabla = list_get(lista_tablas_1er_nivel,nro_tabla);
	index_tabla_1er_nivel = index;
	return tabla[index];
}

// dada un nro de tabla y un index devuelve el nro de marco correspondiente
// si el bit de presencia esta en 0, traerlo a memoria --> algoritmos cock / cock modificado (cock = gallo)
uint32_t obtener_nro_marco_memoria(uint32_t nro_tabla, uint32_t index){
	fila_2do_nivel* tabla_2do_nivel = list_get(lista_tablas_2do_nivel, nro_tabla);
	fila_2do_nivel* pagina = &tabla_2do_nivel[index];
	if (pagina->presencia == 1){
		pagina->uso = 1;
		entrada_2do_nivel_actual = pagina;
		return pagina->nro_marco;
	}
	// si no esta en memoria, traerlo
	FILE* swap = abrir_archivo_swap(pid_actual);
	void* marco = leer_marco_en_swap(swap,pagina->nro_marco, tam_pagina);
	int32_t nro_marco;
	// si ya ocupa todos los marcos que puede ocupar, hacer un reemplazo
	if (marcos_en_memoria() == marcos_por_proceso){
		printf("Reemplazo\n");
		nro_marco = clock_simple(swap);
	}
	else{
		nro_marco = buscar_marco_libre();
		if (nro_marco == -1){
			printf("ERROR!!!!! NO HAY MARCOS LIBRES EN MEMORIA!!!\n");
			exit(EXIT_FAILURE);
		}
	}
	cerrar_archivo_swap(swap);
	marcos_ocupados[nro_marco] = 1;
	pagina->nro_marco = nro_marco;
	pagina->presencia = 1;
	pagina->uso = 1;
	uint32_t nro_marco_en_swap = index_tabla_1er_nivel * entradas_por_tabla + index;
	escribir_marco_en_memoria(pagina->nro_marco, marco);
	agregar_pagina_a_estructura_clock(nro_marco, pagina, nro_marco_en_swap);
	entrada_2do_nivel_actual = pagina;
	return nro_marco;
}

// vemos que algoritmo usar y lo usamos
uint32_t usar_algoritmo(FILE* swap){
	if (strcmp(algoritmo, "CLOCK-M") == 0)
		return clock_modificado(swap);
	else if (strcmp(algoritmo, "CLOCK") == 0)
		return clock_simple(swap);
	else{
		exit(-1);
	}
}

// CADA VEZ QUE LLEGA UNA PETICION, NOS TIENE QUE LLEGAR EL PID
// SI EL PID ES DIFERENTE AL PID ACTUAL, EL PUNTERO LO PONEMOS EN NULL
// Y SI AL RECIBIR UNA PETICION A LA 1ER LISTA DE TABLAS ES != A LA ACTUAL, PONEMOS EN NULL
uint32_t clock_simple(FILE* swap){
	// hasta que no encuentre uno no para
	estructura_clock* estructura = buscar_estructura_clock();
	uint16_t puntero_clock = estructura->puntero;
	fila_estructura_clock* fila;
	fila_2do_nivel* pagina;
	uint32_t nro_marco_en_swap;
	int32_t nro_marco;
	while(1){
		fila = list_get(estructura->marcos_en_memoria, puntero_clock);
		pagina = fila->pagina;
		nro_marco_en_swap = fila->nro_marco_en_swap;
		nro_marco = fila->nro_marco_en_memoria;
		puntero_clock = avanzar_puntero(puntero_clock);

		if (pagina->uso == 0){
			reemplazo_por_clock(nro_marco_en_swap, pagina, swap);
			return nro_marco;
		}
		else{
			pagina->uso = 0;
		}
	}

	return 0;
}

// iterar sobre la lista de tablas de 2do nivel
//uint32_t clock_modificado(FILE* swap){
//
//	// iteramos en las tablas de 2do nivel
//	fila_1er_nivel* tabla_1er_nivel = list_get(lista_tablas_1er_nivel, entrada_1er_nivel_actual);
//	puntero_clock puntero_inicial = puntero;
//	// hasta que no encuentre uno no para
//	while(1){
//		// 1er paso del algoritmo
//		while(1){
//
//			fila_2do_nivel* tabla_2do_nivel = list_get(lista_tablas_2do_nivel,tabla_1er_nivel[puntero.entrada_1er_nivel]);
//			fila_2do_nivel* entrada_2do_nivel = &tabla_2do_nivel[puntero.entrada_2do_nivel];
//
//			int nro_marco_en_swap = puntero.entrada_1er_nivel * entradas_por_tabla + puntero.entrada_2do_nivel;
//
//			// marmo va a chequear esto
//			avanzar_puntero(entrada_2do_nivel);
//
//			// si ya no esta en memoria para que me gasto??? La re concha de tu madre
//			if (entrada_2do_nivel->presencia == 0) continue;
//
//			if (entrada_2do_nivel->uso == 0 && entrada_2do_nivel->modificado == 0){
//				return reemplazo_por_clock(nro_marco_en_swap, entrada_2do_nivel, swap);
//			}
//
//			if (punteros_son_iguales(puntero, puntero_inicial)){
//				break;
//			}
//		}
//		// 2do paso del algoritmo
//		while(1){
//
//			fila_2do_nivel* tabla_2do_nivel = list_get(lista_tablas_2do_nivel,tabla_1er_nivel[puntero.entrada_1er_nivel]);
//			fila_2do_nivel* entrada_2do_nivel = &tabla_2do_nivel[puntero.entrada_2do_nivel];
//
//			int nro_marco_en_swap = puntero.entrada_1er_nivel * entradas_por_tabla + puntero.entrada_2do_nivel;
//
//			// marmo va a chequear esto
//			avanzar_puntero(entrada_2do_nivel);
//
//			// si ya no esta en memoria para que me gasto??? La re concha de tu madre
//			if (entrada_2do_nivel->presencia == 0) continue;
//
//			if (entrada_2do_nivel->uso == 0 && entrada_2do_nivel->modificado == 1){
//				return reemplazo_por_clock(nro_marco_en_swap, entrada_2do_nivel, swap);
//			}
//			else{
//				entrada_2do_nivel->uso = 0;
//			}
//			if (punteros_son_iguales(puntero, puntero_inicial)){
//				break;
//			}
//		}
//		// 3er paso del algoritmo, volver a empezar
//	}
//
//	return 0;
//}


void reemplazo_por_clock(uint32_t nro_marco_en_swap, fila_2do_nivel* entrada_2do_nivel, FILE* swap){
	// logica de swappeo de marco
	printf("Elegido %d!\n",nro_marco_en_swap);
	// si tiene el bit de modificado en 1, hay que actualizar el archivo swap
	if (entrada_2do_nivel->modificado == 1){
		actualizar_marco_en_swap(swap, nro_marco_en_swap, obtener_marco(entrada_2do_nivel->nro_marco), tam_pagina);
		entrada_2do_nivel->modificado = 0;
	}
	entrada_2do_nivel->presencia = 0;
	marcos_ocupados[entrada_2do_nivel->nro_marco] = 0;
	eliminar_marco_estructura_clock(nro_marco_en_swap);
}



void* obtener_marco(uint32_t nro_marco){
	void* marco = malloc(tam_pagina);
	memcpy(marco, memoria + nro_marco * tam_pagina, tam_pagina);
	return marco;
}

uint32_t marcos_en_memoria(){
	int cont = 0;
	fila_1er_nivel* tabla_1er_nivel = list_get(lista_tablas_1er_nivel, entrada_1er_nivel_actual);
	for (int i = 0; i < entradas_por_tabla; i++){
		fila_2do_nivel* tabla_2do_nivel = list_get(lista_tablas_2do_nivel, tabla_1er_nivel[i]);
		for (int j = 0; j < entradas_por_tabla; j++){
			fila_2do_nivel* entrada = &tabla_2do_nivel[j];
			if (entrada->nro_marco == -1) continue;
			if (entrada->presencia == 1) cont++;
		}

	}
	return cont;
}

void escribir_marco_en_memoria(uint32_t nro_marco, void* marco){
	uint32_t marco_en_memoria = nro_marco * tam_pagina;
	memcpy(memoria + marco_en_memoria, marco, tam_pagina);
}

// Dado un nro de marco y un desplazamiento, devuelve el dato en concreto
uint32_t read_en_memoria(uint32_t nro_marco, uint16_t desplazamiento){
	uint32_t desplazamiento_final = nro_marco * tam_pagina + desplazamiento;
	uint32_t dato;
	memcpy(&dato, memoria + desplazamiento_final, sizeof(dato));
	return dato;
}

// Dado un nro de marco, un desplazamiento y un dato, escribe el dato en dicha posicion
void write_en_memoria(uint32_t nro_marco, uint16_t desplazamiento, uint32_t dato) {
	uint32_t desplazamiento_final = nro_marco * tam_pagina + desplazamiento;
	memcpy(memoria + desplazamiento_final, &dato, sizeof(dato));
	// falta hacer que cambie el bit de modificado a 1
	entrada_2do_nivel_actual->modificado = 1;
}

////////////////////////// ESTRUCTURA CLOCK //////////////////////////

void agregar_pagina_a_estructura_clock(int32_t nro_marco, fila_2do_nivel* pagina, uint32_t nro_marco_en_swap){
	estructura_clock* estructura = buscar_estructura_clock();
	fila_estructura_clock fila;
	fila.nro_marco_en_memoria = nro_marco;
	fila.pagina = pagina;
	fila.nro_marco_en_swap = nro_marco_en_swap;
	list_add(estructura->marcos_en_memoria, fila);
}

estructura_clock* buscar_estructura_clock(){
	estructura_clock* estructura;
	for (int i = 0; i < list_size(lista_estructuras_clock); i++){
		estructura = list_get(lista_estructuras_clock, i);
		if (pid_actual == estructura->pid)
			return estructura;
	}
	return 0;
}

void eliminar_marco_estructura_clock(uint32_t nro_marco_en_swap){
	estructura_clock* estructura = buscar_estructura_clock();
	fila_estructura_clock* fila;
	for (int i = 0; i < list_size(estructura->marcos_en_memoria); i++){
		fila = list_get(estructura->marcos_en_memoria, i);
		if (nro_marco_en_swap == fila->nro_marco_en_swap)
			list_remove(estructura->marcos_en_memoria,i);
	}
}

void crear_estructura_clock(uint16_t pid){
	estructura_clock estructura;
	estructura.pid = pid;
	estructura.marcos_en_memoria = list_create();
	estructura.puntero = 0;
	list_add(lista_estructuras_clock, estructura);
}


// avanza al proximo marco del proceso
uint16_t avanzar_puntero(uint16_t puntero_clock){
	if(puntero_clock + 1 == marcos_por_proceso)
		return 0;
	else
		return puntero_clock++;
}

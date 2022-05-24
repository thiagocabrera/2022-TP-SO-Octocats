#include "kernel_utils.h"

pthread_mutex_t mx_cola_new = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t s_new_ready = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t s_multip_actual = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mx_pid_sig = PTHREAD_MUTEX_INITIALIZER;
t_dictionary* sockets;
t_queue* cola_new;
t_list* cola_ready;
t_queue* cola_blocked;
t_config* config;
int pid_sig;
int estimacion_inicial;
int grado_multiprogramacion;
int multiprogramacion_actual;

//HILO
void pasaje_new_ready(){

	grado_multiprogramacion = config_get_int_value(config, "GRADO_MULTIPROGRAMACION");
	//solucion de mierda, buscarle la vuelta a hacer una mas linda
	//no se pq pero el semaforo siempre empieza en 1, entonces me caga
	pthread_mutex_lock(&s_new_ready);  // Quizas aplicaria un contador en vez de mutex y nos ahorramos este lock
	while(1){
		pthread_mutex_lock(&s_new_ready);
		pthread_mutex_lock(&mx_cola_new);
		//AGREGAR QUE SI HAY PCBS EN READY SUSPENDED TIENEN PRIORIDAD DE PASO
		if(multiprogramacion_actual < grado_multiprogramacion){
			PCB_t* p = queue_pop(cola_new);
			list_add(cola_ready,p);
			pthread_mutex_lock(&s_multip_actual);
			multiprogramacion_actual++;
			pthread_mutex_unlock(&s_multip_actual);
		}
		pthread_mutex_unlock(&mx_cola_new);
	}

}

void inicializar(){
	logger = log_create("kernel.log", "kernel", 1, LOG_LEVEL_INFO);
	config = config_create("kernel.config");
	cola_new = queue_create();
	cola_blocked = queue_create();
	cola_ready = list_create();
<<<<<<< HEAD
=======
	sockets = dictionary_create();
	pid_sig = 1;
	grado_multiprogramacion = config_get_int_value(config,"GRADO_MULTIPROGRAMACION");
	estimacion_inicial = config_get_int_value(config,"ESTIMACION_INICIAL");
>>>>>>> solucionando_kernel
	pthread_t hilo_pasaje_new_ready;
	pthread_create(&hilo_pasaje_new_ready,NULL,(void*)pasaje_new_ready,NULL);
	pthread_detach(hilo_pasaje_new_ready);
}

<<<<<<< HEAD
void procesar_consola(procesar_consola_args* argumentos){
=======
void procesar_socket(thread_args* argumentos){
>>>>>>> solucionando_kernel
	int cliente_socket = argumentos->cliente;
	char* server_name = argumentos->server_name;
	uint32_t resultOk = 0;
	int32_t resultError = -1;
	free(argumentos);
	op_code cop;
	while (cliente_socket != -1) {

		if (recv(cliente_socket, &cop, sizeof(op_code), 0) <= 0) {
			log_info(logger, "DISCONNECT_FAILURE!");
			send(cliente_socket, &resultError, sizeof(uint32_t), NULL);
			return;
		}

		switch (cop) {
			case PROGRAMA:
			{
				char** instrucciones = string_array_new();
				uint16_t tamanio = 0;

				if (!recv_programa(cliente_socket, instrucciones, &tamanio)) {
					log_error(logger, "Fallo recibiendo PROGRAMA");
					break;
				}

				log_info(logger, "Tamanio %d", tamanio);
				for(int i = 0; i <  string_array_size(instrucciones); i++){
					log_info(logger, "Instruccion numero %d: %s", i, instrucciones[i]);
				}

				PCB_t proceso;
				proceso.instrucciones = string_array_new();
<<<<<<< HEAD
				proceso.instrucciones = instrucciones;
				proceso.pid = 1;
				proceso.tamanio = tamanio;
				proceso.pc = 0;
				proceso.tabla_paginas = 0;
				proceso.est_rafaga = 5;

				queue_push(cola_new,&proceso);

/****************ESTO LO VA A HACER LA COLA DE READY********************************************************/
=======

				pthread_mutex_lock(&mx_pid_sig);
				proceso.pid = pid_sig;
				pid_sig+=1;
				pthread_mutex_unlock(&mx_pid_sig);

				proceso.tamanio = tamanio;
				proceso.pc = 0;
				proceso.instrucciones = instrucciones;
				proceso.tabla_paginas = 0;   // SOLO LO INICIALIZAMOS, MEMORIA NOS VA A ENVIAR EL VALOR
				proceso.est_rafaga = estimacion_inicial;  // ESTO VA POR ARCHIVO DE CONFIGURACION
>>>>>>> solucionando_kernel

				pthread_mutex_lock(&mx_cola_new);
				queue_push(cola_new,&proceso);
				pthread_mutex_unlock(&mx_cola_new);

				pthread_mutex_unlock(&s_new_ready);
//				ip_cpu = config_get_string_value(config,"IP_CPU");
//				puerto_cpu_dispatch = config_get_string_value(config,"PUERTO_CPU_DISPATCH");
//
//				conexion_cpu_dispatch = crear_conexion(logger, "CPU DISPATCH", ip_cpu, puerto_cpu_dispatch);
//
//				send_proceso(conexion_cpu_dispatch, proceso);
/**********************************************************************************************************/

				string_array_destroy(instrucciones);
				log_info(logger,"Cola de new: %d",queue_size(cola_new));
				log_info(logger,"Cola de ready: %d", list_size(cola_ready));

//				send(cliente_socket, &resultOk, sizeof(uint32_t), NULL); // Forma de avisar a la consola, no iria aca

				return;
			}
			//si la variable que evaluamos en el switch es un op_code
			//que tiene de posibilidades definidas 0 (PROGRAMA) o 1 (PROCESO)
			//cuando en la vida puede llegar a dar -1???
			// Errores
<<<<<<< HEAD
			case -1:
				log_error(logger, "Cliente desconectado de kernel");
				break;
=======
//			case -1:
//				log_error(logger, "Cliente desconectado de kernel");
//				break;
>>>>>>> solucionando_kernel
			default:
				log_error(logger, "Algo anduvo mal en el server del kernel ");
				log_info(logger, "Cop: %d", cop);
				break;
		}
	}

	liberar_conexion(&cliente_socket);

}

int escuchar_servidor(char* name, int server_socket){
<<<<<<< HEAD
	printf("Nro de socket (4 por favor): %d\n",server_socket);
=======

>>>>>>> solucionando_kernel
	int cliente_socket = esperar_cliente(logger, name, server_socket);
	printf("(?)\n");
	if (cliente_socket != -1){
<<<<<<< HEAD
		pthread_t hilo_procesar_consola;
		procesar_consola_args* arg = malloc(sizeof(procesar_consola_args));
		arg->cliente = cliente_socket;
		arg->server_name = name;
		printf("Creando un hilo, maestro\n");
		pthread_create(&hilo_procesar_consola,NULL,(void*)procesar_consola,(void*)arg);
		pthread_detach(hilo_procesar_consola);
		printf("Pude crear el hilo todo bien mostro\n");
=======
		pthread_t hilo;
		thread_args* arg = malloc(sizeof(thread_args));
		arg->cliente = cliente_socket;
		arg->server_name = name;
		pthread_create(&hilo, NULL, (void*) procesar_socket, (void*) arg);
		pthread_detach(hilo);
>>>>>>> solucionando_kernel
		return 1;
	}
	return 0;
}

<<<<<<< HEAD
//HILO
void pasaje_new_ready(){

	int grado_multiprogramacion = config_get_int_value(config, "GRADO_MULTIPROGRAMACION");
	while(1){
		pthread_mutex_lock(&sem_ready);
		//AGREGAR QUE SI HAY PCBS EN READY SUSPENDED TIENEN PRIORIDAD DE PASO
		if(multiprogramacion_actual < grado_multiprogramacion){
			PCB_t* p = queue_pop(cola_new);
			list_add(cola_ready,p);
		}
		pthread_mutex_unlock(&sem_ready);
	}

}

=======
>>>>>>> solucionando_kernel
//exit-->console
//dispatch --> agregar a ready
//agregar semaforos a fifo,sjf,etc
//colas de suspended ready y suspended blocked

PCB_t* fifo(){
	return list_remove(cola_ready,0);
}

PCB_t* sjf(){

	PCB_t* primer_pcb = list_get(cola_ready,0);
	double raf_min = primer_pcb->est_rafaga;
	int index_pcb = 0;
	int i;
	for (i = 1;i<list_size(cola_ready);i++){
		PCB_t* pcb = list_get(cola_ready,i);
		double est_raf = pcb->est_rafaga;
		//si el sjf luego sigue con FIFO, entonces tiene que ser >
		//si el sjf luego sigue con LIFO, entonces tiene que ser >=
		if(raf_min >= est_raf){
			raf_min = est_raf;
			index_pcb = i;
		}
	}
	return list_remove(cola_ready,index_pcb);

}
<<<<<<< HEAD

=======
>>>>>>> solucionando_kernel

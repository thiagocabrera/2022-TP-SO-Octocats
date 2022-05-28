#include "protocol.h"

/***************************** PROGRAMA *****************************/
// Envio y serializacion
bool send_programa(int fd, t_list* instrucciones, uint16_t tamanio) {
    size_t size;
    void* stream = serializar_programa(&size, instrucciones, tamanio);
    if (send(fd, stream, size, 0) != size) {
        free(stream);
        return false;
    }
    free(stream);
    return true;
}

static void* serializar_programa(size_t* size, t_list* instrucciones, uint16_t tamanio) {
    size_t inst_size = sizeof(char) + sizeof(int32_t)*2;
    size_t length_lista = list_size(instrucciones);
    size_t codigo_size = inst_size*length_lista;
    *size =
          sizeof(op_code)	// 4
        + sizeof(size_t)	// size de size_payload
        + sizeof(size_t)	// size de length_lista
        + codigo_size  		// size de la lista de  instrucciones
        + sizeof(uint16_t);	// tamanio de memoria
    size_t size_payload = *size - sizeof(op_code) - sizeof(size_t);

    void* stream = malloc(*size);
    op_code cop = PROGRAMA;
    size_t acumulador = 0;
    memcpy(stream + acumulador, &cop, sizeof(op_code));
    acumulador += sizeof(op_code);
    memcpy(stream + acumulador, &size_payload, sizeof(size_t));
    acumulador += sizeof(size_t);
    memcpy(stream + acumulador, &length_lista, sizeof(size_t));
    acumulador += sizeof(size_t);
    for (int j = 0; j < length_lista; j++){
    	instruccion_t* inst = list_get(instrucciones,j);
        memcpy(stream + acumulador, &inst->operacion, sizeof(char));
        acumulador += sizeof(char);
        memcpy(stream + acumulador, &inst->arg1, sizeof(int32_t));
        acumulador += sizeof(int32_t);
        memcpy(stream + acumulador, &inst->arg2, sizeof(int32_t));
        acumulador += sizeof(int32_t);
    }
    memcpy(stream + acumulador, &tamanio, sizeof(uint16_t));

    return stream;
}

// Recepcion y deserializacion
bool recv_programa(int fd, t_list* instrucciones, uint16_t* tamanio) {
	size_t size_payload;
    if (recv(fd, &size_payload, sizeof(size_t), 0) != sizeof(size_t))
        return false;
    void* stream = malloc(size_payload);
    if (recv(fd, stream, size_payload, 0) != size_payload) {
        free(stream);
        return false;
    }
    printf("A punto de deserializar un programa!!!\n");
    deserializar_programa(stream, instrucciones, tamanio);
    free(stream);
    return true;
}

static void deserializar_programa(void* stream, t_list* instrucciones, uint16_t* tamanio) {
	size_t length_lista, acumulador = 0;
	// Primero deserializo la longitud de la lista (cantidad de elementos)
    memcpy(&length_lista, stream, sizeof(size_t));

	acumulador += sizeof(size_t);
    for(int i = 0; i < length_lista; i++){
    	instruccion_t* instruccion = malloc(sizeof(instruccion_t));
    	// Despues deserializo el size de cada instruccion
    	memcpy(&instruccion->operacion, stream + acumulador, sizeof(char));
    	acumulador += sizeof(char);
    	memcpy(&instruccion->arg1, stream + acumulador, sizeof(int32_t));
    	acumulador += sizeof(int32_t);
    	memcpy(&instruccion->arg2, stream + acumulador, sizeof(int32_t));
    	acumulador += sizeof(int32_t);
    	list_add(instrucciones,instruccion);
    }
    uint16_t r_tamanio;
    // Finalmente deserializo el tamanio del programa para memoria
    memcpy(&r_tamanio, stream + acumulador, sizeof(uint16_t));
    *tamanio = r_tamanio;
}

/***************************** PROCESO *****************************/
// Envio y serializacion PROGRAMA
bool send_proceso(int fd, PCB_t *proceso) {
    size_t size;
    printf("A punto de serializar un proceso!!!");
    void* stream = serializar_proceso(&size, proceso);
    if (send(fd, stream, size, 0) != size) {
        free(stream);
        return false;
    }
    printf("Si lees esto sos un groso!!\n");
    free(stream);
    return true;
}

static void* serializar_proceso(size_t* size, PCB_t *proceso) {
    size_t size_stream_inst = 0, length_lista = 0;

    // Serializamos todas las instrucciones en un stream auxiliar
    void * stream_inst = serializar_instrucciones_cpu(&size_stream_inst, proceso->instrucciones, &length_lista);

//    length_lista = 7;

    *size =
          sizeof(op_code)   // 4
        + sizeof(size_t)    // size de size_payload
		+ sizeof(uint16_t)  // size de pid
		+ sizeof(uint16_t)  // size de tamanio
        + sizeof(size_t)    // size de length_lista
        + size_stream_inst  // size del stream de instrucciones (stream_inst)
		+ sizeof(uint32_t)  // size de pc
		+ sizeof(uint32_t)  // size de tabla_paginas
        + sizeof(double);   // size de est_rafaga
    size_t size_payload = *size - sizeof(op_code) - sizeof(size_t);

    void* stream = malloc(*size);

    op_code cop = PROCESO;
    memcpy(stream, &cop, sizeof(op_code));
    memcpy(stream + sizeof(op_code), &size_payload, sizeof(size_t));
    memcpy(stream + sizeof(op_code) + sizeof(size_t), &proceso->pid, sizeof(uint16_t));
    memcpy(stream + sizeof(op_code) + sizeof(size_t) + sizeof(uint16_t), &proceso->tamanio, sizeof(uint16_t));
    memcpy(stream + sizeof(op_code) + sizeof(size_t) + sizeof(uint16_t) * 2, &length_lista, sizeof(size_t));
    memcpy(stream + sizeof(op_code) + sizeof(size_t) * 2 + sizeof(uint16_t) * 2, stream_inst, size_stream_inst);
    memcpy(stream + sizeof(op_code) + sizeof(size_t) * 2 + sizeof(uint16_t) * 2 + size_stream_inst, &proceso->pc, sizeof(uint32_t));
    memcpy(stream + sizeof(op_code) + sizeof(size_t) * 2 + sizeof(uint16_t) * 2 + size_stream_inst + sizeof(uint32_t), &proceso->tabla_paginas, sizeof(uint32_t));
    memcpy(stream + sizeof(op_code) + sizeof(size_t) * 2 + sizeof(uint16_t) * 2 + size_stream_inst + sizeof(uint32_t) * 2, &proceso->est_rafaga, sizeof(double));
    free(stream_inst);
    return stream;
}

static void* serializar_instrucciones_cpu(size_t* size_stream_inst, char** instrucciones, size_t* length_instrucciones) {
	void * stream = malloc(sizeof(void*));
	size_t size_instruccion;

	for(uint16_t i = 0; instrucciones[i] != NULL; i++){
		// Primero serializo la longitud de cada instruccion
		size_instruccion = strlen(instrucciones[i]) + 1;
		memcpy(stream + *size_stream_inst, &size_instruccion, sizeof(size_t));
		*size_stream_inst += sizeof(size_t);
		// Despues serializo la instruccion en si
		memcpy(stream + *size_stream_inst, instrucciones[i], size_instruccion);
		*size_stream_inst += size_instruccion;
		*length_instrucciones += 1;
	}

    return stream;
}

//Recepcion y deserealizacion
bool recv_proceso(int fd, PCB_t* proceso) {
    size_t size_payload;
    if (recv(fd, &size_payload, sizeof(size_t), 0) != sizeof(size_t))
        return false;
    void* stream = malloc(size_payload);
    if (recv(fd, stream, size_payload, 0) != size_payload) {
        free(stream);
        return false;
    }
    deserializar_proceso(stream, proceso);
    free(stream);
    return true;
}

static void deserializar_proceso(void* stream, PCB_t* proceso) {
	size_t length_lista, size_instruccion, acumulador = 0;
	uint16_t r_pid, r_tamanio;
	uint32_t r_pc, r_tabla_paginas;
	double r_est_rafaga;

	memcpy(&r_pid, stream, sizeof(uint16_t));
	memcpy(&r_tamanio, stream + sizeof(uint16_t), sizeof(uint16_t));
	memcpy(&length_lista, stream + sizeof(uint16_t) * 2, sizeof(size_t));

    char* r_instruccion;
    acumulador = sizeof(uint16_t) * 2 + sizeof(size_t);

    for(int i = 0; i < length_lista; i++){
    	// Despues deserializo el size de cada instruccion
    	memcpy(&size_instruccion, stream + acumulador, sizeof(size_t));
    	acumulador += sizeof(size_t);
    	r_instruccion = malloc(size_instruccion);
    	// Despues deserializo cada instruccion en si y la guardo en un array de strings
    	memcpy(r_instruccion, stream + acumulador, size_instruccion);
    	acumulador += size_instruccion;

    	string_array_push(&(proceso->instrucciones), r_instruccion);
    }

    // Finalmente deserializo el tamanio del programa para memoria
    memcpy(&r_pc, stream + acumulador, sizeof(uint32_t));
    acumulador += sizeof(uint32_t);
    memcpy(&r_tabla_paginas, stream + acumulador, sizeof(uint32_t));
    acumulador += sizeof(uint32_t);
    memcpy(&r_est_rafaga, stream + acumulador, sizeof(double));

    proceso->pid = r_pid;
    proceso->tamanio = r_tamanio;
    proceso->pc = r_pc;
    proceso->tabla_paginas = r_tabla_paginas;
    proceso->est_rafaga = r_est_rafaga;
}



#include <stdio.h>  
#include <stdlib.h>   
#include <string.h>   
#include <unistd.h>    
#include <sys/mman.h>   
#include <sys/stat.h>   
#include <fcntl.h> 
#include "main.h"
#include "memory.h"


/* Allocates dynamic memory with size bytes, initializes it with zeros,
 * and returns a pointer to the allocated area.
 */
void *allocate_dynamic_memory(int size){
     return calloc(1, (size_t)size);
}

/* Function that reserves a shared-memory region with the size indicated
 * by size and the name name, fills that memory region with the value 0,
 * and returns a pointer to it. The result of getuid() can be concatenated
 * to name to make the shared-memory name unique for the current user.
 */
void *create_shared_memory(char *name, int size){
    void *ptr;
    //Cria zona de memoria partilhada 
    int fd = shm_open(name, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
    if (fd == -1){
        perror("shm_open");
        exit(1);
    }
    // Define o tamanho
    if (ftruncate(fd, size) == -1){
        perror("ftruncate");
        exit(2);
    }

    //Atribui um apontador
    ptr = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (ptr == MAP_FAILED){
        perror("shm-mmap");
        exit(3);
    }
    close(fd);

    //Preenche a reagião de memoria com o valor 0
    memset(ptr, 0, size);
    return ptr;
}

/* Frees a previously allocated dynamic-memory region.
 */
void deallocate_dynamic_memory(void *ptr){
    free(ptr);
}

/* Unmaps and removes a previously created shared-memory region.
 */
void destroy_shared_memory(char *name, void *ptr, int size){
    if(munmap(ptr, size) == -1){
        perror("munmap");
        exit(7);
    }
    if(shm_unlink(name) == -1){
        perror("shm_unlink");
        exit(8);   
    }
}

/* Writes one MeasurementInfo request in the main->sensors circular buffer.
 * Returns 1 on success, or 0 if no free slot is available.
 */
int write_main_sensors_buffer(struct circ_buffer *buffer, int buffer_size, const MeasurementInfo *req){
    int in = buffer->ptrs->in;
    int out = buffer->ptrs->out;

    int next = (in + 1) % buffer_size;

    if (next == out){
        return 0;
    }

    buffer->buffer[in] = *req;
    buffer->ptrs->in = next;

    return 1;
}

/* Reads the next pending MeasurementInfo for expected_m_id from the main->sensors buffer.
 * If no request exists yet, sets req->m_id to -1.
 */
void read_main_sensors_buffer(struct circ_buffer *buffer, int buffer_size, int expected_m_id, MeasurementInfo *req){
    int out = buffer->ptrs->out;
    //ver se está vazio
    if (buffer->ptrs->in == out){
        req->m_id = -1;
        return;
    }
    //ver se é o esperado 
    if (buffer->buffer[out].m_id != expected_m_id){
        req->m_id = -1;
        return;
    }
    //põe em req o request
    *req = buffer->buffer[out];
    //atualiza a contagem de sensores
    buffer->buffer[out].counter_sensors--;
    //liberta a posição se todos tiverem lido 
    if (buffer->buffer[out].counter_sensors == 0){
        buffer->ptrs->out = (out + 1) % buffer_size;
    }
}

/* Writes one measurement to the shared sensor->controller random-access buffer.
 * Returns 1 on success, or 0 if the buffer is full.
 */
int write_sensor_controller_buffer(struct ra_buffer *buffer, int buffer_size, const MeasurementInfo *m){
    int index = m->sensor_id;

    if (index < 0 || index >= buffer_size){
        return 0;
    }

    if (buffer->ptrs[index] == 1){
        return 0;
    }

    buffer->buffer[index] = *m;
    buffer->ptrs[index] = 1;

    return 1;
}

/* Reads one measurement for controller_id from the shared sensor->controller buffer.
 * If there is no unread measurement from the paired sensor, sets m->m_id to -1.
 */
void read_sensor_controller_buffer(struct ra_buffer *buffer, int controller_id, int buffer_size, MeasurementInfo *m){
    if (controller_id < 0 || controller_id >= buffer_size){
        m->m_id = -1;
        return;
    }

    if (buffer->ptrs[controller_id] == 0){
        m->m_id = -1;
        return;
    }

    *m = buffer->buffer[controller_id];
    buffer->ptrs[controller_id] = 0;
}

/* Writes one measurement to the controller->server random-access buffer.
 * Returns 1 on success, or 0 if the buffer is full.
 */
int write_controller_servers_buffer(struct ra_buffer *buffer, int buffer_size, const MeasurementInfo *m){
    int index = m->controller_id;

    if (index < 0 || index >= buffer_size){
        return 0;
    }

    if (buffer->ptrs[index] == 1){
        return 0;
    }

    buffer->buffer[index] = *m;
    buffer->ptrs[index] = 1;

    return 1;
}

/* Reads the next expected measurement for one server from the controller->server random-access buffer.
 * expected_m_id and expected_controller_id identify which controller measurement this server is waiting for.
 * If that measurement is not in the buffer yet, sets m->m_id to -1.
 */
void read_controller_servers_buffer(struct ra_buffer *buffer, int buffer_size, int expected_m_id, int expected_controller_id, MeasurementInfo *m){
    if (expected_controller_id < 0 || expected_controller_id >= buffer_size){
        m->m_id = -1;
        return;
    }

    if (buffer->ptrs[expected_controller_id] == 0){
        m->m_id = -1;
        return;
    }

    if (buffer->buffer[expected_controller_id].m_id != expected_m_id){
        m->m_id = -1;
        return;
    }

    *m = buffer->buffer[expected_controller_id];

    buffer->buffer[expected_controller_id].counter_servers--;

    if (buffer->buffer[expected_controller_id].counter_servers == 0){
        buffer->ptrs[expected_controller_id] = 0;
    }
}


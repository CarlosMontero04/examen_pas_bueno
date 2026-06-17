#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <mqueue.h> // Para las colas mq_*
#include <errno.h>

// Incluimos tu archivo común
#include "ej3_common.h"

// -------------------------------------------------------------------------
// FUNCIONES AUXILIARES
// -------------------------------------------------------------------------

/*
 * Función para imprimir por pantalla y guardar en el fichero de log a la vez.
 * Tal y como pide el requisito 4 del guion.
 */
void funcionLog(const char *mensaje) {
    printf("%s\n", mensaje); // Imprime en consola
    
    // Abre el archivo en modo "a" (append) para añadir al final sin borrar
    FILE *f = fopen("log-servidor.txt", "a");
    if (f != NULL) {
        fprintf(f, "%s\n", mensaje);
        fclose(f);
    }
}

// -------------------------------------------------------------------------
// PROGRAMA PRINCIPAL (SERVIDOR)
// -------------------------------------------------------------------------

int main() {
    // 1. OBTENER EL NOMBRE DEL USUARIO PARA LAS COLAS ÚNICAS
    char *usuario = getenv("USER");
    if (usuario == NULL) {
        perror("Error al obtener el usuario");
        exit(EXIT_FAILURE);
    }

    // Preparamos los nombres finales de las colas (ej: "/server_queue-juan")
    char server_queue_name[100];
    char client_queue_name[100];
    sprintf(server_queue_name, "%s-%s", SERVER_QUEUE, usuario);
    sprintf(client_queue_name, "%s-%s", CLIENT_QUEUE, usuario);

    // 2. CONFIGURAR LOS ATRIBUTOS DE LAS COLAS
    struct mq_attr attr;
    attr.mq_flags = 0;              // 0 significa modo bloqueante (se queda esperando)
    attr.mq_maxmsg = 10;            // Máximo 10 mensajes en la cola a la vez
    attr.mq_msgsize = MAX_SIZE;     // Tamaño máximo de cada mensaje (1024, de tu common.h)
    attr.mq_curmsgs = 0;            // Mensajes actuales (se inicializa a 0)

    char log_buffer[MAX_SIZE + 100]; // Buffer para crear mensajes largos para el log

    // 3. CREAR Y ABRIR LAS COLAS
    // El servidor CREA las dos colas (O_CREAT). 
    // - La de servidor (para recibir del cliente) se abre como O_RDONLY (Solo lectura).
    // - La de cliente (para responderle) se abre como O_WRONLY (Solo escritura).
    // El 0644 son los permisos (Lectura/Escritura para el dueño).
    mqd_t mq_server = mq_open(server_queue_name, O_CREAT | O_RDONLY, 0644, &attr);
    mqd_t mq_client = mq_open(client_queue_name, O_CREAT | O_WRONLY, 0644, &attr);

    if (mq_server == (mqd_t)-1 || mq_client == (mqd_t)-1) {
        funcionLog("[SERVIDOR] Error: No se pudieron crear las colas.");
        exit(EXIT_FAILURE);
    }

    sprintf(log_buffer, "[SERVIDOR] Colas creadas exitosamente: %s y %s", server_queue_name, client_queue_name);
    funcionLog(log_buffer);

    // 4. BUCLE PRINCIPAL DEL SERVIDOR
    char buffer_recepcion[MAX_SIZE];
    char buffer_respuesta[MAX_SIZE];

    while (1) {
        funcionLog("[SERVIDOR] Esperando mensaje del cliente...");

        // mq_receive(descriptor_cola, buffer_donde_guardar, tamaño_max, prioridad)
        ssize_t bytes_leidos = mq_receive(mq_server, buffer_recepcion, MAX_SIZE, NULL);
        if (bytes_leidos == -1) {
            funcionLog("[SERVIDOR] Error al recibir el mensaje.");
            break; // Si hay error, salimos del bucle
        }

        // Ponemos el fin de cadena por seguridad
        buffer_recepcion[bytes_leidos] = '\0';

        // Comprobamos si el cliente nos ha mandado la orden de parar ("exit")
        if (strncmp(buffer_recepcion, MSG_STOP, strlen(MSG_STOP)) == 0) {
            funcionLog("[SERVIDOR] Orden de desconexión recibida ('exit'). Cerrando servidor.");
            break; 
        }

        sprintf(log_buffer, "[SERVIDOR] Mensaje recibido: \"%s\"", buffer_recepcion);
        funcionLog(log_buffer);

        // 5. PROCESAR EL MENSAJE (Contar caracteres)
        // El guion dice que hay que contar sin incluir el \0, pero los espacios cuentan.
        // Como 'buffer_recepcion' tiene un '\n' al final (de cuando el usuario pulsa INTRO en el cliente),
        // normalmente se le resta 1 para no contar el intro, pero lo dejaremos como strlen normal si solo queremos contar letras.
        int num_caracteres = strlen(buffer_recepcion); 

        // 6. ENVIAR LA RESPUESTA
        // Formateamos el mensaje tal y como exige el guion
        sprintf(buffer_respuesta, "Número de caracteres recibidos: %d", num_caracteres);
        
        // mq_send(descriptor_cola, mensaje_a_enviar, longitud_mensaje, prioridad)
        if (mq_send(mq_client, buffer_respuesta, strlen(buffer_respuesta), 0) == -1) {
            funcionLog("[SERVIDOR] Error al enviar la respuesta.");
        } else {
            sprintf(log_buffer, "[SERVIDOR] Respuesta enviada: %s", buffer_respuesta);
            funcionLog(log_buffer);
        }
    }

    // 7. LIMPIEZA FINAL (OBLIGATORIA)
    funcionLog("[SERVIDOR] Cerrando y eliminando las colas...");
    
    // Primero se cierran (se desconectan de este programa)
    mq_close(mq_server);
    mq_close(mq_client);
    
    // Luego se destruyen (se borran de la memoria del Sistema Operativo)
    mq_unlink(server_queue_name);
    mq_unlink(client_queue_name);

    funcionLog("[SERVIDOR] Servidor finalizado con éxito.");
    
    return EXIT_SUCCESS;
}



/*O_CREAT | O_RDONLY: Memoriza esto. Al usar colas POSIX, un extremo siempre es de Solo Lectura y el otro de Solo Escritura.*/

/*mq_unlink(): ¡Súper importante! A diferencia de las tuberías (pipes) que se borran solas cuando los procesos mueren, las colas de mensajes se quedan en la RAM del sistema operativo hasta que reinicies el ordenador o hagas un mq_unlink().
 El servidor es el responsable de limpiar la basura antes de irse.*/


/*¡Perfecto! Tienes toda la razón, en un examen es preferible que sobre información a que falte.

Aquí tienes tu Gran Chuleta del Ejercicio 3 (Colas de Mensajes y Señales). Está diseñada como un bloque de comentarios para que la puedas guardar en un archivo chuleta_colas.c o añadirla a las anteriores. He destacado especialmente el "truco" de las banderas cruzadas y cómo enlazar la librería en la terminal.

Cópiala y guárdala a buen recaudo:

C
/* 

   0. COMPILACIÓN (¡SÚPER IMPORTANTE!)
   -------------------------------------------------------------------------
   * A diferencia de los pipes, las colas POSIX requieren enlazar una librería 
     matemática/tiempo real externa.
   * Comando de compilación OBLIGATORIO: gcc archivo.c -o programa -lrt
                                                                    ^^^^
   1. ATRIBUTOS DE LA COLA (struct mq_attr)
   -------------------------------------------------------------------------
   Antes de crear una cola (en el Servidor), hay que definir sus límites:
   struct mq_attr attr;
   attr.mq_flags = 0;           // 0 = Bloqueante (espera si está vacía/llena)
   attr.mq_maxmsg = 10;         // Nº máximo de mensajes que caben a la vez
   attr.mq_msgsize = 1024;      // Tamaño máximo de CADA mensaje en bytes
   attr.mq_curmsgs = 0;         // Mensajes que hay ahora mismo (empieza en 0)

   2. ABRIR / CREAR COLAS (<mqueue.h>)
   -------------------------------------------------------------------------
   * Función: mqd_t mq_open(nombre_cola, banderas, permisos, &atributos);
     - nombre_cola: SIEMPRE debe empezar por la barra '/' (ej: "/mi_cola").
     - permisos: Se suele usar 0644 (Lectura/Escritura para el propietario).

   * MODO SERVIDOR (El que CREA la cola):
     // Cola para RECIBIR del cliente (Solo Lectura)
     mqd_t mq_server = mq_open("/c2s", O_CREAT | O_RDONLY, 0644, &attr);
     
     // Cola para ENVIAR al cliente (Solo Escritura)
     mqd_t mq_client = mq_open("/s2c", O_CREAT | O_WRONLY, 0644, &attr);

   * MODO CLIENTE (El que se CONECTA a una cola existente):
     // Banderas INVERTIDAS respecto al servidor. SIN O_CREAT y SIN atributos.
     // Cola del servidor (para ENVIARLE cosas - Solo Escritura)
     mqd_t mq_server = mq_open("/c2s", O_WRONLY);
     
     // Cola del cliente (para RECIBIR respuestas - Solo Lectura)
     mqd_t mq_client = mq_open("/s2c", O_RDONLY);

   3. ENVIAR Y RECIBIR MENSAJES
   -------------------------------------------------------------------------
   * ENVIAR (mq_send):
     int mq_send(mqd_t cola, const char *mensaje, size_t tamaño, unsigned int prioridad);
     - Ejemplo: mq_send(mq_server, buffer, strlen(buffer), 0);
     - ¡Ojo! Si mandas texto, usa strlen(buffer). Si mandas binarios/números usa sizeof().
     - Prioridad: Normalmente 0. Si hay mensajes con mayor prioridad (ej: 1), 
       el receptor los leerá primero aunque hayan llegado más tarde.

   * RECIBIR (mq_receive):
     ssize_t mq_receive(mqd_t cola, char *buffer, size_t tamaño_max, unsigned int *prioridad);
     - Ejemplo: ssize_t leidos = mq_receive(mq_client, buffer, 1024, NULL);
     - Devuelve el número de bytes leídos (útil para poner el '\0' al final del texto).
     - tamaño_max DEBE SER IGUAL O MAYOR al mq_msgsize que le pusimos al crear la cola.

   4. CERRAR Y DESTRUIR (LIMPIEZA)
   -------------------------------------------------------------------------
   * mq_close(mqd_t cola);
     - Desconecta el programa actual de la cola.
     - LO HACEN AMBOS (Cliente y Servidor) antes de hacer un exit() o return.

   * mq_unlink(const char *nombre_cola);
     - Borra la cola de la memoria RAM del Sistema Operativo.
     - SOLO LO HACE EL CREADOR (El Servidor). Si no se hace, la cola se queda 
       ocupando memoria de forma "fantasma" hasta reiniciar el ordenador.

   5. SEÑALES / INTERRUPCIONES (<signal.h>)
   -------------------------------------------------------------------------
   Sirven para evitar que el programa muera "a lo bruto" si el usuario pulsa Ctrl+C.
   
   * Paso A: Crear una función manejadora global:
     void mi_manejador(int num_senal) {
         // Mandar mensaje de "exit" a la otra máquina
         // Hacer mq_close() de las colas
         // Hacer exit(0);
     }

   * Paso B: Conectar la señal a nuestra función al principio del main():
     signal(SIGINT, mi_manejador);  // SIGINT = Ctrl+C en teclado
     signal(SIGTERM, mi_manejador); // SIGTERM = Señal de matar proceso estándar

   =========================================================================
*/
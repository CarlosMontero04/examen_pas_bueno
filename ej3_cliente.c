#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <mqueue.h>
#include <errno.h>
#include <signal.h> // Para capturar SIGINT y SIGTERM

#include "ej3_common.h" // Usamos el common.h que definiste

// Variables globales para las colas, porque el manejador de señales necesita acceder a ellas
mqd_t mq_server = -1;
mqd_t mq_client = -1;

// -------------------------------------------------------------------------
// FUNCIONES AUXILIARES
// -------------------------------------------------------------------------

/*
 * Función de Log: Imprime en consola y escribe en el archivo.
 */
void funcionLog(const char *mensaje) {
    printf("%s\n", mensaje);
    FILE *f = fopen("log-cliente.txt", "a");
    if (f != NULL) {
        fprintf(f, "%s\n", mensaje);
        fclose(f);
    }
}

/*
 * Manejador de señales (Signal Handler).
 * Esta función se ejecuta automáticamente si el usuario pulsa Ctrl+C (SIGINT)
 * o si se envía una señal de terminación (SIGTERM).
 */
void manejador_senales(int signal) {
    funcionLog("\n[CLIENTE] Señal de terminación recibida (Ctrl+C). Cerrando ordenadamente...");

    // 1. Avisamos al servidor para que él también se apague
    // Mandamos la palabra definida en MSG_STOP ("exit")
    if (mq_server != -1) {
        mq_send(mq_server, MSG_STOP, strlen(MSG_STOP), 0);
    }

    // 2. Cerramos nuestros extremos de las colas
    if (mq_server != -1) mq_close(mq_server);
    if (mq_client != -1) mq_close(mq_client);

    // Nota: El cliente NO hace mq_unlink(). Eso es tarea del servidor.

    funcionLog("[CLIENTE] Desconectado. Adiós.");
    exit(EXIT_SUCCESS); // Salimos del programa de forma limpia
}

// -------------------------------------------------------------------------
// PROGRAMA PRINCIPAL (CLIENTE)
// -------------------------------------------------------------------------

int main() {
    // 1. CONFIGURAR LA CAPTURA DE SEÑALES
    // Le decimos al SO: "Si recibes SIGINT o SIGTERM, no mates el programa a lo bruto. 
    // Llama a mi función 'manejador_senales' primero".
    signal(SIGINT, manejador_senales);
    signal(SIGTERM, manejador_senales);

    // 2. OBTENER EL NOMBRE DEL USUARIO (Igual que en el servidor)
    char *usuario = getenv("USER");
    if (usuario == NULL) {
        perror("Error al obtener el usuario");
        exit(EXIT_FAILURE);
    }

    // Generar nombres
    char server_queue_name[100];
    char client_queue_name[100];
    sprintf(server_queue_name, "%s-%s", SERVER_QUEUE, usuario);
    sprintf(client_queue_name, "%s-%s", CLIENT_QUEUE, usuario);

    char log_buffer[MAX_SIZE + 100];
    sprintf(log_buffer, "[CLIENTE] Intentando conectar a las colas del servidor...");
    funcionLog(log_buffer);

    // 3. ABRIR LAS COLAS (¡Fíjate en las banderas cruzadas que hablamos!)
    // IMPORTANTE: Aquí NO hay O_CREAT. Asumimos que el servidor ya las creó.
    // - La del servidor (para mandarle cosas): O_WRONLY
    // - La del cliente (para recibir respuestas): O_RDONLY
    mq_server = mq_open(server_queue_name, O_WRONLY);
    mq_client = mq_open(client_queue_name, O_RDONLY);

    if (mq_server == (mqd_t)-1 || mq_client == (mqd_t)-1) {
        funcionLog("[CLIENTE] Error: No se pudo conectar a las colas. ¿Está el servidor encendido?");
        exit(EXIT_FAILURE);
    }

    funcionLog("[CLIENTE] Conexión establecida. Escribe un mensaje (o 'exit' para salir):");

    // 4. BUCLE PRINCIPAL DEL CLIENTE
    char buffer_envio[MAX_SIZE];
    char buffer_recepcion[MAX_SIZE];

    while (1) {
        printf("> "); // Prompt para el usuario
        fflush(stdout);

        // Leemos texto del teclado
        if (fgets(buffer_envio, MAX_SIZE, stdin) == NULL) {
            break; // Si hay error leyendo (ej: fin de archivo)
        }

        // fgets guarda el intro '\n' al final. Lo cambiamos por '\0' para que el string sea más limpio
        buffer_envio[strcspn(buffer_envio, "\n")] = '\0';

        // Enviamos el mensaje al servidor
        if (mq_send(mq_server, buffer_envio, strlen(buffer_envio), 0) == -1) {
            funcionLog("[CLIENTE] Error al enviar el mensaje.");
            break;
        }

        // Si el usuario tecleó "exit", rompemos el bucle normal (el servidor ya habrá recibido el "exit")
        if (strcmp(buffer_envio, MSG_STOP) == 0) {
            funcionLog("[CLIENTE] Orden de salida enviada por teclado.");
            break;
        }

        // Esperamos la respuesta del servidor (Bloqueante)
        ssize_t bytes_leidos = mq_receive(mq_client, buffer_recepcion, MAX_SIZE, NULL);
        if (bytes_leidos != -1) {
            buffer_recepcion[bytes_leidos] = '\0';
            sprintf(log_buffer, "[CLIENTE] Respuesta del servidor: %s", buffer_recepcion);
            funcionLog(log_buffer);
        } else {
            funcionLog("[CLIENTE] Error al recibir respuesta.");
            break;
        }
    }

    // 5. LIMPIEZA FINAL (Si salimos del bucle tecleando 'exit')
    funcionLog("[CLIENTE] Cerrando conexiones...");
    mq_close(mq_server);
    mq_close(mq_client);
    
    // Recuerda: NO hacemos mq_unlink(). Eso es del servidor.

    funcionLog("[CLIENTE] Programa terminado.");
    return EXIT_SUCCESS;
}



/*
Comado para mandar seniales:
kill -X pid

Enteros asociados a las macros de señales:
1) SIGHUP       2) SIGINT       3) SIGQUIT      4) SIGILL
5) SIGTRAP      6) SIGABRT      7) SIGBUS       8) SIGFPE
9) SIGKILL     10) SIGUSR1     11) SIGSEGV     12) SIGUSR2
13) SIGPIPE     14) SIGALRM     15) SIGTERM     17) SIGCHLD
18) SIGCONT     19) SIGSTOP     20) SIGTSTP     21) SIGTTIN
22) SIGTTOU     23) SIGURG      24) SIGXCPU     25) SIGXFSZ
26) SIGVTALRM   27) SIGPROF     28) SIGWINCH    29) SIGIO
30) SIGPWR      31) SIGSYS      ....
*/
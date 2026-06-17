#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>     // Para pipe(), fork(), read(), write(), close()
#include <sys/wait.h>   // Para wait() y macros WIFEXITED, etc.
#include <time.h>       // Para time() usado en la semilla de rand()
#include <errno.h>      // Para control de errores
#include <string.h>     // Para strerror()

// -------------------------------------------------------------------------
// FUNCIONES MODULARES (Preparadas para copiar y pegar en un examen)
// -------------------------------------------------------------------------

/*
 * Función que genera dos floats aleatorios, los suma y devuelve el resultado.
 * Si en el examen te piden cambiar la operación (ej. multiplicar), 
 * solo cambias esta función y el resto del programa ni se entera.
 */
float generar_y_sumar_flotantes() {
    // rand() genera enteros. Los dividimos por un float para sacar decimales.
    float num1 = (float)rand() / (float)(RAND_MAX / 100.0); // Número entre 0.0 y 100.0
    float num2 = (float)rand() / (float)(RAND_MAX / 100.0);
    float suma = num1 + num2;
    
    printf("[PROCESO GENERADOR] Generados: %.2f y %.2f. Suma = %.2f\n", num1, num2, suma);
    return suma;
}

/*
 * Función para imprimir el resultado recibido.
 */
void procesar_resultado(float resultado) {
    printf("[PROCESO RECEPTOR] He recibido la suma correctamente: %.2f\n", resultado);
}


// -------------------------------------------------------------------------
// PROGRAMA PRINCIPAL
// -------------------------------------------------------------------------

int main() {
    // 1. Declaración de variables necesarias
    int fildes[2];     // Array que guardará los dos extremos de la tubería: fildes[0] leer, fildes[1] escribir.
    pid_t rf;          // Guardará el ID del proceso devuelto por fork()
    int status;        // Para recoger el estado en el que termina el hijo
    float suma_enviar; // Variable que usará el emisor
    float suma_recibir;// Variable que usará el receptor

    // 2. CREACIÓN DE LA TUBERÍA
    // La tubería debe crearse SIEMPRE antes del fork(), para que al clonarse el proceso,
    // el hijo también herede los descriptores de la tubería abierta.
    if (pipe(fildes) == -1) {
        perror("Error al crear la tubería");
        exit(EXIT_FAILURE);
    }

    // 3. CREACIÓN DEL PROCESO HIJO
    rf = fork();

    switch (rf) {
        case -1:
            // Error al crear el proceso
            perror("Error en el fork()");
            exit(EXIT_FAILURE);
            break;

        case 0:
            
            // =========================================================
            // CÓDIGO DEL PROCESO HIJO (Generador / Emisor)
            // =========================================================
            
            // REGLA DE ORO DE LOS PIPES: Cerrar siempre el extremo que no vas a usar.
            // Como el hijo va a ESCRIBIR, cerramos su extremo de LECTURA (fildes[0]).
            close(fildes[0]);

            // 1. Generamos la suma usando nuestra función modular
            suma_enviar = generar_y_sumar_flotantes();

            // 2. Enviamos el dato por la tubería
            // Función write: (descriptor, puntero_al_dato, tamaño_en_bytes)
            if (write(fildes[1], &suma_enviar, sizeof(float)) == -1) {
                perror("Error del hijo al escribir en la tubería");
                exit(EXIT_FAILURE);
            }

            // 3. Cerramos el extremo de escritura tras terminar y salimos
            close(fildes[1]);
            exit(EXIT_SUCCESS); 
            break;
            
            break;

        default:
            // =========================================================
            // CÓDIGO DEL PROCESO PADRE
            // =========================================================
            // REGLA DE ORO DE LOS PIPES: Cerrar siempre el extremo que no vas a usar.
            // Como el padre va a LEER, cerramos su extremo de ESCRITURA (fildes[1]).
            close(fildes[1]);

            // 1. Leemos el dato de la tubería.
            // Nota: La función read() es BLOQUEANTE. El padre se quedará aquí pausado 
            // esperando hasta que el hijo escriba algo en la tubería.
            // Función read: (descriptor, puntero_donde_guardar, tamaño_en_bytes)
            if (read(fildes[0], &suma_recibir, sizeof(float)) == -1) {
                perror("Error del padre al leer de la tubería");
                exit(EXIT_FAILURE);
            }

            // 2. Imprimimos el resultado usando nuestra función modular
            procesar_resultado(suma_recibir);

            // 3. Cerramos el extremo de lectura porque ya no lo necesitamos más
            close(fildes[0]);

            // 4. SINCRONIZACIÓN: El padre siempre debe esperar a que su hijo muera
            // para evitar que se convierta en un proceso "Zombie".
            wait(&status);
            
            // Opcional pero muy profesional: comprobar cómo terminó el hijo
            if (WIFEXITED(status)) {
                printf("[PROCESO PADRE] El hijo terminó correctamente con estado %d.\n", WEXITSTATUS(status));
            }

        
            
            break;
        }
    
    return EXIT_SUCCESS;
}

/*1. TUBERÍAS / PIPES (<unistd.h>)
   -------------------------------------------------------------------------
   * int pipe(int fildes[2]): Crea un canal de comunicación unidireccional.
     - fildes[0] : Extremo de LECTURA (Boca / Leer / Recibir).
     - fildes[1] : Extremo de ESCRITURA (Bolígrafo / Escribir / Enviar).
   
   * REGLA DE ORO 1: El pipe() se hace SIEMPRE ANTES del fork(). 
     Si se hace después, padre e hijo tendrán tuberías distintas y no se comunicarán.
   
   * REGLA DE ORO 2: Hay que cerrar SIEMPRE el extremo que no se usa en cada proceso.
     - En el proceso que ENVÍA (Escribe): close(fildes[0]);
     - En el proceso que RECIBE (Lee):    close(fildes[1]);
     (Si no cierras el de escritura en el receptor, el read() se quedará bloqueado para siempre esperando datos infinitos).

   2. CREACIÓN DE PROCESOS (<unistd.h>)
   -------------------------------------------------------------------------
   * pid_t fork(void): Clona el proceso actual en dos procesos idénticos.
     - Devuelve -1 : Error.
     - Devuelve 0  : Estamos dentro del código del proceso HIJO.
     - Devuelve >0 : Estamos dentro del código del proceso PADRE (y el número es el PID del hijo).

   * Funciones útiles de IDs:
     - getpid()  : Devuelve el ID del proceso actual.
     - getppid() : Devuelve el ID del proceso padre.

   3. ENVIAR Y RECIBIR DATOS (Lectura / Escritura)
   -------------------------------------------------------------------------
   * write(int fd, const void *buf, size_t n):
     - Ej enviar 1 float  : write(fildes[1], &mi_float, sizeof(float));
     - Ej enviar 1 int    : write(fildes[1], &mi_int, sizeof(int));
     - Ej enviar cadena   : write(fildes[1], mi_string, strlen(mi_string) + 1); // +1 para el '\0'
     - Ej enviar array 10 : write(fildes[1], mi_array_int, sizeof(int) * 10);
   
   * read(int fd, void *buf, size_t nbytes):
     - Ej recibir 1 float : read(fildes[0], &variable_destino, sizeof(float));
     - (Misma lógica de tamaños que el write).
     - ¡OJO!: read() es BLOQUEANTE por defecto. El programa se pausa ahí hasta que alguien escriba.

   4. SINCRONIZACIÓN Y ESPERA (<sys/wait.h>)
   -------------------------------------------------------------------------
   * wait(int *status): El padre pausa su ejecución hasta que un hijo termine.
     - Evita que los hijos se queden como procesos "Zombies".
   * Macros para evaluar el 'status':
     - WIFEXITED(status)   : Devuelve true (1) si el hijo terminó con normalidad (con exit o return).
     - WEXITSTATUS(status) : Saca el código de salida exacto del hijo (ej: el 0 de exit(0)).

   5. UTILIDADES EXTRAS DEL EJERCICIO 2 (<time.h> y <stdlib.h>)
   -------------------------------------------------------------------------
   * Generación de números aleatorios:
     - srand(time(NULL)); : Inicializa la semilla. (Solo se llama UNA vez al inicio del main).
     - rand() : Genera un número entero aleatorio.
     - (float)rand() / (float)(RAND_MAX / maximo) : Fórmula para sacar un float decimal.

   =========================================================================
*/
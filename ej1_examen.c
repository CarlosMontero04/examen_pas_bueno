#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <mqueue.h> // Para las colas mq_*
#include <errno.h>
#include <getopt.h>     // Para usar la función avanzada getopt_long()
#include <pwd.h>        // Para las funciones/estructuras de usuarios 
#include <grp.h>        // Para las funciones/estructuras de grupos 
#include <stdbool.h>    // Para poder usar variables booleanas 
#include <ctype.h>      // Para funciones de caracteres 
#include <sys/wait.h>   // <-- AÑADIDO: Obligatorio para usar wait()

#define MAX_SIZE 1024


int main(int argc, char **argv) {
    // ---------------------------------------------------------------------
    // FASE 1: PREPARACIÓN
    // ---------------------------------------------------------------------
    char *usuario = getenv("USER");
    if (usuario == NULL) {
        perror("Error al obtener el usuario");
        exit(EXIT_FAILURE);
    }

    char nombre_cola[100];
    sprintf(nombre_cola, "/cola_examen_%s", usuario);

    // ---------------------------------------------------------------------
    // FASE 2: PROCESADO DE ARGUMENTOS
    // ---------------------------------------------------------------------
    char *palabra_salida = "exit"; 
    int limite_gid = -1; 
    bool flag_g = false; 

    static struct option long_options[] = {
        {"help",    no_argument,       NULL, 'h'},
        {"psalida", required_argument, NULL, 'p'},
        {0, 0, 0, 0}
    };

    int c;
    opterr = 0; 
    
    while ((c = getopt_long(argc, argv, "hp:g:", long_options, NULL)) != -1) {
        switch (c) {
            case 'h':
                printf("Uso: %s [-p/--psalida palabra] [-g limite_gid] [-h/--help]\n", argv[0]);
                return EXIT_SUCCESS;
            case 'p':
                palabra_salida = optarg; 
                break;
            case 'g':
                flag_g = true;
                limite_gid = atoi(optarg); 
                break;
            case '?':
                fprintf(stderr, "Opción desconocida o falta argumento obligatorio.\n");
                printf("Uso: %s [-p/--psalida palabra] [-g limite_gid] [-h/--help]\n", argv[0]);
                return EXIT_FAILURE;
            default:
                abort();
        }
    }

    // ---------------------------------------------------------------------
    // FASE 3 Y 4: COLAS, FORK Y COREOGRAFÍA
    // ---------------------------------------------------------------------
    struct mq_attr attr;
    attr.mq_flags = 0;              
    attr.mq_maxmsg = 10;            
    attr.mq_msgsize = MAX_SIZE;     
    attr.mq_curmsgs = 0;            

    pid_t pid = fork();
    mqd_t mq_server;
    char grupo[100]; // Declaramos la variable UNA SOLA VEZ aquí

    switch(pid){
        case -1:
            perror("Error al crear el proceso hijo");
            exit(EXIT_FAILURE);
            break;
        
        case 0:
            // --- CÓDIGO DEL HIJO ---
            mq_server = mq_open(nombre_cola, O_CREAT | O_WRONLY, 0644, &attr);
            printf("[HIJO]: mi PID es %d y mi PPID es %d\n", getpid(), getppid());
            printf("[HIJO]: El descriptor de la cola es: %d\n", (int)mq_server);

            do {
                printf("> Escribe el nombre del grupo a buscar (escribir '%s' para parar): ", palabra_salida);
                scanf("%s", grupo); // <-- CORREGIDO: Ya no ponemos 'char' delante
                
                if (mq_send(mq_server, grupo, strlen(grupo) + 1, 0) == -1) {
                    perror("Error al enviar el mensaje a la cola");
                    exit(EXIT_FAILURE);
                }
            } while(strcmp(grupo, palabra_salida) != 0);
            
            mq_close(mq_server);
            exit(EXIT_SUCCESS); 
            break; // Solo necesitamos un break
       
        default:
            // --- CÓDIGO DEL PADRE ---
            mq_server = mq_open(nombre_cola, O_CREAT | O_RDONLY, 0644, &attr);
            printf("[PADRE]: mi PID es %d y el PID de mi hijo es %d\n", getpid(), pid);
            printf("[PADRE]: El descriptor de la cola es: %d\n", (int)mq_server);

            char buffer_padre[MAX_SIZE];
            do {
                ssize_t leidos = mq_receive(mq_server, buffer_padre, MAX_SIZE, NULL);
                if (leidos == -1) {
                    perror("Error al recibir");
                    break;
                }

                if (strcmp(buffer_padre, palabra_salida) == 0) {
                    break; // Salimos inmediatamente si manda la palabra de salida
                }

                printf("[PADRE]: Grupo a buscar: %s\n", buffer_padre);

                struct group *gr = getgrnam(buffer_padre);
                
                if (gr == NULL) {
                    struct passwd *pw = getpwnam(getenv("USER"));
                    gr = getgrgid(pw->pw_gid);
                }

                if (!flag_g || gr->gr_gid > limite_gid) {
                    printf("  Nombre del grupo: %s\n", gr->gr_name);
                    printf("  GID: %d\n", gr->gr_gid);
                    printf("  Miembros secundarios: ");
                    
                    if (gr->gr_mem[0] == NULL) {
                        printf("Ninguno\n");
                    } else {
                        for (int i = 0; gr->gr_mem[i] != NULL; i++) {
                            printf("%s ", gr->gr_mem[i]);
                        }
                        printf("\n");
                    }
                } 
                else {
                    printf("  Nombre del grupo: %s\n", gr->gr_name);
                }

            } while(strcmp(buffer_padre, palabra_salida) != 0);

            mq_close(mq_server);
            mq_unlink(nombre_cola); 
            wait(NULL);             

            break;
    }

    return 0;
}
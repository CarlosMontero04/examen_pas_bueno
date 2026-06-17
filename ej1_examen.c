#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <mqueue.h> // Para las colas mq_*
#include <errno.h>
#include <getopt.h>     // Para usar la función avanzada getopt_long()
#include <pwd.h>        // Para las funciones/estructuras de usuarios (struct passwd, getpwnam, getpwuid)
#include <grp.h>        // Para las funciones/estructuras de grupos (struct group, getgrnam, getgrgid)
#include <stdbool.h>    // Para poder usar variables booleanas (true / false)
#include <ctype.h>      // Para funciones de caracteres como isdigit() que usaremos más adelante



int main(int argc, char **argv) {
    char *usuario = getenv("USER");
    if (usuario == NULL) {
        perror("Error al obtener el usuario");
        exit(EXIT_FAILURE);
    }

    bool flag_help      = false;
    bool flag_user      = false;
    bool flag_active    = false;
    char *valor_user  = NULL;
    char nombre_cola[100];
    sprintf(nombre_cola,"/cola_examen_%s",getenv("USER"));
    
    static struct option long_options[] = {
        {"help",      no_argument,       NULL, 'h'},
        {0,      required_argument, NULL, 'g'}, 
        {"psalida",      required_argument, NULL, 'p'},
         
        {0, 0, 0, 0} // REQUISITO: El array siempre debe terminar con esta línea llena de ceros.
    };

    int c;
    while ((c = getopt_long(argc, argv, "hu:amg:s", long_options, NULL)) != -1) {
        
        switch (c) {
            case 'h':
                if (flag_help) {
                print_help();
                return EXIT_SUCCESS; 
            }
                break;
            case 'p':
                flag_user = true;
                valor_user = optarg; // Guardamos el nombre o UID introducido
                break;
            case 'g':
                flag_user = true;
                valor_user = optarg; // Guardamos el nombre o UID introducido
                break;
            
            case '?':
                // Si el usuario mete una opción que no está en la lista (ej: -x) 
                // o se olvida de poner el argumento obligatorio (ej: escribe solo -u).
                fprintf(stderr, "Opción desconocida '-%c' o falta argumento obligatorio.\n", optopt);
                flag_help = true; // Encendemos la ayuda para que luego se imprima el menú
                break;
            default:
                abort(); // Aborta el programa si ocurre un error catastrófico no contemplado
        }
    }



}
#include <stdio.h>      // Para operaciones de entrada/salida (printf, fprintf, perror)
#include <stdlib.h>     // Para utilidades generales (exit, atoi, getenv, abort)
#include <unistd.h>     // Para llamadas al sistema POSIX y variables externas de getopt (optarg, opterr, optind)
#include <getopt.h>     // Para usar la función avanzada getopt_long()
#include <pwd.h>        // Para las funciones/estructuras de usuarios (struct passwd, getpwnam, getpwuid)
#include <grp.h>        // Para las funciones/estructuras de grupos (struct group, getgrnam, getgrgid)
#include <stdbool.h>    // Para poder usar variables booleanas (true / false)
#include <ctype.h>      // Para funciones de caracteres como isdigit() que usaremos más adelante


// Función que imprime un menú de ayuda detallado
void print_help() {
    printf("Uso del programa: ./ej1 [opciones]\n");
    printf("Opciones:\n");
    printf("  -h, --help                     Imprimir esta ayuda\n");
    printf("  -u, --user (<nombre>|<uid>)    Informacion sobre el usuario\n");
    printf("  -a, --active                   Informacion sobre el usuario activo actual\n");
    printf("  -m, --maingroup                Ademas de info de usuario, imprimir la info de su grupo principal\n");
    printf("  -g, --group (<nombre>|<gid>)   Informacion sobre el grupo\n");
    printf("  -s, --allgroups                Muestra info de todos los grupos del sistema\n");
}


// Función auxiliar para saber si un texto contiene solo números (ej. para UID o GID)
bool es_numero(const char *cadena) {
    if (cadena == NULL || cadena[0] == '\0') return false;
    for (int i = 0; cadena[i] != '\0'; i++) {
        if (!isdigit(cadena[i])) return false; // Si hay una letra, no es un número
    }
    return true;
}

// Función que imprime la información de un grupo (struct group)
void imprimir_grupo(struct group *gr) {
    if (gr == NULL) {
        fprintf(stderr, "Error: No se pudo obtener la información del grupo.\n");
        return;
    }
    printf("\n--- INFO DEL GRUPO ---\n");
    printf("Nombre del grupo: %s\n", gr->gr_name);
    printf("GID: %d\n", gr->gr_gid);
    
    // Imprimir los miembros secundarios del grupo (es un array de cadenas)
    printf("Miembros secundarios: ");
    if (gr->gr_mem[0] == NULL) {
        printf("Ninguno\n");
    } else {
        // Recorremos el array hasta encontrar un puntero a NULL
        for (int i = 0; gr->gr_mem[i] != NULL; i++) {
            printf("%s ", gr->gr_mem[i]);
        }
        printf("\n");
    }
}

// Función que imprime la información de un usuario (struct passwd)
// Recibe también el booleano 'flag_maingroup' por si hay que imprimir también su grupo
void imprimir_usuario(struct passwd *pw, bool flag_maingroup) {
    if (pw == NULL) {
        fprintf(stderr, "Error: No se pudo obtener la información del usuario.\n");
        return;
    }
    
    printf("\n--- INFO DEL USUARIO ---\n");
    printf("Nombre real: %s\n", pw->pw_gecos);
    printf("Login: %s\n", pw->pw_name);
    printf("Password: %s\n", pw->pw_passwd);
    printf("UID: %d\n", pw->pw_uid);
    printf("Home: %s\n", pw->pw_dir);
    printf("Shell: %s\n", pw->pw_shell);
    printf("Número de grupo principal (GID): %d\n", pw->pw_gid);

    // Si el usuario usó la opción -m / --maingroup, buscamos e imprimimos el grupo
    if (flag_maingroup) {
        struct group *gr = getgrgid(pw->pw_gid); // Buscamos el grupo por su GID
        imprimir_grupo(gr);
    }
}

int main(int argc, char **argv) {
    
    // -------------------------------------------------------------------------
    // 1. DECLARACIÓN DE VARIABLES (FLAGS / BANDERAS)
    // -------------------------------------------------------------------------
    // Usamos booleanos para "recordar" qué opciones ha escrito el usuario.
    // Inicializamos todo a 'false' (apagado) para tener un estado limpio.
    bool flag_help      = false;
    bool flag_user      = false;
    bool flag_active    = false;
    bool flag_maingroup = false;
    bool flag_group     = false;
    bool flag_allgroups = false;

    // Cuando la opción requiere un texto extra (ej: -u juan), la función getopt_long 
    // lo guarda en una variable global llamada 'optarg'. Usaremos estos punteros 
    // para guardarnos ese texto y usarlo más tarde.
    char *valor_user  = NULL;
    char *valor_group = NULL;

    int c; // Aquí guardaremos el carácter que nos vaya devolviendo getopt_long()

    // -------------------------------------------------------------------------
    // 2. CONFIGURACIÓN DE LAS OPCIONES LARGAS
    // -------------------------------------------------------------------------
    // Esta estructura le enseña a getopt_long a relacionar la palabra larga con la letra corta.
    // Formato: {"nombre_largo", requiere_argumento?, puntero_a_flag, 'letra_corta'}
    static struct option long_options[] = {
        {"help",      no_argument,       NULL, 'h'},
        {"user",      required_argument, NULL, 'u'}, // required_argument: OBLIGA a poner un nombre o ID
        {"active",    no_argument,       NULL, 'a'},
        {"maingroup", no_argument,       NULL, 'm'},
        {"group",     required_argument, NULL, 'g'}, // required_argument: OBLIGA a poner un nombre o ID
        {"allgroups", no_argument,       NULL, 's'}, // El guion asocia --allgroups a la opción -s
        {0, 0, 0, 0} // REQUISITO: El array siempre debe terminar con esta línea llena de ceros.
    };

    // Deshabilitar la impresión de errores automáticos de getopt() para gestionarlos nosotros.
    // (Aparece comentado en tu fichero ejemplo-getoptlong.c, es buena práctica)
    opterr = 0;

    // -------------------------------------------------------------------------
    // 3. BUCLE DE PROCESADO DE LA LÍNEA DE COMANDOS
    // -------------------------------------------------------------------------
    // El tercer parámetro "hu:amg:s" define las opciones cortas válidas.
    // ¡IMPORTANTE!: Los dos puntos ':' detrás de la 'u' y la 'g' le indican a la función 
    // que esas letras necesitan un argumento obligatorio al lado (ej: -u 1000).
    while ((c = getopt_long(argc, argv, "hu:amg:s", long_options, NULL)) != -1) {
        
        switch (c) {
            case 'h':
                flag_help = true;
                break;
            case 'u':
                flag_user = true;
                valor_user = optarg; // Guardamos el nombre o UID introducido
                break;
            case 'a':
                flag_active = true;
                break;
            case 'm':
                flag_maingroup = true;
                break;
            case 'g':
                flag_group = true;
                valor_group = optarg; // Guardamos el nombre o GID introducido
                break;
            case 's':
                flag_allgroups = true;
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

    // simplemente ha "tomado nota" encendiendo los flags de lo que el usuario quiere hacer.
    // -------------------------------------------------------------------------
    // 2. COMPROBACIÓN DE COMBINACIONES VÁLIDAS (CONTROL DE ERRORES)
    // -------------------------------------------------------------------------

    // 1. La opción --help tiene prioridad absoluta. Si está encendida, mostramos
    //    la ayuda e ignoramos el resto de opciones. Usamos EXIT_SUCCESS (0) porque
    //    pedir ayuda no es un error de ejecución.
    if (flag_help) {
        print_help();
        return EXIT_SUCCESS; 
    }

    // 2. Comportamiento por defecto: Si no se pasan argumentos (solo está ./ej1),
    //    argc vale 1. El guion dice que equivale a usar --active y --maingroup juntos.
    if (argc == 1) {
        flag_active = true;
        flag_maingroup = true;
    } else {
        // A partir de aquí, evaluamos incompatibilidades usando nuestros flags (booleanos).
        
        // 3. --group no se puede mezclar con nada más. Si flag_group es true, 
        //    ninguno de los otros puede ser true.
        if (flag_group && (flag_user || flag_active || flag_maingroup || flag_allgroups)) {
            fprintf(stderr, "Error: La opción -g/--group no puede combinarse con otras opciones.\n");
            print_help();
            return EXIT_FAILURE; // EXIT_FAILURE (1) indica al S.O. que el programa falló
        }

        // 4. --allgroups tampoco se puede mezclar con nada más.
        if (flag_allgroups && (flag_user || flag_active || flag_maingroup || flag_group)) {
            fprintf(stderr, "Error: La opción -s/--allgroups no puede combinarse con otras opciones.\n");
            print_help();
            return EXIT_FAILURE;
        }

        // 5. --maingroup es un modificador. No tiene sentido usarlo solo, 
        //    obligatoriamente debe acompañar a --user o a --active.
        if (flag_maingroup && !flag_user && !flag_active) {
            fprintf(stderr, "Error: La opción -m/--maingroup debe acompañar a -u/--user o a -a/--active.\n");
            print_help();
            return EXIT_FAILURE;
        }

        // 6. No tiene sentido pedir buscar un usuario en concreto (-u) y 
        //    a la vez el usuario activo (-a). Son excluyentes.
        if (flag_user && flag_active) {
            fprintf(stderr, "Error: Las opciones -u/--user y -a/--active son incompatibles entre sí.\n");
            print_help();
            return EXIT_FAILURE;
        }
        
        // 7. CONTROL DE ARGUMENTOS SOBRANTES. 
        // optind es una variable mágica de <getopt.h>. Nos dice por qué índice de argv[] 
        // se ha quedado leyendo. Si optind es menor que argc, significa que el usuario 
        // ha escrito basura al final (por ejemplo: ./ej1 -u juan patata).
        if (optind < argc) {
            fprintf(stderr, "Error: Se han introducido argumentos no reconocidos al final.\n");
            print_help();
            return EXIT_FAILURE;
        }
    }

    // -------------------------------------------------------------------------
    // 3. EJECUCIÓN DE LA FUNCIONALIDAD
    // -------------------------------------------------------------------------

    // A. OPCIÓN --user
    if (flag_user) {
        struct passwd *pw = NULL;
        // Si lo que introdujo el usuario es todo números, usamos getpwuid()
        if (es_numero(valor_user)) {
            pw = getpwuid(atoi(valor_user)); // atoi convierte texto a número (int)
        } else {
            // Si es texto, usamos getpwnam()
            pw = getpwnam(valor_user);
        }

        if (pw == NULL) {
            fprintf(stderr, "Error: No existe el usuario '%s' en el sistema.\n", valor_user);
            return EXIT_FAILURE;
        }
        imprimir_usuario(pw, flag_maingroup);
    }

    // B. OPCIÓN --active
    else if (flag_active) {
        char *usuario_actual = getenv("USER"); // Obtiene el login del usuario que ejecuta el programa
        if (usuario_actual == NULL) {
            fprintf(stderr, "Error: No se pudo determinar el usuario activo.\n");
            return EXIT_FAILURE;
        }
        
        struct passwd *pw = getpwnam(usuario_actual);
        if (pw == NULL) {
            fprintf(stderr, "Error: No se encontró la información del usuario activo.\n");
            return EXIT_FAILURE;
        }
        imprimir_usuario(pw, flag_maingroup);
    }

    // C. OPCIÓN --group
    else if (flag_group) {
        struct group *gr = NULL;
        if (es_numero(valor_group)) {
            gr = getgrgid(atoi(valor_group));
        } else {
            gr = getgrnam(valor_group);
        }

        if (gr == NULL) {
            fprintf(stderr, "Error: No existe el grupo '%s' en el sistema.\n", valor_group);
            return EXIT_FAILURE;
        }
        imprimir_grupo(gr);
    }

    // D. OPCIÓN --allgroups
    else if (flag_allgroups) {
        // Para leer todos los grupos, recorremos la base de datos de grupos del sistema.
        // POSIX nos da la función setgrent() para abrirla, getgrent() para leer línea a línea,
        // y endgrent() para cerrarla.
        printf("\n--- TODOS LOS GRUPOS DEL SISTEMA ---\n");
        setgrent(); // Abre el archivo de grupos
        struct group *gr;
        while ((gr = getgrent()) != NULL) {
            imprimir_grupo(gr);
        }
        endgrent(); // Cierra el archivo
    }
    
    return 0; // Fin temporal del main
}

/*La función getopt() estándar solo sirve para opciones cortas de una letra.*/

/*¿Qué es optarg? Es una variable mágica que ya viene definida dentro de <unistd.h>. Tú no la declaras. Cuando getopt_long procesa algo como -u pepe, guarda la cadena "pepe" dentro de optarg. 
Nosotros lo copiamos a valor_user = optarg; para no perderlo.*/
/*¿Qué es optopt? Es otra variable mágica que guarda la letra de la opción que ha provocado un error 
(por ejemplo, si el usuario teclea -z, en optopt se guardará la 'z').*/
/*fprintf(stderr, ...): En POSIX, los mensajes de error no se imprimen con printf() normal (que va a stdout)*/


/* Miembros de passwd 
        pw_name     - nombre de login
        pw_passwd   - contrasenia
        pw_uid      - id de usuario
        pw_gid      - id de grupo
        pw_change   - passwd change time
        pw_class    - user access class
        pw_gecos    - nombre de usuario completo
        pw_dir      - directorio de login
        pw_shell    - shell del usuario
        pw_expire   - passwd expiration time
    */

     /* Miembros de group
        gr_name     - nombre del grupo
        gr_passwd   - contrasenia
        gr_gid      - id del grupo
        gr_mem      - lista de miembros del grupo
    */   

// Sobre getopt():
//   - optind: Indice del siguiente elemento ("-") a procesar del vector
//   argv[] (basicamente el que estoy procesando)
//   - optarg: recoge el valor del argumento obligario de una opcion. (si es una opt sin arg entonces es NULL)
//   - optopt: recoge el valor de la opcion ("-") cuando es desconocida
//   (?) o INCOMPLETA respecto a las opciones indicadas.

    //printf("aflag = %d, sflag = %d, hflag = %d, uvalue = %s, gvalue = %s, mflag = %d\n", aflag, sflag, hflag, uvalue, gvalue, mflag);

/*
1. PROCESADO DE LÍNEA DE COMANDOS (<getopt.h>)
   -------------------------------------------------------------------------
   * struct option: Estructura para definir opciones largas (--user).
     Campos: {"nombre", tiene_argumento, puntero_flag, 'letra_corta'}
     - no_argument: No lleva nada al lado (ej: --help).
     - required_argument: Obligatorio poner un valor al lado (ej: --user root).

   * getopt_long(argc, argv, "hu:amg:s", long_options, NULL):
     - El tercer parámetro "hu:amg:s" define las letras cortas.
     - Los dos puntos ':' indican que la letra anterior REQUIERE un argumento (ej: u: significa -u algo).
   
   * Variables globales de getopt (¡Mágicas, ya existen en <unistd.h>!):
     - optarg: Puntero (char *) al texto del argumento que puso el usuario (ej: si puso "-u root", optarg vale "root").
     - optopt: Guarda el carácter de la opción desconocida si hubo un error.
     - optind: Índice del siguiente argumento a procesar. Sirve para saber si hay "basura" al final del comando comprobando si (optind < argc).
     - opterr: Si la pones a 0 (opterr = 0), getopt no imprime sus errores feos por defecto, dejando que los gestiones tú.

   2. INFORMACIÓN DE USUARIOS (<pwd.h>)
   -------------------------------------------------------------------------
   * Funciones de búsqueda:
     - struct passwd *getpwnam(const char *name): Busca por nombre (texto).
     - struct passwd *getpwuid(uid_t uid): Busca por número de ID (entero).
       Ambas devuelven NULL si el usuario no existe.

   * struct passwd (Campos principales):
     - pw->pw_name   : Login / Username (char *)
     - pw->pw_passwd : Password encriptada (char *). Suele salir una 'x'
     - pw->pw_uid    : ID del usuario (uid_t / int)
     - pw->pw_gid    : ID de su GRUPO principal (gid_t / int)
     - pw->pw_gecos  : Nombre real / Nombre completo (char *)
     - pw->pw_dir    : Directorio Home (ej: /home/juan) (char *)
     - pw->pw_shell  : Intérprete de comandos (ej: /bin/bash) (char *)

   3. INFORMACIÓN DE GRUPOS (<grp.h>)
   -------------------------------------------------------------------------
   * Funciones de búsqueda:
     - struct group *getgrnam(const char *name): Busca por nombre (texto).
     - struct group *getgrgid(gid_t gid): Busca por número de ID (entero).
       Ambas devuelven NULL si el grupo no existe.

   * struct group (Campos principales):
     - gr->gr_name   : Nombre del grupo (char *)
     - gr->gr_passwd : Password del grupo (char *)
     - gr->gr_gid    : ID del grupo (gid_t / int)
     - gr->gr_mem    : Array de los nombres de los miembros secundarios (char **)

   * Iterar sobre miembros del grupo (gr->gr_mem):
     for (int i = 0; gr->gr_mem[i] != NULL; i++) {
         printf("%s ", gr->gr_mem[i]);
     }

   * Leer TODOS los grupos del sistema (Recorrer base de datos):
     setgrent(); // Abre la base de datos
     struct group *gr;
     while ((gr = getgrent()) != NULL) { // Lee uno por uno hasta llegar al final
         // usar 'gr'
     }
     endgrent(); // Cierra la base de datos

   4. VARIABLES DE ENTORNO (<stdlib.h>)
   -------------------------------------------------------------------------
   * getenv(const char *name): Devuelve el valor de una variable de entorno.
     - Ej: getenv("USER") devuelve el nombre del usuario activo (ej: "juan").
     - Ej: getenv("HOME") devuelve el directorio del usuario.
     (Devuelve NULL si la variable no existe).

   5. UTILIDADES DE C COMUNES EN ESTA PRÁCTICA
   -------------------------------------------------------------------------
   * isdigit(char c): (de <ctype.h>) Devuelve verdadero si el caracter es del '0' al '9'.
   * atoi(const char *str): (de <stdlib.h>) Convierte una cadena (ej: "1000") a entero (int 1000).
   * EXIT_SUCCESS (0) y EXIT_FAILURE (1): Códigos limpios para return o exit() (<stdlib.h>).
   * fprintf(stderr, "Mensaje"): Imprime un mensaje por el flujo de errores, NO por salida estándar.

   =========================================================================
*/
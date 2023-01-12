//LIBRERIAS IMPORTADAS

#include <stdio.h> 		//Libreria estandar
#include <stdlib.h>		//Libreria estandar
#include <sys/types.h>		//Libreria estandar
#include <unistd.h>		//Libreria pipes
#include <sys/wait.h>		//Libreria waits
#include <string.h>		//Libreria strings
#include <sys/stat.h>		//Libreria umask
#include <fcntl.h>		//Libreria ficheros
#include <errno.h>		//Libreria errno
#include <signal.h>		//Libreria señales
#include "parser.h"		//Libreria tline y tcommand

//DEFINICION DE CONSTANTES

#define SIZE 1024		//Tamaño de los arrays

//DEFINICION DE TADS AUXILIARES

struct hijoPosiJobs{
	int pidB;		//Pid
	char man[1024];		//String con el/los mandato/s
	int bloque;		//Numero de mandatos de la instruccion
};


//DEFINICION DE VARIABLES GLOBALES

pid_t pidA;			//Almacenamos pid del mandato ejecutandose en Foreground

//DEFINICION DE FUNCIONES AUXILIARES

int cd(tcommand * com);
int umaskC(tcommand * com);
void fg(tline * li, int* tope, int* indH, int* zeta, int* final, int est[], int est2[], struct hijoPosiJobs arj[], int hijosB[]);
void jobs(int g, int* tope, int* indH, int* zeta, int* final, int est[], int est2[], struct hijoPosiJobs arj[], int hijosB[]);
void manejador(int sig);


int main(void) {

	//VARIABLES
	
	pid_t pid;		//Pid del Padre-Hijos
	int status;
	
	int p_par[2];		//pipe-tuberia
	int p_impar[2];		//pipe-tuberia
	
	char buf[SIZE];		//buffer para leer el/los mandato/s
	
	tline * line;		//tline para leer la entrada
	
	int* child;					//Array de hijos en Foreground
	int childB[SIZE];				//Array de hijos en Background
	int estado[SIZE];				//Array para comprobar si se espero a la instruccion
	int estado2[SIZE];				//Array para comprobar si se mostro que la instruccion termino
	struct hijoPosiJobs arjobs[SIZE];		//Array para gestionar el Background
	
	int a,b;		//tamaños arrays de hijos en Foreground/Background
	
	int c,z,fin;		//indices para controlar instrucciones a/por ejecutar
	
	int auxEstado;		//indice para controlar el primer hijo que queda sin terminar en el array de hijos en Background
	
	int i,j;		//indices para recorrer bucles For
	
	int fd;			//Descriptor para abrir/leer los ficheros en caso de redireccion
	
	int error;		//Variable para comprobar si los metodos auxiliares han sido bien ejecutados
	
	
	//INICIALIZACION DE VARIABLES
	
	b = 0;
	c = 0;
	auxEstado=0;
	
	//SEÑALES

	signal(SIGINT,manejador);	//Trataremos SIGINT con el manejador
	
	umask(75);			//Establecemos los permisos por defecto en primer momento
			
	
	printf("msh> ");
	while (fgets(buf, 1024, stdin)) {
	
		//Volvemos a asignar a SIGINT el manejador
		signal(SIGINT,manejador);
		
		//Reiniciamos el array de hijos en Foreground
		a = 0;
		child = (int*)malloc(sizeof(int));
		
		//Abrimos las pipes
		pipe(p_par);
		pipe(p_impar);
		
		//Leemos la entrada
		line = tokenize(buf);
		
		if (line->ncommands > 0){ 			
			if(strcmp(line->commands[0].argv[0], "cd") == 0) {
				error = cd(line->commands);
				if(error != 0) printf("Error al intentar cambiar de directorio\n"); 	
			}
			else if(strcmp(line->commands[0].argv[0], "exit") == 0){
			
				//liberamos memoria
				free(child);
				
				exit(0);
			}
			else if(strcmp(line->commands[0].argv[0], "umask") == 0) {
				error = umaskC(line->commands);
				if(error != 0) printf("Error al intentar modificar la mascara\n");
			}
			else if(strcmp(line->commands[0].argv[0], "fg") == 0) fg(line,&c,&auxEstado,&z,&fin,estado,estado2,arjobs,childB);
			
			else if(strcmp(line->commands[0].argv[0], "jobs") == 0) jobs(1,&c,&auxEstado,&z,&fin,estado,estado2,arjobs,childB);
			
			else if(line->commands[0].filename == NULL) printf("%s: no se encuentra el mandato\n",line->commands[0].argv[0]);
			
			else {						
				if (line->background) {
					//Almacenamos la informacion de la instruccion en arjobs
					buf[strlen(buf) -1] = '\0';			//ultimo caracter es un \n molesto
					strcpy(arjobs[c].man,buf);
					arjobs[c].bloque = line -> ncommands;
					c++;			
				}
				
				for (i=0; i<line->ncommands; i++) {
				
					pid = fork();
					
					if(pid < 0) { //Error a la hora de hacer el fork
						fprintf(stderr, "Fallo el fork().\n%s\n", strerror(errno));
						exit(1);
					}else if(pid == 0){ //HIJO
					
						//Establecemos en primer momento que los hijos ignoren SIGINT
						signal(SIGINT,SIG_IGN);	
					
						if (i == 0){ //Primer hijo
							close(p_impar[0]);
							close(p_impar[1]);
							close(p_par[0]);
							if(line -> ncommands != 1) {
								dup2(p_par[1],1);
							}
							close(p_par[1]);
						} else { //Resto de hijos
							if(i%2 == 0) { //Hijos pares
								close(p_impar[1]);
								close(p_par[0]);
								dup2(p_impar[0],0);
								close(p_impar[0]);
								if(i != line->ncommands - 1){ 	//No estoy en el último hijo
									dup2(p_par[1],1);
								}
								close(p_par[1]);
							
							} else { //Hijos impares
								close(p_impar[0]);
								close(p_par[1]);
								dup2(p_par[0],0);
								close(p_par[0]);
								if(i != line->ncommands -1){ 	//No estoy en el último hijo
									dup2(p_impar[1],1);
								}
								close(p_impar[1]);
							
							}
						}

						if (line->redirect_output != NULL && i == line->ncommands -1){
							fd = creat(line->redirect_output, 0777);
							if(fd < 0) {
								printf("Error al abrir el archivo llamado %s\n",line->redirect_output);
							} else{
								dup2(fd,1);
							}
						}
						if (line->redirect_input != NULL && i == 0){
							fd = open(line->redirect_input, O_RDWR);
							if(fd < 0) {
								printf("Error al abrir el archivo llamado %s\n",line->redirect_input);
							} else {
								dup2(fd,0);
							}
						}
						if (line->redirect_error != NULL){
							fd = creat(line->redirect_error, 0777);
							if(fd < 0){
								printf("Error al abrir el archivo llamado %s\n",line->redirect_error);
							} else {
								dup2(fd,2);
							}
						}
						execvp(line->commands[i].filename, line->commands[i].argv);
						fprintf(stderr, "Se ha producido un error\n");
						exit(1);
					
					} else { //Padre
						if (line->background){
							//Mostramos Pid del mandato a Background
							if(i == line->ncommands - 1){
								printf("[%d]: %d\n",c,pid);
							}
							
							//Nos almacenamos el pid del mandato por su acaso hay un fg
							pidA=pid;
							
							//Añadimos la informacion en arjobs y childB						
							childB[b] = pid;
							arjobs[b].pidB = pid;
							b++;	
							
						} else {
							
							//Nos almacenamos el pid del mandato en Foreground por si hay SIGINT
							pidA=pid;
							
							//Almacenamos la informacion en child					
							child[a] = pid;
							a++;
							child = realloc(child, (a+1)*sizeof(int));	//aumentamos el array de hijos en Foreground				
						}
						
						if(i % 2 == 0) { //Pares (Reseteamos las pipes)
							close(p_impar[0]);
							close(p_impar[1]);
							pipe(p_impar);
						} else { //Impares (Reseteamos las pipes)
							close(p_par[0]);
							close(p_par[1]);
							pipe(p_par);
						}		
					}
				}
				
				//Cerramos pipes por seguridad
				close(p_par[0]);
				close(p_par[1]);
				close(p_impar[0]);
				close(p_impar[1]);
				
				//HIJOS EN BACKGROUND
				for(j = 0; j < b; j++){
					waitpid(childB[j],&status,WNOHANG);			
				} 
				
				//HIJOS EN FOREGROUND
				for(j = 0; j < a; j++){
					waitpid(child[j],&status,0);			
				}
				
				jobs(0,&c,&auxEstado,&z,&fin,estado,estado2,arjobs,childB); //para que salga el Hecho en una instruccion en Background al ejecutar otra en Foreground	
			}
		}
		printf("msh> ");
	}
	return 0;
}


//IMPLEMENTACION DE FUNCIONES AUXILIARES


int cd(tcommand * com){

	//VARIABLES
	char buffer[100];
	
	if(com -> argc <= 1){
		if(chdir(getenv("HOME")) != 0){
			fprintf(stderr, "No ha podido acceder a HOME\n");
			return 1;
		} else {
			printf("%s\n", getcwd(buffer, 100));
		}
	} else if(com -> argc == 2) {
		if(chdir(com -> argv[1]) != 0){
			fprintf(stderr, "No ha podido acceder al directorio del argumento\n");
			return 1;
		} else {
			printf("%s\n", getcwd(buffer, 100));
		}
	} else {
		fprintf(stderr, "Mas de un argumento, fin del programa\n");
		return 1;
	}
	return 0;
}


int umaskC(tcommand * com){
	
	//VARIABLES
	int mask;
	
	if(com -> argc == 2){
		mask = (int) strtol(com -> argv[1], NULL, 8);	//Base 8 porque necesitamos el numero en octal
		umask(mask);  					//cambia a la máscara nueva
		printf("La mascara es : %d\n",mask);
		return 0;
	}
	return 1;
}

void fg(tline * li, int* tope, int* indH, int* zeta, int* final, int est[], int est2[], struct hijoPosiJobs arj[], int hijosB[]){

	//VARIABLES
	int num;	//parametro que nos introduce el usuario
	int st;		//Analogo a Status
	int t;		//Controlar el ultimo mandato sin hacer
	int h,p;	//indices bucles For
	int auxFG;	//Indice para posicionarnos
	
	auxFG=*indH;
	
	if(li -> commands -> argc == 1) {
		if(*tope == 0) printf("msh: actual: no existe ese trabajo\n");	//Mensaje de error
		else{
			//Calculamos cual es la ultima instruccion sin hacer
			t = *tope-1;
			while(est[t]==1) t--;
			printf("%s\n",arj[t].man);
			
			//Nos posicionamos en el lugar correcto
			for(p = *zeta; p<t; p++){
				auxFG = auxFG + arj[p].bloque;
			}
			
			for(h = auxFG; h < auxFG+arj[t].bloque; h++){
				
				//Asignamos la señal al manejador y nos guardamos el pid
				signal(SIGINT,manejador);
				pidA=hijosB[h];
				
				waitpid(hijosB[h], &st, 0);
			}

			//Marcamos a hecho la instruccion
			est2[t] = 1;
			est[t] = 1;
		}
			
	} else if(li -> commands -> argc == 2){
		num = strtol(li -> commands->argv[1],NULL,10);
		num--;
		if(num < *tope){
			if(est[num]==1) printf("msh: actual: no existe ese trabajo\n");
			else {
				//Nos posicionamos en el lugar correcto
				for(p = *zeta; p < num; p++){
					auxFG = auxFG + arj[p].bloque;
				}
				
				//Imprimimos la instruccion a esperar
				printf("%s\n",arj[num].man);
				
				//Esperamos a todos los mandatos de la instruccion
				for(h = auxFG; h < auxFG+arj[num].bloque; h++){
				
					//Asignamos la señal al manejador y nos guardamos el pid
					signal(SIGINT,manejador);
					pidA=hijosB[h];
					
					waitpid(hijosB[h], &st, 0);
				}

				//Marcamos a hecho la instruccion
				est2[num] = 1;
				est[num] = 1;
			}			
		}
	} 
	
	//Llamamos a jobs para mostrar si es que mientras tanto alguna instruccion termino
	jobs(0,tope,indH,zeta,final,est,est2,arj,hijosB);
}

void jobs(int g, int* tope, int* indH, int* zeta, int* final, int est[], int est2[], struct hijoPosiJobs arj[], int hijosB[]){

	//VARIABLES
	int p,k,i,h;	//Indices bucles For
	int ok;		//Almacenar valor devuelto por wait
	int cont;	//Contar si todos los mandatos de la instruccion han acabado
	int y;		//Variable auxiliar para posicionarnos correctamente
	int st;		//Analogo de Status
	
	y=0;
	
	//Comprobamos si las instrucciones de la lista han acabado
	for(p = *zeta; p<*tope; p++){
		cont = 0;
		//Comprobamos si todos los mandatos de una instruccion han acabado
		for(k = y + *indH; k < arj[p].bloque + y + *indH && k<SIZE; k++){
			ok = waitpid(hijosB[k],&st,WNOHANG);
			if (ok != 0) cont++;
		}
		if (cont == arj[p].bloque) { 			//el proceso en bg ha terminado
			est[p] = 1;				//Marcamos a hecho la instruccion
		}		
		y = y + arj[p].bloque;	
	} 

	//Imprimimos las instrucciones realizadas y las que estan en ejecucion
	for(i = 0; i < *tope; i++){
		if(i%2 == 0){
			if(est[i] == 0 && g == 1){ 
				printf("[%d]+ Ejecutando 		%s\n",i+1,arj[i].man);
			}
	        	else if(est[i] == 1){
	              		if(est2[i] != 1) printf("[%d]+ Hecho 			%s\n",i+1,arj[i].man);             	
	        		est2[i] = 1;		//Ponemos a 1 para no volver a mostrar
	        	}
	        		
		} else {
			if(est[i] == 0 && g == 1){ 
				printf("[%d]- Ejecutando 		%s\n",i+1,arj[i].man);
			}
	        	else if(est[i] == 1){ 
	        		if(est2[i] != 1) printf("[%d]- Hecho 			%s\n",i+1,arj[i].man);
	        		est2[i] = 1;		//Ponemos a 1 para no volver a mostrar
	          	}
	        	
		}
	
	}
	
	//Actualizamos el valor de auxEstado(indH) y actualizamos la lista de jobs
	while(est[*zeta] == 1){
		*final=*final + 1;
		*indH = *indH + arj[*zeta].bloque;
		*zeta=*zeta+1;
	} 
	if(*final == *tope){
		for(h=0;h<*tope;h++) {
			est[h]=0;
			est2[h]=0;
		}
		*zeta=0;
		*tope = 0;
		*final=0;
	}
}


void manejador(int sig) {
	if(sig==SIGINT) {
		if(pidA!=0) kill(pidA,SIGKILL);		//Mandamos SIGKILL al mandato ejecutandose en Foreground (si lo hay)
	}
}









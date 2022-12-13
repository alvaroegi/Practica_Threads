#include <stdio.h>
#include <string.h>
#include <stdlib.h>		//Libreria estandar
#include <sys/types.h>		//Libreria estandar
#include <fcntl.h>		//Libreria ficheros
#include <errno.h>		//Libreria errno


#define SIZE 1024

int main(int argc, char *argv[]) {

	char in[SIZE];
	char out[SIZE];
	FILE *f;

	if(argc == 1) { //No nos meten ni fichero entrada ni salida
		strcpy(in,"entrada_vacunacion.txt");
		strcpy(out,"salida_vacunacion.txt");
	} else if(argc == 2) { //Nos meten fichero entrada
		strcpy(in,argv[1]);
		strcpy(out,"salida_vacunacion.txt");
	} else { //Nos meten fichero entrada y salida
		strcpy(in,argv[1]);
		strcpy(out,argv[2]);
	}

	f = fopen(in,"r");
	if(f == NULL) {
		 printf("Error al abrir el archivo llamado %s\n",in);
		return 1;
	}
	else {
		int dato;
		for(int i=0;i<9;i++) {
			fscanf(f,"%d",&dato);
			printf("%d\n",dato);
		}
		fclose(f);
	}
	return 0;
}

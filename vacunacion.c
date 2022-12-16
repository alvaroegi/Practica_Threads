#include <stdio.h>
#include <string.h>
#include <stdlib.h>		//Libreria estandar
#include <sys/types.h>		//Libreria estandar
#include <fcntl.h>		//Libreria ficheros
#include <errno.h>		//Libreria errno
#include <pthread.h>
#include <unistd.h>


#define SIZE 1024

struct paciente {
	int numPac;
	int centroP;
};

struct farmacia {
	int objetivo;
	int num;
};

int hab;
//int vacI;
int vacMin;
int vacMax;
int fabMin;
int fabMax;
int tReparto;
int tCita;
int tDesp;
int dato;

int vacunadosTanda;

int vacunasCentro[5];
int demanda[5];

struct farmacia f1,f2,f3;

int vacunados;
pthread_mutex_t v, mutexVac, mutexDema, mutexVacTanda;
pthread_cond_t hayVac;

void *crearVacunas(void* arg);
void *vacunarse(void* arg);

int main(int argc, char *argv[]) {

	char in[SIZE];
	char out[SIZE];
	int i;
	pthread_mutex_init(&v,NULL);
	pthread_mutex_init(&mutexVac,NULL);
	pthread_mutex_init(&mutexDema,NULL);
	pthread_mutex_init(&mutexVacTanda,NULL);
	pthread_cond_init(&hayVac,NULL);
	
	
	srand(time(NULL));
	
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
		printf("VACUNACION EN PANDEMIA: CONFIGURACION INICIAL\n");
		int dato;
		for(i=0;i<9;i++) {
			fscanf(f,"%d",&dato);
			if(i == 0) hab = dato;
			else if(i == 1) {
				for(int j=0;j<5;j++) vacunasCentro[j]=dato;
			}
			else if(i == 2) vacMin = dato;
			else if(i == 3) vacMax = dato;
			else if(i == 4) fabMin = dato;
			else if(i == 5) fabMax = dato;
			else if(i == 6) tReparto = dato;
			else if(i == 7) tCita = dato;
			else if(i == 8) tDesp = dato;
			//printf("%d\n",dato);
		}
		printf("Habitantes: %d\n",hab);
		printf("Centros de vacunacion: 5\n");
		printf("Fabricas: 3\n");
		printf("Vacunados por tanda: %d\n",(hab/10));
		printf("Vacunas iniciales en cada centro: %d\n",vacunasCentro[0]);
		printf("Vacunas totales en fabrica: %d\n",(hab/3));
		printf("Minimo numero de vacunas fabricadas en cada tanda: %d\n",vacMin);
		printf("Maximo numero de vacunas fabricadas en cada tanda: %d\n",vacMax);
		printf("Tiempo minimo de fabricacion de una tanda de vacunas: %d\n",fabMin);
		printf("Tiempo maximo de fabricacion de una tanda de vacunas: %d\n",fabMax);
		printf("Tiempo maximo que un habitante tarda en ver que esta citado para vacunarse: %d\n",tCita);
		printf("Tiempo maximo de desplazamiento del habitante al centro de vacunacion: %d\n",tDesp);
		printf("\n");
		printf("PROCESO DE VACUNACION\n");
		fclose(f);
	}
	
	f1.objetivo = hab/3 + hab%3;
	f1.num = 1;
	f2.objetivo = hab/3;
	f2.num = 2;
	f3.objetivo = hab/3;
	f3.num = 3;
	
	pthread_t farm1, farm2, farm3;
	pthread_create(&farm1, NULL, crearVacunas,&f1);
	pthread_create(&farm2, NULL, crearVacunas,&f2);
	pthread_create(&farm3, NULL, crearVacunas,&f3);
	
	
	for(int j = 0; j < 10; j++){
		vacunados = 0;
		for(int h = 0; h < hab/10; h++){
			pthread_mutex_lock(&v);
			struct paciente p;
			pthread_t paciente;
			p.centroP = rand()%5+1;
			p.numPac = j*120 + h + 1;
			
			pthread_create(&paciente, NULL, vacunarse, &p);
		}
		while(vacunadosTanda<(hab/10)) {
			//printf("vacunadosTanda: %d\n",vacunadosTanda);
		}
		pthread_mutex_lock(&mutexVacTanda);
		vacunadosTanda=0;
		pthread_mutex_unlock(&mutexVacTanda);
	}
	
	
	
	while(1);
}



void *crearVacunas(void* arg){
	struct farmacia farma = *(struct farmacia *)arg;
	int numVacCreadas = 0;
	while(numVacCreadas < farma.objetivo){
		int numVac = rand() % (vacMax-vacMin+1) + vacMin; //numero aleatorio de vacunas entre el vacMin y el vacMax
		if(numVac + numVacCreadas > farma.objetivo) numVac = farma.objetivo - numVacCreadas;
		int tiempoFab = rand() % (fabMax-fabMin+1) + fabMin; //numero aleatorio de tiempo de fabricacion de vacunas (entre fabMin y fabMax)
		sleep(tiempoFab);
		
		printf("Fabrica %d prepara %d vacunas\n",farma.num,numVac);
		numVacCreadas = numVacCreadas + numVac;
		
		int tiempoRep = rand() % (tReparto) + 1; //numero aleatrio de tiempo de reparto (entre 1 y tReparto)
		sleep(tiempoRep);
		
		
		//Gestionar entregas
		//Añadir print con el numero de vacunas enviadas al centro x
		
		//MUTEX
		for(int i=0;i<5;i++) {
			pthread_mutex_lock(&mutexDema);
			if(demanda[i]>0) {
				pthread_mutex_lock(&mutexVac);
				printf("entra aqui\n");
				if(demanda[i]<=numVac) {
					vacunasCentro[i]=demanda[i];
					numVac=numVac-demanda[i];
				} else {
					vacunasCentro[i]=numVac;
					numVac=0;
				}
				pthread_cond_broadcast(&hayVac);
				printf("Fabrica %d entrega %d vacunas en el centro %d\n",farma.num,vacunasCentro[i],i+1);
				pthread_mutex_unlock(&mutexVac);
			}
			pthread_mutex_unlock(&mutexDema);
		}
		int vacuRepaIgua = numVac/5;
		for(int i=0;i<5;i++) {
			pthread_mutex_lock(&mutexVac);
			vacunasCentro[i] = vacunasCentro[i] + vacuRepaIgua;
			if(i==0) { 
				if (vacuRepaIgua != 0 || (numVac%5 != 0)) {
					pthread_cond_broadcast(&hayVac);
					vacunasCentro[i] = vacunasCentro[i] + numVac%5;
					printf("Fabrica %d entrega %d vacunas en el centro %d\n",farma.num,(vacuRepaIgua+numVac%5),i+1);
				}
			}
			else if (vacuRepaIgua != 0) {
				printf("Fabrica %d entrega %d vacunas en el centro %d\n",farma.num,vacuRepaIgua,i+1);
				pthread_cond_broadcast(&hayVac);
			}
			pthread_mutex_unlock(&mutexVac);
		}
		
		//REPARTIR EQUITATIVAMENTE
		
	}
	printf("Fabrica %d ha fabricado todas sus vacunas\n",farma.num);
	pthread_exit(0);
	
}

void *vacunarse(void* arg){
	struct paciente paci = *(struct paciente *) arg;
	pthread_mutex_unlock(&v);
	sleep(rand()%tCita+1);
	
	if(paci.centroP == 1){
		printf("Habitante %d elige el centro %d para vacunarse\n",paci.numPac, paci.centroP);
		sleep(rand()%tDesp+1);
		pthread_mutex_lock(&mutexVac);
		while(vacunasCentro[0]==0) {
			pthread_mutex_lock(&mutexDema);
			demanda[0]++;
			printf("demanda[0]: %d\n",demanda[0]);
			pthread_mutex_unlock(&mutexDema);
			pthread_cond_wait(&hayVac,&mutexVac);
			printf("Dejo de esperar\n");
		}
		vacunasCentro[0]--;
		pthread_mutex_lock(&mutexDema);
		if(demanda[0]> 0) demanda[0]--;
		pthread_mutex_unlock(&mutexDema);
		pthread_mutex_lock(&mutexVacTanda);
		vacunadosTanda++;
		printf("vacunadosTanda: %d\n",vacunadosTanda);
		pthread_mutex_unlock(&mutexVacTanda);
		printf("Habitante %d vacunado en el centro %d\n",paci.numPac, paci.centroP);
		pthread_mutex_unlock(&mutexVac);
	} else if(paci.centroP == 2){
		printf("Habitante %d elige el centro %d para vacunarse\n",paci.numPac, paci.centroP);
		sleep(rand()%tDesp+1);
		pthread_mutex_lock(&mutexVac);
		while(vacunasCentro[1]==0) {
			pthread_mutex_lock(&mutexDema);
			demanda[1]++;
			printf("demanda[1]: %d\n",demanda[1]);
			pthread_mutex_unlock(&mutexDema);
			pthread_cond_wait(&hayVac,&mutexVac);
		}
		vacunasCentro[1]--;
		pthread_mutex_lock(&mutexDema);
		if(demanda[1]> 0) demanda[1]--;
		pthread_mutex_unlock(&mutexDema);
		pthread_mutex_lock(&mutexVacTanda);
		vacunadosTanda++;
		printf("vacunadosTanda: %d\n",vacunadosTanda);
		pthread_mutex_unlock(&mutexVacTanda);
		printf("Habitante %d vacunado en el centro %d\n",paci.numPac, paci.centroP);
		pthread_mutex_unlock(&mutexVac);
	} else if(paci.centroP == 3){
		printf("Habitante %d elige el centro %d para vacunarse\n",paci.numPac, paci.centroP);
		sleep(rand()%tDesp+1);
		pthread_mutex_lock(&mutexVac);
		while(vacunasCentro[2]==0) {
			pthread_mutex_lock(&mutexDema);
			demanda[2]++;
			printf("demanda[2]: %d\n",demanda[2]);
			pthread_mutex_unlock(&mutexDema);
			pthread_cond_wait(&hayVac,&mutexVac);
		}
		vacunasCentro[2]--;
		pthread_mutex_lock(&mutexDema);
		if(demanda[2]> 0)demanda[2]--;
		pthread_mutex_unlock(&mutexDema);
		pthread_mutex_lock(&mutexVacTanda);
		vacunadosTanda++;
		printf("vacunadosTanda: %d\n",vacunadosTanda);
		pthread_mutex_unlock(&mutexVacTanda);
		printf("Habitante %d vacunado en el centro %d\n",paci.numPac, paci.centroP);
		pthread_mutex_unlock(&mutexVac);
	} else if(paci.centroP == 4){
		printf("Habitante %d elige el centro %d para vacunarse\n",paci.numPac, paci.centroP);
		sleep(rand()%tDesp+1);
		pthread_mutex_lock(&mutexVac);
		while(vacunasCentro[3]==0) {
			pthread_mutex_lock(&mutexDema);
			demanda[3]++;
			printf("demanda[3]: %d\n",demanda[3]);
			pthread_mutex_unlock(&mutexDema);
			pthread_cond_wait(&hayVac,&mutexVac);
		}
		vacunasCentro[3]--;
		pthread_mutex_lock(&mutexDema);
		if(demanda[3]> 0)demanda[3]--;
		pthread_mutex_unlock(&mutexDema);
		pthread_mutex_lock(&mutexVacTanda);
		vacunadosTanda++;
		printf("vacunadosTanda: %d\n",vacunadosTanda);
		pthread_mutex_unlock(&mutexVacTanda);
		printf("Habitante %d vacunado en el centro %d\n",paci.numPac, paci.centroP);
		pthread_mutex_unlock(&mutexVac);
	} else if(paci.centroP == 5){
		printf("Habitante %d elige el centro %d para vacunarse\n",paci.numPac, paci.centroP);
		sleep(rand()%tDesp+1);
		pthread_mutex_lock(&mutexVac);
		while(vacunasCentro[4]==0) {
			pthread_mutex_lock(&mutexDema);
			demanda[4]++;
			printf("demanda[4]: %d\n",demanda[4]);
			pthread_mutex_unlock(&mutexDema);
			pthread_cond_wait(&hayVac,&mutexVac);
		}
		vacunasCentro[4]--;
		pthread_mutex_lock(&mutexDema);
		if(demanda[4]> 0)demanda[4]--;
		pthread_mutex_unlock(&mutexDema);
		pthread_mutex_lock(&mutexVacTanda);
		vacunadosTanda++;
		printf("vacunadosTanda: %d\n",vacunadosTanda);
		pthread_mutex_unlock(&mutexVacTanda);
		printf("Habitante %d vacunado en el centro %d\n",paci.numPac, paci.centroP);
		pthread_mutex_unlock(&mutexVac);
	}

	
	pthread_exit(0);

}

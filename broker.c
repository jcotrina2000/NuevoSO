#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <pthread.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <signal.h>
#include <semaphore.h>
#include "hashmap.h"

#define SERVER_PORT 54321
#define BUFFER_SIZE 1024
#define MSG_PART 5 // Mensajes por particion cuando no se recibe la clave

typedef struct{
	char *topico;
	int suscriptor_fd;
	int clave;
} SuscriptorHilo;

void desconectarPorSenial(int signal);
int obtenerTipo(char identificacion[], char **clienteIdentificacion);
void *handleConsumidor(void* arg);
void *handleProductor(void *arg);

int serverSocket;
HashMap nuevoMapa;
sem_t hashmap_sem;

int main(void) {

	signal(SIGINT, desconectarPorSenial);

	struct sockaddr_in serverAddress;

	serverSocket = socket(AF_INET, SOCK_STREAM, 0);
	if (serverSocket == -1) {
		perror("Creacion de socket");
		exit(EXIT_FAILURE);
  	}

  	serverAddress.sin_family = AF_INET;
  	serverAddress.sin_addr.s_addr = INADDR_ANY;
  	serverAddress.sin_port = htons(SERVER_PORT);

  	if (bind(serverSocket, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) < 0) {
    		perror("Bind Fallo");
    		exit(EXIT_FAILURE);
  	}

  	if (listen(serverSocket, 10) < 0) {
    		perror("Listen fallo");
    		exit(EXIT_FAILURE);
  	}

  	sem_init(&hashmap_sem, 0, 1);
  	iniciarMap(&nuevoMapa);

  	while (1) {

    		struct sockaddr_in clientAddress;
    		socklen_t clientAddressLength = sizeof(clientAddress);

    		int clientSocket = accept(serverSocket, (struct sockaddr*)&clientAddress, &clientAddressLength);
	    	if (clientSocket == -1) {
	      		perror("Aceptar nuevo cliente");
	      		exit(EXIT_FAILURE);
	    	}

	    	int client_port = ntohs(clientAddress.sin_port);
	    	char client_ip[INET_ADDRSTRLEN];
	    	inet_ntop(AF_INET, &(clientAddress.sin_addr), client_ip, INET_ADDRSTRLEN);

	    	printf("Nuevo cliente conectado: %s:%d\n", client_ip, client_port);
	    	fflush(stdout);

	    	char bufferIdentificacion[BUFFER_SIZE];
	    	if(recv(clientSocket, bufferIdentificacion, BUFFER_SIZE, 0) == -1){
	      		perror("Error al leer desde el nuevo cliente");
	      		close(clientSocket);
	      		continue;
	    	}

	    	bufferIdentificacion[strlen(bufferIdentificacion)] = '\0';

	    	char *clienteTopico;
	    	int tipoDeCliente = obtenerTipo(bufferIdentificacion, &clienteTopico);

	   	 if(tipoDeCliente == -1){
	      		fprintf(stderr, "TIPO DE CLIENTE DESCONOCIDO\n");
	      		close(clientSocket);
	      		continue;
	    	}

	    	char *topico = strtok(clienteTopico, "|");
	    	int clave = atoi(strtok(NULL, "|"));

	    	printf("Identificacion del cliente: %s (%s)\n", clienteTopico, tipoDeCliente == 1? "Productor": "Consumidor");
	    	fflush(stdout);
	    	printf("Clave del cliente: %d\n", clave);
	    	fflush(stdout);

	    	if(tipoDeCliente == 1){

	      		pthread_t threadID_g;

	      		SuscriptorHilo *threadArg = (SuscriptorHilo *)malloc(sizeof(SuscriptorHilo));
	      		if(threadArg != NULL){
				threadArg->suscriptor_fd = clientSocket;
				threadArg->topico = strdup(topico);
				threadArg->clave = clave;
	      		}

	      		if(pthread_create(&threadID_g, NULL, handleProductor, (void *)threadArg) != 0){
				perror("Creacion de hilo para gateway");
				close(clientSocket);
				continue;
	      		}

	    	} else if (tipoDeCliente == 0){

	      		pthread_t threadID_s;

	      		SuscriptorHilo *threadArg = (SuscriptorHilo *)malloc(sizeof(SuscriptorHilo));
	      		if(threadArg != NULL){
				threadArg->suscriptor_fd = clientSocket;
				threadArg->topico = strdup(topico);
				threadArg->clave = clave;
	      		}

	      		if(pthread_create(&threadID_s, NULL, handleConsumidor, (void *)threadArg) != 0){
				perror("Creacion de hilo para consumidor");
				close(clientSocket);
				continue;
	      		}

	    	}

	}

  	close(serverSocket);

  	return 0;
}

int obtenerTipo(char identificacion[], char **clienteIdentificacion){

	char *slice = strtok(identificacion, ":");
	if(!strcmp(slice, "productor")){
		*clienteIdentificacion = strtok(NULL, ":");
		return 1;

	}else if(!strcmp(slice, "consumidor")){
		*clienteIdentificacion = strtok(NULL, ":");
		return 0;

	}

	return -1;
}

void *handleConsumidor(void* arg){

	if(arg == NULL){
		fprintf(stderr, "No se puede atender al cliente, no se encuentra su informacion\n");
		pthread_exit(NULL);
	}

	if(pthread_detach(pthread_self()) != 0){
		perror("Detach hilo");
	}

	  SuscriptorHilo *dataHilo = (SuscriptorHilo *)arg;

	  int index = -1;

	if(strchr(dataHilo->topico, '#') != NULL){

		sem_wait(&hashmap_sem);
		if ((index = insertar_wildcard(&nuevoMapa, dataHilo->topico, dataHilo->suscriptor_fd, dataHilo->clave)) != -1) {
			printf("Nuevo consumidor agregado, indice en la tabla %d\n", index);
			fflush(stdout);
		} else {
		      fprintf(stderr, "No se pudo agregar el consumidor a la tabla\n");
		      close(dataHilo->suscriptor_fd);
   		}
    		sem_post(&hashmap_sem);

  	} else {

		sem_wait(&hashmap_sem);
		if ((index = insertar(&nuevoMapa, dataHilo->topico, dataHilo->suscriptor_fd, dataHilo->clave)) != -1) {
			printf("Nuevo consumidor agregado, indice en la tabla %d\n", index);
			fflush(stdout);
		} else {
			fprintf(stderr, "No se pudo agregar el consumidor a la tabla\n");
			close(dataHilo->suscriptor_fd);
		}
		sem_post(&hashmap_sem);

	}

  	free(arg);

  	pthread_exit(NULL);

}

void *handleProductor(void *arg){

	if(arg == NULL){
		fprintf(stderr, "No se puede atender al cliente, no se encuentra su informacion\n");
		pthread_exit(NULL);
	}

	if(pthread_detach(pthread_self()) != 0){
		perror("Detach hilo");
	}

	SuscriptorHilo *dataHilo = (SuscriptorHilo *)arg;

	char buffer[BUFFER_SIZE];

	int counter = 0;
	int current_partition = 0;
	ssize_t recv_size = 0;
	while((recv_size = recv(dataHilo->suscriptor_fd, buffer, BUFFER_SIZE, 0)) > 0){

		sem_wait(&hashmap_sem);
		HashValue *estructura_subs = buscar(&nuevoMapa, dataHilo->topico);
		sem_post(&hashmap_sem);

		if(estructura_subs == NULL){
			fprintf(stderr, "Topico no encontrado, no hay consumidores para %s\n", dataHilo->topico);
			continue;
		}

		if(dataHilo->clave == 0){

			for(size_t i = 0; i < (size_t)estructura_subs->p1.size; i++){
				if(send(estructura_subs->p1.subs[i], buffer, strlen(buffer), 0) <= 0){
				  	perror("Envio del mensaje productor - consumidor");
				}
			}

		} else if(dataHilo->clave == 1){

			for(size_t i = 0; i < (size_t)estructura_subs->p2.size; i++){
				if(send(estructura_subs->p2.subs[i], buffer, strlen(buffer), 0) <= 0){
		  			perror("Envio del mensaje productor - consumidor");
				}
			}

		} else if (dataHilo->clave == -1){

			if(counter % MSG_PART == 0){
				current_partition = (current_partition == 0)? 1: 0;
			}

			if(current_partition == 0){

				for(size_t i = 0; i < (size_t)estructura_subs->p1.size; i++){
					if(send(estructura_subs->p1.subs[i], buffer, strlen(buffer), 0) <= 0){
						perror("Envio del mensaje productor - consumidor");
					}
				}

			} else if(current_partition == 1){

				for(size_t i = 0; i < (size_t)estructura_subs->p2.size; i++){
					if(send(estructura_subs->p2.subs[i], buffer, strlen(buffer), 0) <= 0){
						perror("Envio del mensaje productor - consumidor");
					}
				}

			}
		}

		memset(buffer, 0, BUFFER_SIZE);
		counter++;
	}

	close(dataHilo->suscriptor_fd);
	free(dataHilo->topico);

	free(arg);

	pthread_exit(NULL);

}

void desconectarPorSenial(int signal) {

	(void)signal;

	destruir_mapa(&nuevoMapa);
	close(serverSocket);
	sem_destroy(&hashmap_sem);

	exit(EXIT_SUCCESS);

}

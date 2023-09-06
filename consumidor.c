#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define MAX_BUFFER_SIZE 1024
#define SERVER_IP "192.168.2.2" // Dirección IP de server2
#define SERVER_PORT 54321 // Puerto de conexión de server2

int clientSocket;

int main(int argc, char *argv[]){
  
	if(argc != 3){
		fprintf(stderr, "Uso del programa: %s <topico> <clave>\n", argv[0]);
		exit(EXIT_FAILURE);
	}

	int clave = atoi(argv[2]);
	if(clave < -1){ // -1 para indicar que se suscribe a cualquier particion
		fprintf(stderr, "La clave debe ser un numero positivo\n");
		exit(EXIT_FAILURE);
	}

	struct sockaddr_in server_addr;
	char buffer[MAX_BUFFER_SIZE];

	clientSocket = socket(AF_INET, SOCK_STREAM, 0);
	if (clientSocket == -1) {
		perror("Error al crear el socket del cliente hacia broker");
		exit(EXIT_FAILURE);
	}
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(SERVER_PORT);
  	server_addr.sin_addr.s_addr = inet_addr(SERVER_IP);

	if(connect(clientSocket, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
		perror("Error al conectar con server2");
		close(clientSocket);
		exit(EXIT_FAILURE);
	}

	char *identificacion = "consumidor:";
	snprintf(buffer, strlen(identificacion) + strlen(argv[1]) + strlen(argv[2]) + 2, "%s%s|%s", identificacion, argv[1], argv[2]);

	if(send(clientSocket, buffer, MAX_BUFFER_SIZE, 0) < 0){
		fprintf(stderr, "No se puede escribir en el broker\n");
		close(clientSocket);
		exit(EXIT_FAILURE);
	} 

	printf("\nConectado al broker.\n");
	fflush(stdout);
	printf("Formato de mensajes recibidos= topico: valor\n");
	fflush(stdout);
  
  	memset(buffer, 0, MAX_BUFFER_SIZE);

	while (1) {

		ssize_t num_bytes = recv(clientSocket, buffer, sizeof(buffer), 0);

		if (num_bytes > 0) {

			printf("Mensaje recibido: %s: %s\n", argv[1], buffer);
			fflush(stdout);

		} else if (num_bytes == 0) {

			printf("Servidor desconectado.\n");
			fflush(stdout);
			break;

		} else {

			perror("Error al recibir mensaje del broker");
			break;

		}

		memset(buffer, 0, MAX_BUFFER_SIZE);
	}

	close(clientSocket);

	return 0;

}

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <paths.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <signal.h>
#include <unistd.h>
#include <semaphore.h>

#define BUFFER_SIZE 1024
#define SERVER_IP "192.168.2.2"
#define SERVER_PORT 54321
#define TIME 5

#define MENSAJE_DEAFULT "Valor de metrica"

int server_fd, client_socket;

void desconectarPorSenial(int signal){

	(void)signal;

	printf("\nDesconectando el servidor");
	fflush(stdout);
	close(server_fd);
	printf("\n[!!] Servidor desconectado");
	fflush(stdout);

	exit(0);

}

int main(int argc, char *argv[]){

	if (argc != 3) {
		fprintf(stderr, "Uso del programa: %s <topico> <clave>\n", argv[0]);
		return EXIT_FAILURE;
	}

	int clave = atoi(argv[2]);
	if(clave < -1){ // -1 para indicar que se suscribe a cualquier particion
		fprintf(stderr, "La clave debe ser un numero positivo\n");
		exit(EXIT_FAILURE);
	}

	client_socket = socket(AF_INET, SOCK_STREAM, 0);
	if (client_socket == -1) {
		perror("Error al crear el socket del cliente hacia el broker");
		exit(EXIT_FAILURE);
	}
	printf("Crear socket cliente (exitoso)\n");
	fflush(stdout);


	struct sockaddr_in server_addr;
	memset(&server_addr, 0, sizeof(server_addr));

	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = inet_addr(SERVER_IP);
	server_addr.sin_port = htons(SERVER_PORT);

	if(connect(client_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1) {
		perror("Error al conectar con server2");
		close(client_socket);
		exit(EXIT_FAILURE);
	}
	printf("Connect cliente (exitoso)\n");
	fflush(stdout);

	char buffer[BUFFER_SIZE];
	char *identificacion = "productor:";
	snprintf(buffer, strlen(identificacion) + strlen(argv[1]) + strlen(argv[2]) + 2, "%s%s|%s", identificacion, argv[1], argv[2]);

	if(send(client_socket, buffer, strlen(buffer) + 1, 0) == -1){
		perror("Error al enviar identificacion al broker");
		close(client_socket);
		exit(EXIT_FAILURE);
	}

	while (1) {

		memset(buffer, 0, BUFFER_SIZE);

		sleep(TIME);

		int p[2];
		pipe(p);

		if(fork() == 0){

			close(p[0]);
			dup2(p[1], STDOUT_FILENO);

			if(strstr(argv[1], "cpu") != NULL){
				execl(_PATH_BSHELL, _PATH_BSHELL, "-c", "./scripts/cpu_usage", NULL);
			}else if(strstr(argv[1], "memory") != NULL){
				execl(_PATH_BSHELL, _PATH_BSHELL, "-c", "./scripts/memory_usage", NULL);
			}else{
				fprintf(stderr, "Metrica desconocida\n");
				exit(EXIT_FAILURE);
			}

		}

		int status;
		wait(&status);

		close(p[1]);

		if(status != 0){
			if(send(client_socket, MENSAJE_DEAFULT, strlen(MENSAJE_DEAFULT) + 1, 0) == -1) {
				perror("Error al enviar el mensaje al broker");
				break;
			}
		} else {

			if(read(p[0], buffer, BUFFER_SIZE) <= 0){
				perror("Leer mensaje desde el pipe");
				break;
			}

			if(send(client_socket, buffer, strlen(buffer) + 1, 0) == -1) {
				perror("Error al enviar el mensaje al broker");
				break;
			}

			printf("Enviando mensaje: %s...\n", buffer);
			fflush(stdout);
		}
		close(p[0]);
	}

	close(client_socket);

	return 0;
}

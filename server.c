#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdbool.h>
#include <pthread.h>
#include "chat.pb-c.h"

#define MAX_CLIENTS 10
#define BUFFER_SIZE 1024
#define PORT 8080

typedef struct
{
    int client_fd;
    char username[BUFFER_SIZE];
    int user_state;
    pthread_t thread_id;
} client_t;

client_t clients[MAX_CLIENTS];
int num_clients = 0;

void *client_handler(void *arg)
{
    client_t *client = (client_t *)arg;
    char buffer[BUFFER_SIZE];
    int bytes_received, bytes_sent;
    int client_fd = client->client_fd;

    ChatSistOS__Answer answer = CHAT_SIST_OS__ANSWER__INIT;
    int answer_size;

    // Recibimos mensajes del cliente
    while ((bytes_received = recv(client_fd, buffer, BUFFER_SIZE, 0)) > 0)
    {
        // Procesamos el mensaje
        ChatSistOS__UserOption *user_option = chat_sist_os__user_option__unpack(NULL, bytes_received, buffer);
        if (user_option == NULL)
        {
            perror("Error al decodificar el mensaje");
            break;
        }

        // Manejamos la opción
        switch (user_option->op)
        {
        case 1:
            // Crear nuevo usuario
            strcpy(client->username, user_option->createuser->username);

            // Creamos el mensaje de respuesta de usuario
            answer.response_message = "Usuario registrado exitosamente.";
            answer_size = chat_sist_os__answer__get_packed_size(&answer);
            chat_sist_os__answer__pack(&answer, buffer);

            // Enviamos el mensaje al cliente
            bytes_sent = send(client_fd, buffer, answer_size, 0);
            if (bytes_sent < 0)
            {
                perror("Error al enviar el mensaje al cliente");
            }

            printf("Nuevo usuario creado: %s\n", client->username);
            break;
        case 2:
            // Ver usuarios conectados
            printf("Usuario %s solicitó lista de usuarios conectados\n", client->username);
            printf("Lista de usuarios conectados:\n");
            for (int i = 0; i < num_clients; i++)
            {
                if (clients[i].thread_id != 0)
                {
                    printf("- %s\n", clients[i].username);
                }
            }

            // Creamos el mensaje de respuesta de usuario
            answer.response_message = "Usuarios conectados mostrados.";
            answer_size = chat_sist_os__answer__get_packed_size(&answer);
            chat_sist_os__answer__pack(&answer, buffer);

            // Enviamos el mensaje al cliente
            bytes_sent = send(client_fd, buffer, answer_size, 0);
            if (bytes_sent < 0)
            {
                perror("Error al enviar el mensaje al cliente");
            }

            break;
        case 3:
            if (user_option->status != NULL && strlen(user_option->status->user_name) > 0){
                for (int i = 0; i < num_clients; i++)
                {
                    if (strcmp(clients[i].username, user_option->status->user_name) == 0)
                    {
                        // Mostrar informacion
                        char estado[20] = "";
                        if ((clients[i].user_state)==1){
                                                        strcpy(estado, "Disponible");
                        } else if ((clients[i].user_state)==2){
                            strcpy(estado, "Ocupado");
                        } else if ((clients[i].user_state)==3){
                            strcpy(estado, "Ausente");
                        }

                        printf("Información del usuario solicitado:\n");
                        printf("Nombre de usuario: %s\n", clients[i].username);
                        printf("Estado: %s\n", estado);

                        // Creamos el mensaje de respuesta de usuario
                        answer.response_message = "Información del usuario mostrada.";
                        answer_size = chat_sist_os__answer__get_packed_size(&answer);
                        chat_sist_os__answer__pack(&answer, buffer);

                        // Enviamos el mensaje al cliente
                        bytes_sent = send(client_fd, buffer, answer_size, 0);
                        if (bytes_sent < 0)
                        {
                            perror("Error al enviar el mensaje al cliente");
                        }
                        break;
                    }
                }
            } else {
                // Cambiar estado del usuario
                client->user_state = user_option->status->status;

                // Creamos el mensaje de respuesta de usuario
                answer.response_message = "Estado cambiado.";
                answer_size = chat_sist_os__answer__get_packed_size(&answer);
                chat_sist_os__answer__pack(&answer, buffer);

                // Enviamos el mensaje al cliente
                bytes_sent = send(client_fd, buffer, answer_size, 0);
                if (bytes_sent < 0)
                {
                    perror("Error al enviar el mensaje al cliente");
                }
            }
            break;
        }

        chat_sist_os__user_option__free_unpacked(user_option, NULL);
    }

    printf("Cliente desconectado: %s\n", client->username);
    close(client_fd);
    client->thread_id = 0;

    return NULL;
}

int main()
{
    int server_fd, new_client_fd;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_addr_size = sizeof(client_addr);
    pthread_t thread_id;

    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0)
    {
        perror("Error al crear el socket del servidor");
        exit(EXIT_FAILURE);
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    if (bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        perror("Error en el bind");
        exit(EXIT_FAILURE);
    }

    if (listen(server_fd, 10) < 0)
    {
        perror("Error en el listen");
        exit(EXIT_FAILURE);
    }

    printf("Servidor iniciado. Esperando clientes...\n");

    while ((new_client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &client_addr_size)))
    {
        printf("Nuevo cliente conectado\n");

        if (num_clients >= MAX_CLIENTS)
        {
            printf("Se alcanzó el máximo número de clientes.\n");
            close(new_client_fd);
            continue;
        }

        client_t *client = &clients[num_clients++];
        client->client_fd = new_client_fd;

        pthread_create(&thread_id, NULL, client_handler, (void *)client);
        client->thread_id = thread_id;
    }

    close(server_fd);

    return 0;
}


#include <arpa/inet.h>
#include <netinet/in.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <stdbool.h>

#include "chat.pb-c.h" // Incluir el archivo generado por protoc para Protocol Buffers

#define PORT 8080
#define BUFFER_SIZE 1024

void *receive_handler(void *arg)
{
    int client_fd = *((int *)arg);
    char buffer[BUFFER_SIZE];
    int bytes_received;

    while ((bytes_received = recv(client_fd, buffer, BUFFER_SIZE, 0)) > 0)
    {
        ChatSistOS__Answer *answer = chat_sist_os__answer__unpack(NULL, bytes_received, buffer);
        if (answer == NULL)
        {
            perror("Error al decodificar el mensaje");
            break;
        }

        switch (answer->op)
        {
        case 1: // Crear nuevo usuario
            if (answer->response_status_code == 400) {
                printf("Error: %s\n", answer->response_message);
                printf("Por favor, intente con un nombre de usuario diferente.\n");
            } else {
                printf("%s\n", answer->response_message);
            }
            break;
        case 2: // Ver usuarios conectados
            printf("%s\n", answer->response_message);
            break;
        case 3: // Cambiar estado de usuario
            printf("%s\n", answer->response_message);
            break;
        case 4: // Recibir mensaje
            printf("[%s]: %s\n", answer->message->message_sender, answer->message->message_content);
            break;
        }

        chat_sist_os__answer__free_unpacked(answer, NULL);
    }

    return NULL;
}

int main(int argc, char *argv[])
{
    if (argc != 3)
    {
        fprintf(stderr, "Uso: %s <IPdelservidor> <puertodelservidor>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    int client_fd;
    struct sockaddr_in server_addr;
    pthread_t thread_id;

    client_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (client_fd < 0)
    {
        perror("Error al crear el socket del cliente");
        exit(EXIT_FAILURE);
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr(argv[1]);
    server_addr.sin_port = htons(atoi(argv[2]));

    if (connect(client_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        perror("Error en la conexión");
        exit(EXIT_FAILURE);
    }

    printf("Cliente conectado al servidor.\n");

    pthread_create(&thread_id, NULL, receive_handler, (void *)&client_fd);

    char input[256];
    ChatSistOS__UserOption user_option;
    char buffer[BUFFER_SIZE];
    int bytes_sent;
    int op;

    while (1)
    {
        char user_name[256];
        printf("\n-------------------------------------\n");
        printf("1. Crear nuevo usuario\n");
        printf("2. Ver usuarios \n");
        printf("3. Cambiar estado de usuario\n");
        printf("4. Enviar mensaje\n");
        printf("5. Salir\n");
        printf("Ingrese la opción que desea realizar: ");
        scanf("%d", &op);

        chat_sist_os__user_option__init(&user_option);

        if (op == 1) {
            user_option.op = 1;
            ChatSistOS__NewUser new_user = CHAT_SIST_OS__NEW_USER__INIT;
            printf("Ingrese el nombre de usuario: ");
            scanf("%s", input);
            new_user.username = input;
            user_option.createuser = &new_user;

            // Agregar esta línea para almacenar el nombre de usuario ingresado
            strncpy(user_name, input, sizeof(user_name));
        }
        else if (op == 2)
        
        {
            
            
            user_option.op = 2;
            ChatSistOS__UserList user_list = CHAT_SIST_OS__USER_LIST__INIT;
            printf("¿Desea ver la lista de todos los usuarios o un usuario específico? (1: todos, 2: específico): ");
            int list_option;
            scanf("%d", &list_option);

            if (list_option == 1)
            {
                user_list.list = true;
            }
            else if (list_option == 2)
            {
                user_list.list = false;
                printf("Ingrese el nombre de usuario para obtener información: ");
                scanf("%s", input);
                user_list.user_name = input;
            }
            else
            {
                printf("Opción inválida.\n");
            }

            user_option.userlist = &user_list;
            
        }
        else if (op == 3)
        {
            user_option.op = 3;
            ChatSistOS__Status *status = malloc(sizeof(ChatSistOS__Status));
            chat_sist_os__status__init(status);
            
            printf("Ingrese el nombre de usuario: ");
            scanf("%s", input);
            status->user_name = strdup(input);

            printf("Ingrese el estado (1: Activo, 2: Ocupado, 3: Inactivo): ");
            int new_status;
            scanf("%d", &new_status);
            status->user_state = new_status;

            user_option.status = status;
            
        }
            
        else if (op == 4)
        {
            user_option.op = 4;
            ChatSistOS__Message message = CHAT_SIST_OS__MESSAGE__INIT;
            printf("¿Desea enviar un mensaje privado? (1: sí, 0: no): ");
            int private_message;
            scanf("%d", &private_message);
            message.message_private = private_message;
            if (private_message == 1)
            {
                printf("Ingrese el nombre de usuario del destinatario: ");
                scanf("%s", input);
                message.message_destination = strdup(input);

                printf("Ingrese el contenido del mensaje: ");
                scanf(" %[^\n]", input);
                message.message_content = input;

                // Asignar el nombre de usuario del emisor al mensaje
                message.message_sender = strdup(user_name);
            }
            if (private_message == 0)
            {
                printf("Ingrese el contenido del mensaje: ");
                scanf(" %[^\n]", input);
                message.message_content = input;

                // Asignar el nombre de usuario del emisor al mensaje
                message.message_sender = strdup(user_name);
            }
            user_option.message = &message;
        }
        else if (op == 5)
        {
            close(client_fd);
            break;
        }    
        else
        {
            printf("Opción no válida.\n");
            continue;
        }

        int user_option_size = chat_sist_os__user_option__get_packed_size(&user_option);
        chat_sist_os__user_option__pack(&user_option, buffer);

        bytes_sent = send(client_fd, buffer, user_option_size, 0);

        if (bytes_sent < 0)
        {
            perror("Error al enviar el mensaje al servidor");
        }
    }

    return 0;

}
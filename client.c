#include <arpa/inet.h>
#include <netinet/in.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

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
        case 2: // Ver usuarios conectados
        case 3: // Cambiar estado de usuario
            printf("%s\n", answer->response_message);
            break;
        case 5: // Recibir mensaje
            printf("[%s]: %s\n", answer->message->message_sender, answer->message->message_content);
            break;
        }

        chat_sist_os__answer__free_unpacked(answer, NULL);
    }

    return NULL;
}

int main()
{
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
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

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
        printf("1. Crear nuevo usuario\n");
        printf("2. Ver usuarios conectados\n");
        printf("3. Cambiar estado de usuario\n");
        printf("4. Enviar mensaje\n");
        printf("5. Salir\n");
        printf("Ingrese la opción que desea realizar: ");
        scanf("%d", &op);

        chat_sist_os__user_option__init(&user_option);

        if (op == 1)
        {
            user_option.op = 1;
            ChatSistOS__NewUser new_user = CHAT_SIST_OS__NEW_USER__INIT;
            printf("Ingrese el nombre de usuario: ");
            scanf("%s", input);
            new_user.username = input;
            user_option.createuser = &new_user;
        }
        else if (op == 2)
        {
            user_option.op = 2;
            ChatSistOS__UserList user_list = CHAT_SIST_OS__USER_LIST__INIT;
            printf("¿Desea ver la lista de todos los usuarios conectados? (1: sí, 0: no): ");
            int list_all;
            scanf("%d", &list_all);
            user_list.list = list_all;

            if (!list_all)
            {
                printf("Ingrese el nombre de usuario para obtener información: ");
                scanf("%s", input);
                user_list.user_name = input;
            }

            user_option.userlist = &user_list;
        }
        else if (op == 3)
        {
            ChatSistOS__Status status = CHAT_SIST_OS__STATUS__INIT;
            printf("Ingresa el nuevo estado (1: en línea, 2: ocupado, 3: desconectado): ");
            scanf("%d", &temp);
            getchar(); // Limpiar el buffer de entrada
            status.user_state = temp;

            user_option.op = 3;
            user_option.status = &status;

            // Enviar la opción al servidor
            send_option_and_receive_answer(socket_fd, &user_option, &answer);
            printf("Respuesta del servidor: %s\n", answer.response_message);
        }
        else if (op == 4)
        {
            ChatSistOS__Message message = CHAT_SIST_OS__MESSAGE__INIT;
            printf("¿Deseas enviar un mensaje privado? (1: Sí, 0: No): ");
            scanf("%d", &temp);
            getchar(); // Limpiar el buffer de entrada
            message.message_private = temp;

            if (message.message_private)
            {
                printf("Ingresa el nombre de usuario del destinatario: ");
                fgets(buffer, BUFFER_SIZE, stdin);
                buffer[strcspn(buffer, "\n")] = '\0';
                message.message_destination = buffer;
            }

            printf("Ingresa el contenido del mensaje: ");
            fgets(buffer2, BUFFER_SIZE, stdin);
            buffer2[strcspn(buffer2, "\n")] = '\0';
            message.message_content = buffer2;

            user_option.op = 4;
            user_option.message = &message;

            // Enviar la opción al servidor
            send_option_and_receive_answer(socket_fd, &user_option, &answer);
            printf("Respuesta del servidor: %s\n", answer.response_message);
        }
        else if (op == 5)
        {
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

    close(client_fd);

    return 0;
}


       

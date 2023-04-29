# Chat-Sistos

### Compilar el Servidor

gcc servidor.c chat.pb-c.c -o servidor -lprotobuf-c

### Compilar el Cliente

gcc client.c chat.pb-c.c -o cliente -lprotobuf-c

### Correr Servidor
./servidor 8080

### Correr Cliente
./cliente



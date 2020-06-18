#include <stdio.h>
#include <sys/socket.h>
#include <unistd.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <string.h>
#include <sys/stat.h>
#include <fcntl.h>

#define PORT 8080       //Client access port

int createSocket(struct sockaddr_in address);
void exchange(int server_fd, struct sockaddr_in address);
char *msgConstuct(char *msg);
char *snip(const char intputString[]);

int main(int argc, char const *argv[])
{
    int server_socket;          
    struct sockaddr_in address;                                 

    address.sin_family = AF_INET;                               //Address family.
    address.sin_addr.s_addr = htonl(INADDR_ANY);                //IPv4 transport address
    address.sin_port = htons(PORT);                             //Port number
    memset(address.sin_zero, '\0', sizeof address.sin_zero);    //Writes zeros to &address, for the length of address (?)

    server_socket = createSocket(address);      //Create a socket for accepting connections.
    exchange(server_socket, address);           //Enable client access
 
}

/*
- The analogy of creating a socket is that of requesting a telephone line from the phone company.
- Once we create the socket we must name it to assign it a transport address (here a port number).
- ToThen, we listen on the socket to make sure that it is ready for communication
*/
int createSocket(struct sockaddr_in address)
{
    //Create a socket in the IP Domain of type TCP
    int server_socket;      //Empty var for the socket
    if ((server_socket = socket(AF_INET, SOCK_STREAM, 0)) == 0)
    {   
        perror("In socket");
        exit(EXIT_FAILURE);
    }   

    //Bind the socket using the parameters stored in the sockaddr struct
    if (bind(server_socket,(struct sockaddr *)&address, sizeof(address)) < 0) 
    { 
        perror("bind failed"); 
        close(server_socket);
        return 0; 
    }

    //Listen for incoming connections. Max of 10.
    if (listen(server_socket, 10) < 0) 
    { 
        perror("In listen"); 
        exit(EXIT_FAILURE); 
    }

   return server_socket;

}

/*
Enable communication between the server and the client by calling this function:
*/
void exchange(int server_socket, struct sockaddr_in address)
{
    int comms_socket;                   //The socket facilitating data transfer
    long *valread;                       //Container for recieved client data
    int addrlen = sizeof(address);      //Length of the address field
    char *packet;

    //char *Ack = msgConstuct("Ack");
    char *Ack = "HTTP/1.1 200 OK\nContent-Type: text/plain\nContent-Length: 12\n\nReq Recieved";

    while(1)
    {
        //Grabs a waiting connection then creates a new socket to facilitate manipulation
        printf("\n+++++++ WAITING FOR NEW CONNECTION ++++++++\n\n");
        if ((comms_socket = accept(server_socket, (struct sockaddr *)&address, (socklen_t*)&addrlen))<0)
        {
            perror("Error in accept");
            close(server_socket);           
            exit(EXIT_FAILURE);
        }   

        //Read a message from the client, save it, then print it to log
        char buffer[30000] = {0};                       
        valread = read(comms_socket, buffer, 30000);
        printf("RECIEVED MSG:\n%s\n", buffer);

        //Snip the location of the file from the client message
        //FILE *clientReq;
        char *location = snip(buffer);
        if (location != "0")
        {
            /* ATTEMPT ONE
            //clientReq = fopen(location, "r");       //Pointer to start of file in memory
            struct stat st;
            stat(location, &st);
            int status = sendfile(comms_socket, location, 0, st.st_size);
            //int status = sendfile(comms_socket, location, NULL, size);
            printf("STATUS: %i\n", status); 
            //packet = msgConstuct("clientReq");
            */

            //Try to open file first
            int fd = open(location, O_RDONLY);
            if (fd == -1) 
            {   
                printf("UNABLE TO OPEN '%s'\n", location);
                exit(1);
            }

            struct stat stat_buf;
            fstat(fd, &stat_buf);

            int rc = sendfile(comms_socket, fd, 0, stat_buf.st_size);
            if (rc == -1) 
            {
                printf("ERROR SENDING: %s\n", location);
                exit(1);
            }
            if (rc != stat_buf.st_size) {
                printf("INCOMPLETE TRANSFER: %d of %d bytes\n", rc, (int)stat_buf.st_size);
                exit(1);
            }   
            if (rc == stat_buf.st_size)
                printf("TRANSFER COMPLETED");
        }

        //Write a response to the client
        write(comms_socket, Ack, strlen(Ack));                  
        printf("ACK RETURNED\n");  
        printf("CONNECTION CLOSED\n\n");    

        close(comms_socket);    //Close socket

    }
}

char *snip(const char intputString[])
{
    static char location[16];

    //Find the start of the file name and save
    char *p = strchr(intputString, '/');
    if (p == NULL)
        return "0";

    int startIndex = (int)(p - intputString);

    //Copy inputString to location whilst we are within the file name
    int cpyIndex = 0;

    //location[cpyIndex++] = '.';
    while( (location[cpyIndex++] = intputString[startIndex++]) != ' ')
        ;

    location[cpyIndex++] = '\0';
    return location;
}

/*
Construct a string message including header for sending to clients
*/
char *msgConstuct(char *msg)
{
    char msgLen = strlen(msg);
    char *header = "HTTP/1.1 200 OK\nContent-Type: text/plain\nContent-Length: "; 
    header = strcat(header, msgLen);
    header = strcat(header, "\n\n");
    return strcat(header, msg);
}


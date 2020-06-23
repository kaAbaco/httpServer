#include <stdio.h>
#include <sys/socket.h>
#include <unistd.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <string.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>

#define PORT 8080       //Client access port

int createSocket(struct sockaddr_in address);
void exchange(int server_fd, struct sockaddr_in address);
char *msgConstuct(char *msg);
char *snip(const char intputString[]);

int main(void)
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
The analogy of creating a socket is that of requesting a telephone line from the phone company.
Once we create the socket we must name it to assign it a transport address (here a port number).
Then, we listen on the socket to make sure that it is ready for communication
*/
int createSocket(struct sockaddr_in address)
{
    //Create a socket in the IP Domain of type TCP
    int server_socket;     
    if ((server_socket = socket(AF_INET, SOCK_STREAM, 0)) == 0)
    {   
        perror("In socket");
        close(server_socket);
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
        close(server_socket);
        exit(EXIT_FAILURE); 
    }

   return server_socket;

}

/*
ENABLES COMMUNCATION BETWEEN SERVER AND CLIENT
argument:   server_socket is the connection broker socket created in createSocket()
            address is the socket descripion struct that was populated in main()
*/
void exchange(int server_socket, struct sockaddr_in address)
{

    char *Ack = "HTTP/1.1 200 OK\nContent-Type: text/plain\nContent-Length: 12\n\nReq Recieved";

    while(1)
    {
        //Grabs a waiting connection then creates a new socket to facilitate manipulation
        int comms_socket;
        int addrlen = sizeof(address);                  
        printf("\n+++++++ WAITING FOR NEW CONNECTION ++++++++\n\n");
        if ((comms_socket = accept(server_socket, (struct sockaddr *)&address, (socklen_t*)&addrlen))<0)
        {
            perror("Error in accept");          
            exit(EXIT_FAILURE);
        }   

        //Read a message from the client, save it, then print it to log                      
        char inputBuffer[3000] = {0};
        long valread = read(comms_socket, inputBuffer, 3000);
        printf("RECIEVED MSG:\n%s\n", inputBuffer);

        //Find full working path
        char *dirBuff; 
        long *buffSize = pathconf(".", _PC_PATH_MAX); 
        char *wrkDir = getcwd(dirBuff, buffSize);

        char *p = strchr(inputBuffer, '/');

        //Snip the location of the file from the client message
        char *reqLocation = snip(inputBuffer);                      /*CONSTANT SEGFAULT HERE*/
        char *fullLocation = strcat(wrkDir, reqLocation); 
        
        //If we actualy found a location then procede
        if (reqLocation != "0")
        {

            //Try to open the file, then read and store its contents 
            FILE *fptr;
            char toWrite[255];
            if ( !(fptr = fopen(reqLocation, "r")) )
            {
                fprintf(stderr, "open error for %s, errno = %d\n", reqLocation, errno);
                exit(1);
            }
            fgets(toWrite, 255, fptr);
            
            //Find the size of the requested file
            /*struct stat stat_buf;
            fstat(fd, &stat_buf);*/

            //Send file through comms socket
            int rc = sendfile(comms_socket, fptr, 0, strlen(toWrite));
            if (rc == -1) 
            {
                printf("ERROR SENDING: %s\n", fullLocation);
                //exit(1);
            }

            //If sizes dont match then an incomplete transfer occured
            if (rc != strlen(toWrite)) {
                printf("INCOMPLETE TRANSFER: %d of %d bytes\n", rc, (int)strlen(toWrite));
                //exit(1);
            } 

            //If they do, then it was (maybe/probably/hopefully successful)  
            if (rc == strlen(toWrite))
                printf("TRANSFER COMPLETED");
        }

        //Write a response to the client
        write(comms_socket, Ack, strlen(Ack));                  
        printf("ACK RETURNED\n");  
        printf("CONNECTION CLOSED\n\n");    

        close(comms_socket);    //Close socket

    }
}

/*
SNIP OUT THE FILE LOCATION FROM CLIENT REQUEST
Argument:   inputSring[] is the message reieved from the client
*/
char *snip(const char intputString[])
{   
    //Find the start of the file name and save
    char *p = strchr(intputString, '/');
    if (p == NULL)
        return "0";
    int startIndex = (int)(p - intputString);

    //Copy inputString to location whilst we are within the file name
    int cpyIndex = 0;
    static char location[16];
    while( (location[cpyIndex++] = intputString[startIndex++]) != ' ')
        ;
    location[--cpyIndex] = '\0';

    return location;
}

/*REQUIRES MORE WORK
//Construct a string message including header for sending to clients
char *msgConstuct(char *msg)
{
    char msgLen = strlen(msg);
    char *header = "HTTP/1.1 200 OK\nContent-Type: text/plain\nContent-Length: "; 
    header = strcat(header, msgLen);
    header = strcat(header, "\n\n");
    return strcat(header, msg);
}
*/
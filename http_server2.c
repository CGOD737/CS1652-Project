/*
 * CS 1652 Project 1
 * (c) Jack Lange, 2020
 * (c) <Christopher Godfrey, Diana Kocsis>
 *
 * Computer Science Department
 * University of Pittsburgh
 */

#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#define BUFSIZE 1024
#define FILENAMESIZE 100
#define TRUE 1
#define FALSE 0

static int handle_connection(int sock, int c){
    /*header declaration*/
    char * ok_response_f  = "HTTP/1.0 200 OK\r\n"        \
        "Content-type: text/plain\r\n"                  \
        "Content-Length: %d \r\n\r\n";

    char * notok_response = "HTTP/1.0 404 FILE NOT FOUND\r\n"   \
        "Content-type: text/html\r\n\r\n"                       \
        "<html><body bgColor=black text=white>\n"               \
        "<h2>404 FILE NOT FOUND</h2>\n"
        "</body></html>\n";
    /*variable declarations*/
    int len, i, j, k, z, res, bytes;
    char buf[BUFSIZE];
    int ok_response;
    char * write_buf = NULL;
    char * fileName = NULL;

fileName = (char*)malloc(sizeof(char)*FILENAMESIZE); //declares the filename array

    /* first read loop -- get request and headers*/
        memset(buf, 0, BUFSIZE);
        if ((len = read(c, buf, sizeof(buf) - 1)) <= 0){
            fprintf(stderr, "FAILED TO READ LINE FROM SOCKET");
            exit(-1);
        }

        /*Lets Server Operator know that Client has connected */
        printf("Client Connected, Reading Request...%c", '\n');

        /*gets the filename from the read buffer*/
        j = FALSE; /*file size*/
        k = 0;

        memset(fileName, 0, FILENAMESIZE);

        for ( i = 0; i < len; i++){ /*Assumption that is made is that every buffer request to the server will look the same so we can start on the 5th index point*/
            if ( buf[i] == '/' && j == FALSE)
                j = TRUE;/*When you reach a space, that is how you know you reach the end of the file.*/
            else if ( buf[i] == ' ' && j == TRUE){
                break;/*When you reach a space, that is how you know you reach the end of the file.*/
            }
            else if ( j == TRUE ){
                fileName[k] = buf[i];
                k++;
            }
            else
                continue;
        }
        buf[len] = 0; /*resets the buffer array so we can store the contents of file into the buffer array.*/
        /*Lets the sever operator know the exact requested file*/
        printf("The Requested file is: ");
        for ( z= 0; z < k; z++)
            printf("%c", fileName[z]);

         printf("%c", '\n');

         /* open and read the files */
        FILE *req = fopen(fileName, "rb" );

        if ( req == NULL){
            fprintf(stderr, "Error Opening file: %s%c ", fileName, '\n');
            ok_response = FALSE;
        }
        else{
            printf("Successfully opened file... %c", '\n');
            ok_response = TRUE;
            free(fileName);
        }
        /*if the file was found, the response will be okay*/
        if ( ok_response ){
            /*first it sends the header to the socket to indicate an OK response*/
            if ((res = write(c, ok_response_f, strlen(ok_response_f))) <= 0){
                fprintf(stderr, "FAILED TO WRITE OUT LINE TO SOCKET");
                exit(-1);
            }

            write_buf = (char*)malloc(BUFSIZE);

            int j = 0;
            /*Opens and reads the file into buf*/
            while ( (bytes = fread(&buf, 1, BUFSIZE - 1, req)) > 0){
               buf[bytes] = 0;
               if (j == 0)
                  strcpy(write_buf, buf);
               else {
                  write_buf = realloc(write_buf, strlen(write_buf) + strlen(buf));
                  strcat(write_buf, buf);
               }
              j++;
            }
            /*sends the contents of the finle into the socket*/
            if ((res = write(c, write_buf, strlen(write_buf))) <= 0){
                fprintf(stderr, "FAILED TO WRITE OUT LINE TO SOCKET");
                exit(-1);
            }
        }

        /* sends ok_response message if the file was not found */
        if ( !ok_response ){
            if ((res = write(c, notok_response, strlen(notok_response))) <= 0){
                fprintf(stderr, "FAILED TO WRITE OUT LINE TO SOCKET");
                exit(-1);
            }
        }
free(write_buf);
        printf("Request Processed, sending response to the Client...%c", '\n');
        close(c);
        printf("Waiting for client connection...%c", '\n');
    /* close socket and free pointers */

    close(sock);
return 0;
}


int
main(int argc, char ** argv)
{
    printf("TESTING: 1 %c", '\n');
    int server_port = -1;
    int ret         =  0;
    //int sock        = -1;
    //int addrlen;
    int listener, binder, s, clientSocket, result;

    struct sockaddr_in saddr; //structure to initialize socket address
    /* parse command line args */
    if (argc != 2) {
        fprintf(stderr, "usage: http_server1 port\n");
        exit(-1);
    }
    server_port = atoi(argv[1]);

    if (server_port < 0 || server_port > 65535) {
        fprintf(stderr, "INVALID PORT NUMBER: %d;", server_port);
        exit(-1);
    }
    if (server_port < 1500) {
        fprintf(stderr, "SPECIAL PERMISSION IS NEEDED TO BIND PORT NUMBER: %d;", server_port);
        exit(-1);
    }
    /*create socket*/
    s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

    if ( s < 0 ){
        fprintf(stderr, "FAILED TO CREATE SOCKET\n");
        exit(-1);
    }
    /* set server address*/
    memset(&saddr, 0, sizeof(saddr));
    saddr.sin_family = AF_INET;
    saddr.sin_addr.s_addr = INADDR_ANY;
    saddr.sin_port = htons(server_port);
    /* bind listening socket */
    binder = bind(s, (struct sockaddr *) &saddr, sizeof(saddr));
    if (binder < 0){
        fprintf(stderr, "FAILED TO BIND SOCKET ADDRESS\n ");
        exit(-1);
    }
    printf("Bind to Port Number: %d%c", server_port, '\n');
    /* start listening */
    listener = listen(s, 32);
    printf("Listening Started%c", '\n');
    if (listener < 0){
        fprintf(stderr, "FAILED TO LISTEN FOR CONNECTION\n ");
        exit(-1);
    }

    fd_set full;
    fd_set connected;
    FD_ZERO(&full);         // removes all file descriptors from full
    FD_ZERO(&connected);    // removes all file descriptors from connected
    FD_SET(s, &full);       // adds the socket s to the set full
    /* connection handling loop: wait to accept connection */
    int i;
    printf("Waiting for client connection...%c", '\n');

    //addrlen = sizeof(saddr);
    int max_sock = s;

    while (TRUE) {
      /* create read list */
      connected = full;

      /* do a select */
      if ((result = select(max_sock+1, &connected, 0, 0, 0)) > 0) {
        /* process sockets that are ready */
        for (i = 0; i < max_sock + 1; i++) {

          if (FD_ISSET(i, &connected)) {
            // returns zero if socket is not in list anymore
          /* for the accept socket, add accepted connection to connections */
            if (i == s) {
              if ( (clientSocket = accept(s, NULL, NULL)) >= 0) {
                  if (clientSocket > max_sock) {
                      max_sock = clientSocket;
                  }
                  FD_SET(clientSocket, &full);      //adds the socket to the list
              }
            }
            else {
              /* for a connection socket, handle the connection */
                ret = handle_connection(i, clientSocket);
                if (ret < 0)
                 fprintf(stderr, "FAILED TO CONNECT");
                close(i);           //close the socket
                FD_CLR(i, &full);   //removes the socket i from the list
            }
          }
            }
        }
    }
    close(listener);
}

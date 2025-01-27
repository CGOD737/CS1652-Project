/*
 * CS 1652 Project 1
 * (c) Jack Lange, 2020
 * (c) <Diana Kocsis and Christopher Godfrey>
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

#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#define BUFSIZE 1024

int
main(int argc, char ** argv)
{
    char * server_name = NULL;
    int    server_port = -1;
    char * server_path = NULL;
    char * req_str     = NULL;

    int ret = 0;

    int clientSocket, res;
    struct hostent *hp;
    struct sockaddr_in saddr;
    fd_set set;
    char buf[BUFSIZE];
    char header[BUFSIZE];
    bool b = false;

    /*parse args */
    // Parameter checking
    if (argc != 4) {
        fprintf(stderr, "usage: http_client <hostname> <port> <path>\n");
        exit(1);
    }

    server_name = argv[1];
    server_port = atoi(argv[2]);
    server_path = argv[3];


    // returns number of bytes
    // remember to free req_str
    // request has 3 fields: method, URL, and HTTP version
    ret = asprintf(&req_str, "GET /%s HTTP/1.0\r\n\r\n", server_path);

    if (ret == -1) {
        fprintf(stderr, "Failed to allocate request string\n");
        exit(1);
    }


    /* make socket */
    // Returns file descriptor
    // AF_INET indicates network is using IPv4
    // SOCK_STREAM indicates the socket is a TCP socket
    // IPPROTO_TCP indicates that the TCP protocol is to be used.
    if ((clientSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) {
        fprintf(stderr, "Socket creation failed.\n");
        free(req_str);
        exit(1);
    }

    /* get host IP address  */
    /* Hint: use gethostbyname() */
    // name lookup
    if ((hp = gethostbyname(server_name)) == NULL) {
        fprintf(stderr, "ERROR, no such host.\n");
        free(req_str);
        close(clientSocket);
        exit(1);
    }

    /* set address */
    saddr.sin_family = AF_INET;
    memcpy(&saddr.sin_addr.s_addr, hp->h_addr, hp->h_length);
    saddr.sin_port = htons(server_port);


    /* connect to the server socket */
    if (connect(clientSocket, (struct sockaddr *) &saddr, sizeof(saddr)) < 0) {
        fprintf(stderr, "Error connecting.\n");
        free(req_str);
        close(clientSocket);
        exit(1);
    }

    /* send request message */
    if ((res = write(clientSocket, req_str, strlen(req_str))) <= 0) {
        fprintf(stderr, "Error sending request");
        free(req_str);
        close(clientSocket);
        exit(1);
    }

    /* wait till socket can be read. */
    /* Hint: use select(), and ignore timeout for now. */
    FD_ZERO(&set); // clears (removes all file descriptors from) set
    FD_SET(clientSocket, &set); // adds the file descriptor (the socket) to set
    select(clientSocket + 1, &set, NULL, NULL, NULL);

    /* first read loop -- read headers */
    if ((res = read(clientSocket, &buf, BUFSIZE - 1)) <= 0) {
        fprintf(stderr, "Error reading from socket.\n");
        free(req_str);
        close(clientSocket);
        exit(1);
    }

    buf[res] = 0;

    // split response at \r\n\r\n

    char *token = "\r\n\r\n";
    char *response = NULL;
    char *new_resp = NULL;

    response = (char *) malloc(BUFSIZE);
    int i = 0;
    while (res > 0) {
        // copy buffer into response string
        char *p;

        // if header was not found in first buffer
        if (i > 0) {
            response = realloc(response,strlen(response) + strlen(buf));
            char temp[strlen(response) + strlen(buf)];
            for (int i = 0; i < (int) strlen(header); i++)
                temp[i] = header[i];
            char header[strlen(response) + strlen(buf)];
            strncpy(header, temp, strlen(temp));

        }
        // Add buffer to response string
        if (i == 0)
            strcpy(response, buf);
        else
            strcat(response, buf);

        // p becomes pointer to start of token to end of string
        p = strstr(response, token);

        // if rnrn is found
        if (p) {

            strncpy(header, response, p-response);
            header[p-response] = 0;
            new_resp = (char *) malloc(strlen(buf));
            if (i == 0)
                strncpy(new_resp, p +strlen(token), strlen(p) + 1);
            else
                strncat(new_resp, p +strlen(token), strlen(p) + 1);
            break;
        }
        strcat(header, response);
        res = read(clientSocket, buf, sizeof(buf) - 1);
        buf[res] = 0;

        i++;
    }


    free(response);

    // Grab rest of response
    res = 1;
    while (res > 0) {
        select(clientSocket + 1, &set, NULL, NULL, NULL);
        res = read(clientSocket, buf, sizeof(buf) - 1);
        buf[res] = 0;
        new_resp = realloc(new_resp, strlen(new_resp) + strlen(buf));
        strcat(new_resp, buf);
    }

    // Copy into temp buffer to grab status
    char tmp[strlen(header)];

    for (int i = 0; i < (int) strlen(header); i++)
        tmp[i] = header[i];

    char * toks = " ";
    char * tp = strstr(tmp, toks);
    char * status = strtok(tp, toks);

    if (atoi(status) == 200)
        b = true;

    /* print first part of response: header, error code, etc. */
    if (b == true) {
        printf(header, strlen(new_resp));
        printf("\n%s\n", new_resp);
    }

    else {
        fprintf(stderr, header, strlen(new_resp));

        fprintf(stderr, "\n%s\n", new_resp);
    }

    /*close socket and deinitialize */
    close(clientSocket);
    free(new_resp);
    free(req_str);
    if (b == true)
        return 0;
    else
        return -1;
}

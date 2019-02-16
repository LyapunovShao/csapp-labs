#include <stdio.h>
#include "csapp.h"
/* Recommended max cache and object sizes */
#define MAX_CACHE_SIZE 1049000
#define MAX_OBJECT_SIZE 102400

/* You won't lose style points for including this long line in your code */
static const char *user_agent_hdr = "User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:10.0.3) Gecko/20120305 Firefox/10.0.3\r\n";

int main(int argc, char **argv) {
    int clientfd;
    char *host, *port, buf[MAXLINE] = {0}, line[MAXLINE] = {0};
    line[0] = '\r';
    rio_t rio_client, rio_input;

    if (argc != 3) {
        fprintf(stderr, "usage: %s <host> <port>\n", argv[0]);
        exit(1);
    }

    host = argv[1], port = argv[2];

    clientfd = Open_clientfd(host, port);
    Rio_readinitb(&rio_input, STDIN_FILENO);
    Rio_readinitb(&rio_client, clientfd);
    while (strcmp(line, "\r\n")) {
        Rio_readlineb(&rio_input, line + 1, MAXLINE);
        strcat(buf, line);
    }
    rio_writen(clientfd, buf, MAXLINE);
    while (Rio_readlineb(&rio_client, line, MAXLINE))
        puts(line);


    /*
    while (Fgets(temp_buf, MAXLINE, stdin)) {
        strcpy(buf + 1, temp_buf);
        Rio_writen(clientfd, buf, strlen(buf));
        Rio_readlineb(&rio, temp_buf, MAXLINE);
        Fputs(temp_buf, stdout);
    }
     */
    Close(clientfd);
    exit(0);
    //printf("%s", user_agent_hdr);

}




/*
 *
 #include <stdio.h>
#define MAX_CACHE_SIZE 1049000
#define MAX_OBJECT_SIZE 102400

static const char *user_agent_hdr = "User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:10.0.3) Gecko/20120305 Firefox/10.0.3\r\n";

int main()
{
    printf("%s", user_agent_hdr);
    return 0;
}
 *
 */

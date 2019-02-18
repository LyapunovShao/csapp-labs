#include <stdio.h>
#include "csapp.h"
/* Recommended max cache and object sizes */
#define MAX_CACHE_SIZE 1049000
#define MAX_OBJECT_SIZE 102400

/* You won't lose style points for including this long line in your code */
static const char *user_agent_hdr = "User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:10.0.3) Gecko/20120305 Firefox/10.0.3\r\n";


struct cache_item {
    char *website;
    char *file;
    char *response;
    size_t content_size;
    struct cache_item *next;
};


void work(int fd, char *hostname, struct cache_item **cache_head_p, size_t *cache_size_p);

int requestheaders(rio_t *rp, char *clienthost);

int parse_uri(char *uri, char *website, char *file,
              char *port);

int check_website(char ch);

int check_port(char ch);

struct cache_item *find_cache_item(char *current_website, char *current_file, struct cache_item **cache_head_p);

int main(int argc, char **argv) {
    if (argc != 2) {
        fprintf(stderr, "usage: %s <port>\n", argv[0]);
        exit(1);
    }
    char hostname[MAXLINE], port[MAXLINE];
    struct cache_item *cache_head = NULL;
    size_t cache_size = 0;
    struct sockaddr_storage clientaddr;
    socklen_t clientlen;
    int listenfd, connfd;
    listenfd = Open_listenfd(argv[1]);
    while (1) {
        clientlen = sizeof(struct sockaddr_storage);
        connfd = Accept(listenfd, (SA *) &clientaddr, &clientlen);
        Getnameinfo((SA *) &clientaddr, clientlen, hostname, MAXLINE, port, MAXLINE, 0);
        printf("Accepted connection from (%s, %s)\n", hostname, port);
        work(connfd, hostname, &cache_head, &cache_size);
        Close(connfd);
    }


}

struct cache_item *find_cache_item(char *current_website, char *current_file, struct cache_item **cache_head_p) {
    for (struct cache_item *p = *cache_head_p; p; p = p->next)
        if ((!strcmp(p->website, current_website)) && (!strcmp(p->file, current_file)))
            return p;
    return NULL;
}

void work(int fd, char *hostname, struct cache_item **cache_head_p, size_t *cache_size_p) {
    char line[MAXLINE], method[MAXLINE], uri[MAXLINE],
            version[MAXLINE], clienthost[MAXLINE],
            website[MAXLINE], serverport[MAXLINE] = "80",
            file[MAXLINE], message[MAXLINE] = {0};
    rio_t rio_client, rio_server;
    Rio_readinitb(&rio_client, fd);
    int hostexist;

    if (!Rio_readlineb(&rio_client, line, MAXLINE))
        return;
    printf("%s", line);
    sscanf(line, "%s %s %s", method, uri, version);


    hostexist = requestheaders(&rio_client, clienthost);


    int type = parse_uri(uri, website, file, serverport);
    if (!strcmp(file, "/"))
        strcpy(file, "/home.html");
    (hostexist) ? strcpy(hostname, clienthost) : strcpy(hostname, website);

    struct cache_item *cache_p;
    if (cache_p = find_cache_item(website, file, cache_head_p)) {
        Rio_writen(fd, cache_p->response, strlen(cache_p->response));
    } else {
        int proxyfd = Open_clientfd(website, serverport);
        Rio_readinitb(&rio_server, proxyfd);

        sprintf(message, "%s %s HTTP/1.0\r\n", method, file);
        sprintf(message, "%sHost: %s\r\n", message, hostname);
        sprintf(message, "%s%s", message, user_agent_hdr);
        sprintf(message, "%sConnection: close\r\n", message);
        sprintf(message, "%sProxy-Connection: close\r\n\r\n", message);
        printf("HTTP request:\n%s", message);

        Rio_writen(proxyfd, message, MAXLINE);
        memset(message, 0, sizeof(message));
        while (Rio_readlineb(&rio_server, line, MAXLINE)) {
            printf("%s", line);
            sprintf(message, "%s%s", message, line);
        }
        Rio_writen(fd, message, MAXLINE);
        Close(proxyfd);
        if (strlen(website) + strlen(file) + strlen(message) >= MAX_CACHE_SIZE)
            return;
        struct cache_item *tem;

        while ((*cache_head_p) && strlen(website) + strlen(file) + strlen(message) +
                                  (*cache_size_p) >= MAX_CACHE_SIZE) {
            Free((*cache_head_p)->file);
            Free((*cache_head_p)->website);
            Free((*cache_head_p)->response);
            tem = *cache_head_p;
            *cache_head_p = (*cache_head_p)->next;
            Free(tem);
            *cache_size_p -= tem->content_size;
        }
        tem = Malloc(sizeof(struct cache_item));
        tem->next = *cache_head_p;
        *cache_head_p = tem;
        tem->website = Malloc(strlen(website));
        strcpy(tem->website, website);
        tem->file = Malloc(strlen(file));
        strcpy(tem->file, file);
        tem->response = Malloc(strlen(message));
        strcpy(tem->response, message);
        tem->content_size = strlen(message) + strlen(file) + strlen(website);
        *cache_size_p += tem->content_size;
    }
}

int parse_uri(char *uri, char *website, char *file,
              char *port) {//return -1 for failed ;0 for default serverport;1 for a port specified
    int httpexist = 1, err = 0, i = 0, j, type = 0;
    char prefix[10];
    for (int k = 0; k <= 6; ++k)
        prefix[k] = uri[k];
    prefix[7] = 0;
    if (strcmp(prefix, "http://")) {
        prefix[7] = uri[7];
        prefix[8] = 0;
        if (strcmp(prefix, "https://"))
            httpexist = 0;
    }
    if (httpexist) {
        for (int cnt = 0; cnt < 2; ++i) {
            if (uri[i] == '/')
                ++cnt;
        }
    }
    for (j = 0; uri[i] != '/'; ++i, ++j)
        website[j] = uri[i];
    website[j] = 0;
    strcpy(file, uri + i);
    for (i = 0; check_website(website[i]); ++i);
    if (i < strlen(website)) {
        type = 1;
        if (website[i] == ':') {
            int len = (int) strlen(website);
            website[i++] = 0;
            for (j = 0; i < len; ++i, ++j)
                port[j] = website[i];
            port[j] = 0;
            if (port[0] == '0' || strlen(port) < 1)
                err = 1;
            for (j = 0; j < strlen(port); ++j) {
                if (!check_port(port[j])) {
                    err = 1;
                    break;
                }
            }
        } else err = 1;
    }
    if (err)
        return -1;
    return type;
}

int requestheaders(rio_t *rp, char *clienthost) { //return 1 for Host: is specified by client
    char line[MAXLINE], firstword[MAXLINE];
    Rio_readlineb(rp, line, MAXLINE);
    printf("%s", line);
    sscanf(line, "%s", firstword);
    if (!strcmp(firstword, "Host:")) {
        sscanf(line, "%s %s", firstword, clienthost);
        return 1;
    } else return 0;


}

int check_website(char ch) {
    return (ch <= 'z' && ch >= 'a') || (ch <= 'Z' && ch >= 'A') || ch == '.' || (ch <= '9' && ch >= '0');
}

int check_port(char ch) {
    return (ch <= '9' && ch >= '0');
}



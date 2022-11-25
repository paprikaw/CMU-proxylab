#include <stdio.h>
#include "csapp.h"
// Sequential -> cocurrent ->
/* Recommended max cache and object sizes */
#define MAX_CACHE_SIZE 1049000
#define MAX_OBJECT_SIZE 102400
#define PORT "38888"
/* You won't lose style points for including this long line in your code */
static const char *user_agent_hdr = "User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:10.0.3) Gecko/20120305 Firefox/10.0.3";
static const char *conn_hdr = "Connection: close\r\n";
static const char *proxy_conn_hdr = "Proxy-Connection: close\r\n";
int transmit(int connfd);
int parse_request_line(int fd, char *request_line, char *buf, char *host, char *abs_path);
void clienterror(int fd, char *cause, char *errnum,
                 char *shortmsg, char *longmsg);
int parse_request(int contfd, char *request_buf, char *url);

int main(int argc, char **argv)
{
    int listenfd, connfd;
    socklen_t clientlen;
    struct sockaddr_in clientaddr;
    char request_buf[MAX_OBJECT_SIZE];
    char url[MAXLINE];

    listenfd = Open_listenfd(PORT);
    while (1)
    {
        clientlen = sizeof(clientaddr);
        // 接收client发来的链接
        connfd = Accept(listenfd, (SA *)&clientaddr, &clientlen);
        printf("connected to %d\n", clientaddr.sin_addr.s_addr);
        /* determine the domain name and IP address of the client */
        // 处理client发来的链接
        if (parse_request(connfd, request_buf, url) != -1)
        {
            printf("here");
            printf(request_buf);
            Rio_writen(connfd, request_buf, strlen(request_buf));
        }
        Close(connfd);
        printf("connection is closed\n");
    }
    exit(0);
}

/*
 * parse_request - parse the whole request
 * return -1 if message format is not correct or exceed maximum object size limit
 */
int parse_request(int contfd, char *request_buf, char *url)
{
    char buf[MAXLINE];
    char parsed_request_line[MAXLINE];
    char header_field[MAXLINE];
    char header_field_value[MAXLINE];
    char host[MAXLINE];
    char abs_path[MAXLINE];
    rio_t rio;
    int is_host_header_present = 0; // Host header是否在request message中的header出现过

    Rio_readinitb(&rio, contfd);
    Rio_readlineb(&rio, buf, MAX_OBJECT_SIZE);
    /* parse request line */
    if (parse_request_line(contfd, buf, parsed_request_line, host, abs_path) == -1)
    {
        return -1;
    };
    snprintf(request_buf, MAX_OBJECT_SIZE, "%s", parsed_request_line);
    int host_hdr_exist = 0, conn_hdr_exist = 0, proxy_conn_hdr_exist = 0;
    while (1)
    {
        Rio_readlineb(&rio, buf, MAXLINE);
        if (!strcmp(buf, "\r\n"))
        {
            break;
        }
        if (strstr(buf, "Host:"))
        {
            host_hdr_exist = 1;
            strcat(request_buf, buf);
        }
        else if (strstr(buf, "Connection:"))
        {
            conn_hdr_exist = 1;
            strcat(request_buf, conn_hdr);
        }
        else if (strstr(buf, "Proxy connection:"))
        {
            proxy_conn_hdr_exist = 1;
            strcat(request_buf, proxy_conn_hdr);
        }
        else
        {
            strcat(request_buf, buf);
        }
    }
    strcat(request_buf, user_agent_hdr);
    if (!host_hdr_exist)
    {
        char host_hdr[MAXLINE];
        sprintf(host_hdr, "Host: %s\r\n", host);
        strcat(request_buf, host_hdr);
    }
    if (!conn_hdr_exist)
    {
        strcat(request_buf, conn_hdr);
    }
    if (!proxy_conn_hdr_exist)
    {
        strcat(request_buf, proxy_conn_hdr);
    }
    strcat(request_buf, "\r\n");
#ifdef MY
    /* Add the default value for crutial header fields */
    snprintf(request_buf, MAX_OBJECT_SIZE, "%s", parsed_request_line);
    snprintf(request_buf, MAX_OBJECT_SIZE, "%sUser-Agent: %s\r\n", request_buf, user_agent_hdr);
    snprintf(request_buf, MAX_OBJECT_SIZE, "%sConnection: close\r\n", request_buf);
    snprintf(request_buf, MAX_OBJECT_SIZE, "%sProxy-Connection: close\r\n", request_buf);
    /* Start to read the whole header fields*/
    Rio_readlineb(&rio, buf, MAXLINE);
    while (strcmp(buf, "\r\n"))
    {
        // 如果是这几种已经设置好的header fields，就直接跳过
        if (
            strstr(buf, "User-Agent") ||
            strstr(buf, "Connection") ||
            strstr(buf, "Proxy-Connection"))
        {
            continue;
        }

        // 如果是Host，那么直接使用request header中的Host字段
        if (strstr(buf, "Host"))
        {
            is_host_header_present = 1;
        }
        snprintf(request_buf, MAX_OBJECT_SIZE, "%s%s", request_buf, buf);
        Rio_readlineb(&rio, buf, MAXLINE);
    }

    // 没有Host字段，使用从url中获取的host字段
    if (!is_host_header_present)
    {
        snprintf(request_buf, MAX_OBJECT_SIZE, "%sHost: %s\r\n", request_buf, host);
    }
    // // 将剩下entity body部分写入, 并且检查是否超过buffer的size
    if (snprintf(request_buf, MAX_OBJECT_SIZE, "%s%s", request_buf, buf) >= MAX_OBJECT_SIZE)
        snprintf(request_buf, MAX_OBJECT_SIZE, "\r\n");
        // Rio_readnb(&rio, buf, MAX_OBJECT_SIZE);
        // if (snprintf(request_buf, MAX_OBJECT_SIZE, "%s%s", request_buf, buf) >= MAX_OBJECT_SIZE)
        //     return -1;
#endif
    return 0;
}

/*
 * parse_request_line - taking a request line and parse properly
 * fd is connection file descriptor used to transform error handling page
 */
int parse_request_line(int fd, char *request_line, char *buf, char *host, char *abs_path)

{
    char method[MAXLINE];
    if (sscanf(request_line, "%s http://%[a-zA-Z.]/%s HTTP", method, host, abs_path) != 3)
    {
        clienterror(fd, method, "501", "Not Implemented",
                    "HTTP request is illegal");
        return -1;
    }

    if (strcmp(method, "GET"))
    {
        clienterror(fd, method, "501", "Not Implemented",
                    "Tiny does not implement this method");
        return -1;
    }
    sprintf(buf, "%s /%s %s\r\n", method, abs_path, "HTTP/1.0");

    return 0;
}

/*
 * clienterror - returns an error message to the client
 */
/* $begin clienterror */
void clienterror(int fd, char *cause, char *errnum,
                 char *shortmsg, char *longmsg)
{
    char buf[MAXLINE], body[MAXBUF];

    /* Build the HTTP response body */
    sprintf(body, "<html><title>Tiny Error</title>");
    sprintf(body, "%s<body bgcolor="
                  "ffffff"
                  ">\r\n",
            body);
    sprintf(body, "%s%s: %s\r\n", body, errnum, shortmsg);
    sprintf(body, "%s<p>%s: %s\r\n", body, longmsg, cause);
    sprintf(body, "%s<hr><em>The Tiny Web server</em>\r\n", body);

    /* Print the HTTP response */
    sprintf(buf, "HTTP/1.0 %s %s\r\n", errnum, shortmsg);
    Rio_writen(fd, buf, strlen(buf));
    sprintf(buf, "Content-type: text/html\r\n");
    Rio_writen(fd, buf, strlen(buf));
    sprintf(buf, "Content-length: %d\r\n\r\n", (int)strlen(body));
    Rio_writen(fd, buf, strlen(buf));
    Rio_writen(fd, body, strlen(body));
}
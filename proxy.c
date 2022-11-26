#include <stdio.h>
#include "csapp.h"
#include "url_parser.h"
// Sequential -> cocurrent ->
/* Recommended max cache and object sizes */
#define MAX_CACHE_SIZE 1049000
#define MAX_OBJECT_SIZE 102400
#define PORT "38888"
/* You won't lose style points for including this long line in your code */
static const char *user_agent_hdr = "User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:10.0.3) Gecko/20120305 Firefox/10.0.3\r\n";
static const char *conn_hdr = "Connection: close\r\n";
static const char *proxy_conn_hdr = "Proxy-Connection: close\r\n";
int transmit(int connfd);
void clienterror(int fd, char *cause, char *errnum,
                 char *shortmsg, char *longmsg);
int parse_request(int contfd, char *request_buf, char *host, char *port);
int parse_request_line(rio_t *client_rp, char *parsed_request,
                       char *host, char *port, char *uri);

int main(int argc, char **argv)
{
    int listenfd, connfd;
    socklen_t clientlen;
    struct sockaddr_in clientaddr;
    char request_buf[MAX_OBJECT_SIZE];
    char host[MAXLINE];
    char port[MAXLINE];
    size_t receive_size;
    listenfd = Open_listenfd(PORT);
    while (1)
    {
        int proxy_client_fd;
        clientlen = sizeof(clientaddr);
        // 接收client发来的链接
        connfd = Accept(listenfd, (SA *)&clientaddr, &clientlen);
        printf("connected to %d\n", clientaddr.sin_addr.s_addr);

        // 处理client发来的链接, 将处理好的内容放在request_buf中
        if (parse_request(connfd, request_buf, host, port) == -1)
        {
            Close(connfd);
            continue;
        }

        printf("received msg from client:\n");
        printf(request_buf);

        // 向对向服务器发送信息
        if ((proxy_client_fd = Open_clientfd(host, port)) < 0)
        {
            continue;
        };

        Rio_writen(proxy_client_fd, request_buf, strlen(request_buf));
        receive_size = Rio_readn(proxy_client_fd, request_buf, MAX_OBJECT_SIZE);
        close(proxy_client_fd);

        printf("received msg from server:\n");
        Rio_writen(STDOUT_FILENO, request_buf, receive_size);

        //将服务器内容发送给client
        Rio_writen(connfd, request_buf, receive_size);

        // 将处理好的内容转发给目标host
        Close(connfd);
        printf("connection is closed\n");
    }
    exit(0);
}

/*
 * parse_request - parse the whole request
 * return -1 if message format is not correct or exceed maximum object size limit
 */
int parse_request(int contfd, char *request_buf, char *host, char *port)
{
    char buf[MAXLINE];
    char parsed_request_line[MAXLINE];
    char header_field[MAXLINE];
    char header_field_value[MAXLINE];
    char abs_path[MAXLINE];
    rio_t rio;

    Rio_readinitb(&rio, contfd);

    /* parse request line */
    if (parse_request_line(&rio, parsed_request_line, host, port, abs_path) == -1)
    {
        return -1;
    }
    printf("request line:\n");
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
    return 0;
}

/*
 * parse_request_line - taking a request line and parse properly
 * fd is connection file descriptor used to transform error handling page
 */
int parse_request_line(rio_t *client_rp, char *parsed_request,
                       char *host, char *port, char *uri)
{
    char request_line[MAXLINE];
    char method[MAXLINE], url[MAXLINE], version[MAXLINE];

    if (!Rio_readlineb(client_rp, request_line, MAXLINE))
        return -1;

    sscanf(request_line, "%s %s %s", method, url, version);

    // Check request method
    if (strcasecmp(method, "GET"))
    {
        clienterror(client_rp->rio_fd, method, "501", "Not Implemented",
                    "The proxy does not implement this method");
        return -1;
    }

    URL_INFO info;
    split_url(&info, url);
    strcpy(host, info.host);
    strcpy(port, info.port);
    strcpy(uri, info.path);

    // Check HTTP version
    if (strcmp("HTTP/1.0", version) && strcmp("HTTP/1.1", version))
    {
        clienterror(client_rp->rio_fd, method, "400", "Bad request",
                    "Invalid HTTP version");
        return -1;
    }
    else
    {
        strcpy(version, "HTTP/1.0");
    }

    sprintf(parsed_request, "%s %s %s\r\n", method, uri, version);
    return 0;
}

#ifdef MY
int parse_request_line(int fd, char *request_line, char *buf, char *host, char *abs_path)

{
    char method[MAXLINE];
    if (sscanf(request_line, "%s http://%[a-zA-Z.]/%s HTTP", method, host, abs_path) < 2)
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
#endif

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
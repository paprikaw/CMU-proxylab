#include <stdio.h>
#include "csapp.h"
// Sequential -> cocurrent ->
/* Recommended max cache and object sizes */
#define MAX_CACHE_SIZE 1049000
#define MAX_OBJECT_SIZE 102400
#define PORT 38888
/* You won't lose style points for including this long line in your code */
static const char *user_agent_hdr = "User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:10.0.3) Gecko/20120305 Firefox/10.0.3";
int transmit(int connfd);
int parse_request_line(int fd, char *request_line, char *buf, char *host, char *abs_path);
void clienterror(int fd, char *cause, char *errnum,
                 char *shortmsg, char *longmsg);
int parse_request(int contfd, char *request_buf, char *url);

int main(int argc, char **argv)
{
    printf("%s", user_agent_hdr);
    int listenfd, connfd, port, clientlen;
    struct sockaddr_in clientaddr;
    struct hostent *hp;
    char *haddrp;
    char request_buf[MAX_OBJECT_SIZE];
    char url[MAXLINE];

    listenfd = Open_listenfd(PORT);
    while (1)
    {
        clientlen = sizeof(clientaddr);
        // 接收client发来的链接
        connfd = Accept(listenfd, (SA *)&clientaddr, &clientlen);
        printf("connected to %s", clientaddr.sin_addr.s_addr);
        /* determine the domain name and IP address of the client */
        // 处理client发来的链接
        parse_request(connfd, request_buf, url);
        Close(connfd);
    }
    exit(0);
}

/*
 * parse_request - parse the header of client request
 */
int parse_request(int contfd, char *request_buf, char *url)
{
    size_t output_space = 0;
    char buf[MAXLINE];
    char parsed_request_line[MAXLINE];
    char header_field[MAXLINE];
    char header_field_value[MAXLINE];
    char host[MAXLINE];
    char abs_path[MAXLINE];
    rio_t rio;
    int is_host_header_present = 0; // Host header是否在request message中的header出现过

    Rio_readinitb(&rio, contfd);
    Rio_readlineb(&rio, buf, MAXLINE);
    /* parse request line */
    if (parse_request_line(contfd, buf, parsed_request_line, host, abs_path) == -1)
    {
        return -1;
    };

    /* Add the default value for crutial header fields */
    sprintf(request_buf, parsed_request_line);
    sprintf(request_buf, "%sUser-Agent: %s\r\n", request_buf, user_agent_hdr);
    sprintf(request_buf, "%sConnection: close\r\n", request_buf);
    sprintf(request_buf, "%sProxy-Connection: close\r\n", request_buf);
    /* Start to read the whole header fields*/
    Rio_readlineb(&rio, buf, MAXLINE);
    while (strcmp(buf, "\r\n"))
    {
        sscanf(buf, "%s: %s", header_field, header_field_value);
        // 如果是这几种已经设置好的header fields，就直接跳过
        if (
            (strcasecmp(header_field, "User-Agent") == 0) ||
            (strcasecmp(header_field, "Connection") == 0) ||
            (strcasecmp(header_field, "Proxy-Connection") == 0))
        {
            continue;
        }

        // 如果是Host，那么直接使用request header中的Host字段
        if ((strcasecmp(header_field, "Host") == 0))
        {
            is_host_header_present = 1;
        }
        sprintf(request_buf, "%s%s", request_buf, buf);
        Rio_readlineb(&rio, buf, MAXLINE);
    }

    // 没有Host字段，使用从url中获取的host字段
    if (!is_host_header_present)
    {
        sprintf(request_buf, "%sHost: %s\r\n", request_buf, host);
    }
    // 将剩下entity body部分写入
    sprintf(request_buf, "\r\n");
    Rio_readlineb(&rio, buf, MAXLINE);
    sprintf(request_buf, buf);
    return 0;
}

/*
 * parse_request_line - taking a request line and parse properly
 * fd is connection file descriptor used to transform error handling page
 */
int parse_request_line(int fd, char *request_line, char *buf, char *host, char *abs_path)

{
    char method[MAXLINE];
    if (sscanf(request_line, "%s http://%s/%s HTTP", method, host, abs_path) != 3)
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
 * request_cat - concatinate a string to a partial http request message
 */
void request_cat(size_t *acc, char *src, char *dst)
{
    size_t cat_size = strlen(src) + strlen(dst) + 1;
    (*acc) += cat_size;
    snprintf(dst, MAX_OBJECT_SIZE, "%s%s", dst, src);
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
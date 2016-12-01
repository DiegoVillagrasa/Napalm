#include<stdio.h>
#include<string.h>
#include<stdlib.h>
#include<sys/socket.h>
#include<arpa/inet.h>
#include<unistd.h>
#include<pthread.h>
#include<assert.h>

int main_sock;
int _rewrite = 0;
int _srvPort = 8080;
char *_request_heaader;
char cwd[1024];

void *_handler(void *);
int main(int argc , char *argv[])
{
    int socket_desc , client_sock , c , *new_sock;
    struct sockaddr_in server , client;
    int _online = 0;

    if (getcwd(cwd, sizeof(cwd)) != NULL)
    {
       strcat(cwd, "/");
    }
    else
    {
      perror("Could not get work dir");
      exit(0);
    }

    while ((++argv)[0])
    {
            if (argv[0][0] == '-' )
            {
                    switch (argv[0][1])  {

                            default:
                                    printf("Unknown option -%c\n\n", argv[0][1]);
                                    exit(0);
                                    break;
                            case 'o':
                                    _online = 1;
                                    break;
                            case 'r':
                                    _rewrite = 1;
                                    break;
                    }
            }
            else{
              _srvPort = atoi(argv[0]);
              if(_srvPort == 0){
                puts("Wrong server port specified");
                exit(0);
              }
            }
    }
    main_sock = socket_desc;
    socket_desc = socket(AF_INET , SOCK_STREAM , 0);
    if (socket_desc == -1)
    {
        printf("Could not create socket");
    }

    server.sin_family = AF_INET;
    if(_online == 0)
    {
      server.sin_addr.s_addr = inet_addr("127.0.0.1");
    }
    else
    {
      server.sin_addr.s_addr = INADDR_ANY;
    }
    server.sin_port = htons(_srvPort);

    if( bind(socket_desc,(struct sockaddr *)&server , sizeof(server)) < 0)
    {
        perror("bind failed. Error");
        return 1;
    }

    listen(socket_desc , 3);
    puts(" ");
    if(_online == 1)
      printf("Server started in Online mode on port: %d\n", _srvPort);
    else
      printf("Server started in Local mode on port: %d\n", _srvPort);
    c = sizeof(struct sockaddr_in);
    while( (client_sock = accept(socket_desc, (struct sockaddr *)&client, (socklen_t*)&c)) )
    {
        pthread_t sniffer_thread;
        new_sock = malloc(1);
        *new_sock = client_sock;

        if( pthread_create( &sniffer_thread , NULL ,  _handler , (void*) new_sock) < 0)
        {
            perror("Could not create thread");
            return 1;
        }
    }
    if (client_sock < 0)
    {
        perror("accept failed");
        return 1;
    }
    printf("Exiting top");
    return 0;
}
char* concat(const char *s1, const char *s2)
{
    char *result = malloc(strlen(s1)+strlen(s2)+1);
    strcpy(result, s1);
    strcat(result, s2);
    return result;
}
void removeSubstring(char *s,const char *toremove)
{
  while( s=strstr(s,toremove) )
    memmove(s,s+strlen(toremove),1+strlen(s+strlen(toremove)));
}
void rand_str(char *dest, size_t length) {
    char charset[] = "0123456789"
                     "abcdefghijklmnopqrstuvwxyz";

    while (length-- > 0) {
        size_t index = (double) rand() / RAND_MAX * (sizeof charset - 1);
        *dest++ = charset[index];
    }
    *dest = '\0';
}
void *_handler(void *socket_desc)
{
    int sock = *(int*)socket_desc;
    int read_size;
    char *message , buffer[1048];

    while( (read_size = recv(sock , buffer , 1048 , 0)) > 0 )
    {
      char *_req;
      char *_reqType;
      char *_reqFile;
      char _bfr[1048];
      strcpy(_bfr, buffer);

      int _reqPointer = 0;
      _req = strtok(buffer, " ");
      int i;
      for(i = 0; i < 2; i++)
      {
        if(i == 0){
          _reqType = _req;
        }
        else if(i == 1){
          _reqFile = _req;
        }
        _req = strtok (NULL, " ");
        _reqPointer++;
      }
      char *f_file;
      if(strlen(_reqFile) == 1 && _reqFile[0] == '/')
      {
        if( access( concat(cwd,"index.php"), F_OK ) != -1 ) {
            f_file = "index.php";
        } else {
            f_file = "index.html";
        }
      }
      else{
        f_file = memmove(_reqFile, _reqFile+1, strlen(_reqFile));
        if(f_file[strlen(f_file)-1] == '/')
        {
          if(_rewrite == 1)
          {
            f_file = "index.php";
            printf("Rewriting Url");
          }
          else
          {
            if( access( concat(cwd,concat(f_file, "index.php")), F_OK ) != -1 ) {
                f_file = concat(f_file, "index.php");
            } else {
                f_file = concat(f_file, "index.html");
            }
          }
        }
      }
      f_file = strdup(concat(cwd, f_file));
      if(strstr(f_file, ".php") != NULL)
      {
        int setCookie = 1;
        char *_reqCookie;
        char *_prt;
        char *token = NULL;
        token = strtok(_bfr, "\n");
        while (token) {
            //printf("Current token: %s.\n", token);
            if(strstr(token, "Cookie: phpsessid=")!= NULL)
            {
              char *tempo;
              tempo = malloc(strlen(token)+1);
              strcpy(tempo,token);
              removeSubstring(tempo, "Cookie: phpsessid=");
              _reqCookie = malloc(strlen(tempo)+1);
              strcpy(_reqCookie,tempo);
              setCookie = 0;
            }
            token = strtok(NULL, "\n");
        }
        FILE *fp;
        char path[8928];
        if(setCookie == 1)
        {
          _reqCookie = malloc(33);
          rand_str(_reqCookie, 32);
        }
        size_t _lenF = strlen(f_file);
        size_t _reqLength = strlen("/usr/bin/php -r '$_COOKIE[\"PHPSESSID\"] = \"") + strlen(_reqCookie) + strlen("\"; require(\"\");'") +strlen(f_file) + 1;
        char *_phpSess;
        _phpSess = malloc(_reqLength);
        memcpy(_phpSess, "/usr/bin/php -r '$_COOKIE[\"PHPSESSID\"] = \"", strlen("/usr/bin/php -r '$_COOKIE[\"PHPSESSID\"] = \""));
        memcpy(_phpSess + strlen("/usr/bin/php -r '$_COOKIE[\"PHPSESSID\"] = \""), _reqCookie, 32);
        memcpy(_phpSess + 42 + 32, "\"; require(\"", 12);
        memcpy(_phpSess + 42 + 32 +12, f_file, _lenF);
        memcpy(_phpSess + 42 + 32 + 12 + _lenF, "\");'", 5);
        fp = popen(_phpSess, "r");

        if (fp == NULL) {
          printf("Failed to run PHP\n" );
        }
        else{
          send(sock, "HTTP/1.1 200 OK\n", 16, MSG_NOSIGNAL);
          send(sock, "Server: Napalm/0.0.1\n", 21, MSG_NOSIGNAL);
          if(setCookie == 1)
          {
            char _sessID[33];
            rand_str(_sessID, 32);
            char *_cookieSet = concat("Set-Cookie: phpsessid=", _sessID);
            send(sock, _cookieSet, strlen(_cookieSet), MSG_NOSIGNAL);
            send(sock, "\n", 1, MSG_NOSIGNAL);
          }
          send(sock, "Content-Type: text/html\n\n", 25, MSG_NOSIGNAL);
          while (fgets(path, sizeof(path)-1, fp) != NULL) {
            send(sock, path, strlen(path), MSG_NOSIGNAL);
          }
        }
        pclose(fp);
      }
      else{
        int c;
        char t[2];
        FILE *file;
        file = fopen(f_file, "r");

        if (file) {
          send(sock, "HTTP/1.1 200 OK\n", 16, MSG_NOSIGNAL);
          send(sock, "Server: Napalm/0.0.1\n", 21, MSG_NOSIGNAL);
          if(strstr(f_file, ".css") != NULL){
            send(sock, "Content-Type: text/css\n\n", strlen("Content-Type: text/css\n\n"), MSG_NOSIGNAL);
          }
          else if(strstr(f_file, ".js") != NULL){
            send(sock, "Content-Type: text/javascript\n\n", strlen("Content-Type: text/javascript\n\n"), MSG_NOSIGNAL);
          }
          else if(strstr(f_file, ".svg") != NULL){
            send(sock, "Content-Type: image/svg+xml\n\n", strlen("Content-Type: image/svg+xml\n\n"), MSG_NOSIGNAL);
          }
          else if(strstr(f_file, ".png") != NULL){
            send(sock, "Content-Type: image/png\n\n", strlen("Content-Type: image/png\n\n"), MSG_NOSIGNAL);
          }
          else if(strstr(f_file, ".jpg") != NULL){
            send(sock, "Content-Type: image/jpeg\n\n", strlen("Content-Type: image/jpeg\n\n"), MSG_NOSIGNAL);
          }
          else if(strstr(f_file, ".mp4") != NULL){
            send(sock, "Content-Type: video/mp4\n\n", strlen("Content-Type: video/mp4\n\n"), MSG_NOSIGNAL);
          }
          else if(strstr(f_file, ".ogg") != NULL){
            send(sock, "Content-Type: video/ogg\n\n", strlen("Content-Type: video/ogg\n\n"), MSG_NOSIGNAL);
          }
          else if(strstr(f_file, ".webm") != NULL){
            send(sock, "Content-Type: video/webm\n\n", strlen("Content-Type: video/webm\n\n"), MSG_NOSIGNAL);
          }
          else{
            send(sock, "Content-Type: text/html\n\n", 25, MSG_NOSIGNAL);
          }
          while ((c = getc(file)) != EOF) //To Do: Use a buffer
          {
              t[0] = c;
              send(sock, t, 1, MSG_NOSIGNAL);
          }
          fclose(file);
          send(sock, "\n", 1, MSG_NOSIGNAL);
          printf("%s %s OK 200\n", _reqType, _reqFile);
        }
        else{
          send(sock, "HTTP/1.1 404 Not Found\n\n", strlen("HTTP/1.1 404 Not Found\n\n"), MSG_NOSIGNAL);
          printf("Request of type: %s for file: %s ERROR 404\n", _reqType, _reqFile);
        }
      }
      close(sock);
      free(socket_desc);
    }

    if(read_size == 0)
    {
        puts("Client disconnected");
        close(sock);
        fflush(stdout);
    }
    else if(read_size == -1)
    {
      close(sock);
    }
    return 0;
}

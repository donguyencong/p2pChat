//Usage: To make 2 local server to communicate:
//terminal 1: app 8000 5000
//terminal 2: app 5000 8000

#include <sys/socket.h>
#include <netdb.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <strings.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>


pthread_mutex_t count_mutex;

int count = 0;

void *client(void *client_ptr)
{
  int sockfd, port;
  struct sockaddr_in servaddr;
  char * server = "127.0.0.1";
  port = *((int *) client_ptr);

  /* Create a new UDP socket */
  if (-1 == (sockfd = socket(AF_INET, SOCK_DGRAM, 0))) {
    perror("socket creation");
  }

  /* Initialize the server address */
  bzero(&servaddr, sizeof(struct sockaddr_in));
  servaddr.sin_family = AF_INET;
  if (INADDR_NONE == (servaddr.sin_addr.s_addr = inet_addr(server))) {
    perror(server);
  }
  servaddr.sin_port = htons(port);

  /* Send a message to the server and wait for a reply */
  static const int MAX_MESSAGE_LENGTH = 80;
  char buff[MAX_MESSAGE_LENGTH];
  ssize_t len;
  socklen_t addrlen;
  for(;;){
    //strncpy(buff, "Hello, world!", sizeof(buff));
    printf("Enter message: ");
    fgets(buff, MAX_MESSAGE_LENGTH, stdin);
    if (buff[0] == '%' && buff[1] == 's' && buff[2] == 't' && buff[3] == 'o' && buff[4] == 'p')
    {
      sendto(sockfd, buff, strlen(buff), 0, (struct sockaddr *)&servaddr, sizeof(servaddr));
      printf("Exit\n");
      close(sockfd);
      exit(1);
      break;
    }
    
    printf("Sending to server with port %d\n", port);
    if (-1 == sendto(sockfd, buff, strlen(buff), 0, 
         (struct sockaddr *)&servaddr, sizeof(servaddr))
        || -1 == (len = recvfrom(sockfd, buff, sizeof(buff), 0, 
               (struct sockaddr *)&servaddr, &addrlen))) {
     perror("send/receive");  
    buff[len] = 0;
     }
    pthread_mutex_unlock(&count_mutex);
    count = count - 1;
    pthread_mutex_lock(&count_mutex);
    printf("Sent, current count:%d \n", count );
  } 
    
  /* The reply arrived; NULL-terminate it */

  close(sockfd);
  return NULL;
}

void *server(void *server_ptr)
{
  int sockfd;
  int port = *((int *) server_ptr);
  struct sockaddr_in servaddr;
  int serverLen=sizeof(servaddr);
  //printf("%d\n", port );

  /* Create a new UDP socket */
  if (-1 == (sockfd = socket(AF_INET, SOCK_DGRAM, 0))) {
    perror("socket creation");
  }

  
  /* Initialize the address and bind the socket */
  //bzero(&servaddr, sizeof(servaddr));
  servaddr.sin_family = AF_INET;
  servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
  servaddr.sin_port = htons(port);
  if((bind(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr))) != 0) {
    perror("socket bind");
  }

  

  
  /* Define variables for sending/receiving */
  static const int MAX_MESSAGE_LENGTH = 80;
  char buff[MAX_MESSAGE_LENGTH];
  struct sockaddr_in cliaddr;
  ssize_t len;
  socklen_t cliaddr_len = sizeof(cliaddr);
  for(;;){
    /* Receive a message */
    bzero(&cliaddr, sizeof(cliaddr));
    
    if (-1 == (len = recvfrom(sockfd, buff, sizeof(buff), 0, 
            (struct sockaddr *)&cliaddr, &cliaddr_len))) {
      perror("receive");
    }
    if (buff[0] == '%' && buff[1] == 's' && buff[2] == 't' && buff[3] == 'o' && buff[4] == 'p')
    {
      printf("Exit\n");
      close(sockfd);
      break;
    }
    pthread_mutex_unlock(&count_mutex);
    count = count + 1;
    pthread_mutex_lock(&count_mutex);
    /* NULL-terminate and display the message */
    buff[len] = 0;
    printf("(-%d)> Client sends: %s\n",count, buff);
    if (-1 == sendto(sockfd, buff, strlen(buff), 0, 
         (struct sockaddr *)&cliaddr, sizeof(cliaddr))) {
      perror("send");
    }
  }
  close(sockfd);
  return NULL;
}



int main(int argc, char const *argv[])
{
  pthread_t client_thread;
  pthread_t server_thread;
  int port, host;
  if (argc != 3) {
    fprintf(stderr, "Usage: [Receive from port] [Send to port]\n");
    return 1;
  }

  /* Is the port number provided? If not, use 8088 */

  if (argc == 3) {
    char *endp;
    char *endp2;
    host = strtol(argv[1], &endp, 10);
    port = strtol(argv[2], &endp2, 10);

    if (endp == argv[2] || *endp) {
      fputs("Port number must be integer\n", stderr);
      return 1;
    }
  } else
    port = 8088;

  printf("Receive from port: %d, send to port: %d \n", host, port);

  if(pthread_create(&server_thread, NULL, server, (void *) &host)) {
    fprintf(stderr, "Error creating thread\n");
    return 1;
  }
  if(pthread_create(&client_thread, NULL, client,(void *) &port)) {
    fprintf(stderr, "Error creating thread\n");
    return 1;
  }
  if(pthread_join(client_thread, NULL)) {
    fprintf(stderr, "Error joining thread\n");
    return 2;
  }
  if(pthread_join(server_thread, NULL)) {
    fprintf(stderr, "Error joining thread\n");
    return 2;
  }
  
  return 0;
}


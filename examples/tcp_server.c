#include <stdlib.h>
#include <stdio.h>
#include <strings.h>
#include <arpa/inet.h>
#include <math.h>
#include <unistd.h>

#define MY_PORT         9999

int main()
{int sockfd;
 struct sockaddr_in self;
 float *buffer;
 struct sockaddr_in client_addr;
 int clientfd;
 socklen_t addrlen=sizeof(client_addr);
 int taille,k;

 sockfd = socket(AF_INET, SOCK_STREAM, 0);  // ICI LE TYPE DE SOCKET

 bzero(&self, sizeof(self));
 self.sin_family = AF_INET;
 self.sin_port = htons(MY_PORT);
 self.sin_addr.s_addr = INADDR_ANY;

 bind(sockfd, (struct sockaddr*)&self, sizeof(self));
 listen(sockfd, 20);
 
 while (1) {
  clientfd = accept(sockfd, (struct sockaddr*)&client_addr, &addrlen);
  printf("%s:%d connected\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
  while (taille!=-1)
    {recv(clientfd, &taille, sizeof(long), 0);
     taille=ntohl(taille);
     printf("%d\n",taille);
     if (taille>0)
        {buffer=(float*)malloc(sizeof(float)*taille);
         for (k=0;k<taille/2;k++) {buffer[2*k]=sin(2*M_PI*(float)k/(float)taille*10);
                                   buffer[2*k+1]=cos(2*M_PI*(float)k/(float)taille*10);
                                  }
         send(clientfd, buffer, taille*sizeof(float), 0);
        }
    }
  close(clientfd);
 }
 close(sockfd);return(0);  // Clean up (should never get here)
}

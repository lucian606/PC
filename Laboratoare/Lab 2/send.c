#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include "link_emulator/lib.h"

#define HOST "127.0.0.1"
#define PORT 10000


int main(int argc,char** argv){
  init(HOST,PORT);
  char buf[1396];
  int readBytes;
  msg t;
  pkt p;
  if(argc < 2) {
    printf("No file parameter given!");
    return -2;
  }
  int fd=open(argv[1],O_RDONLY);
  if(fd < 0) {
    printf("File can't be opened error!");
  }
  strcpy(t.payload, argv[1]); //Argv[1] (nume fisier) se pune in payload
  memcpy(p.payload,t.payload,strlen(t.payload));  //Bag in pachet titlul
  p.type=1; //1->Mesaj de trimis;
  t.len=strlen(t.payload)+4;
  memcpy(t.payload, &p, sizeof(pkt)); //Bag pachetu in mesaj
  //printf("%s", t.payload+4); //Primii 4 octeti sunt ocupati de int, la payload+4 e mesaju;
  printf("Sending title ...\n");
  send_message(&t); //Trimitem tiltul
  if(recv_message(&t) < 0) {
    printf("Waiting for ACK...");
    return -1;
  }
  else {
    int end_of_file=lseek(fd,0,SEEK_END); //Calculam dimensiunea fisierului
    lseek(fd,0,SEEK_CUR);
    //printf("%d", end_of_file);
    char size[100];
    sprintf(size, "\n%d\n", end_of_file);  //O convertim la string
    strcpy(t.payload, size);
    memset(p.payload, 0, strlen(p.payload));
    p.type=1;
    memcpy(p.payload, t.payload, strlen(t.payload));  //O facem pachet
    t.len=strlen(p.payload)+4;
    memcpy(t.payload, &p, sizeof(pkt)); //Il facem mesaj
    printf("Sending size ... \n");
    send_message(&t); //Il trimitem
  }
  close(fd);  //Inchidem fisierul
  fd=open(argv[1], O_RDONLY); //Il redeschidem
  lseek(fd, 0, SEEK_CUR);
  readBytes=1;
  while(recv_message(&t) && readBytes > 0) {  //Cat timp citim/primim ACK
    readBytes=read(fd,buf,1395); //Citesc din fisier 1395 bytes, iar 1396 e '\0'    printf("%d=== %ld\n",readBytes,strlen(buf));
      if(readBytes <= 0) {
        printf("All info sent.\n");
        return 1;
      }
      else {
        strcpy(t.payload,buf);  //Copiez textu in mesaj
        memset(p.payload, 0, 1396); //Curat pachetul
        p.type=1;
        memcpy(p.payload, t.payload, readBytes);  //Impachetez
        t.len=readBytes+4;  //Adaug 4 pt ca int
        memcpy(t.payload, &p, sizeof(pkt)); //O facem mesaj
        printf("Sending info ... \n");
        send_message(&t);
      }
  }     
  return 0;
}


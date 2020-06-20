#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include "link_emulator/lib.h"

#define HOST "127.0.0.1"
#define PORT 10001


int main(int argc,char** argv){
  init(HOST,PORT);
  msg r;
  pkt p;
  int ft=open("Destination_File.txt", O_WRONLY | O_CREAT, 0644);
  int ok=recv_message(&r); //Primim mesajul (titlul)
  if(ok < 0) {
    printf("Error receiving message!");
    return -1;
  } //Verificam daca a ajuns cu bine
  memcpy(&p, r.payload, sizeof(pkt)); //Il facem paket
  if(ft < 0) {
    printf("Destination can't be opened!");
    return -1;
  } //Verificam daca putem deschite fisierul de output
  if(p.type == 1) {
    printf("Title received\n");
    write(ft, p.payload, strlen(p.payload));  //Scriem mesaju in fisier
    char ACK_Message[]="ACK";
    strcpy(p.payload, ACK_Message);
    memcpy(r.payload, &p, sizeof(pkt)); //Creem mesajul de confirmare
    printf("%s\n", r.payload+4);
    send_message(&r); //Trimitem mesajul de confirmare (ACK)
  }
  ok=recv_message(&r);  //Primim sizeul
  if(ok < 0) {
    printf("Error receiving message!");
    return -1;
  } //Verificam daca a ajuns cu bine
  memcpy(&p, r.payload, sizeof(pkt)); //Il facem pachet
  if(p.type == 1) {
    printf("Size received\n");
    write(ft, p.payload, strlen(p.payload));
    char ACK_Message[]="ACK";
    strcpy(p.payload, ACK_Message);
    memcpy(r.payload, &p, sizeof(pkt)); //Creem mesajul de confirmare
    printf("%s\n", r.payload+4);
    send_message(&r); //Trimitem mesajul de confirmare (ACK)
  }

  while (recv_message(&r) >= 0) { //Cat timp citim din fisier
    memcpy(&p, r.payload, sizeof(pkt));
    if(p.type == 1) {
      printf("Info received\n");
      write(ft, p.payload, strlen(p.payload));        
      char ACK_Message[]="ACK";
      strcpy(p.payload, ACK_Message);
      memcpy(r.payload, &p, sizeof(pkt)); //Creem mesajul de confirmare
      printf("%s\n", r.payload+4);
      send_message(&r); //Trimitem mesajul de confirmare (ACK)
    }
  }
  
  
  return 0;
}

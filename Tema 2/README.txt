324CC Iliescu Lucian-Marius

Server.cpp

-am creat socketi pentru conexiunile UDP si TCP, facand multiplexare I/O
-serverul ruleaza intr-o bucla infinita, din care se iese folosind comanda exit
-atunci cand serverul primeste un mesaj de la un client UDP, acesta il decodeaza
si il trimite subscriberilor care sunt abonati la topicul respectiv
-in implementare am creat o structura pentru user, care are id-ul,file
descriptorul, statusul lui de online/offline si o coada in care sunt retinute
mesajele pe care acesta le-a primit cat timp el a fost deconectat de la
topicurile cu care acesta este abonat in modul S&F
-am creat o structura pentru topic, cu numele, si doi vectori de useri, unul
pentru fiecare tip de abonat
-am folosit 3 unordered_map, unul in care identificam useri prin id, altul
prin care identificam topicurile dupa nume si altul in care legam id-ul
userilor de file-descriptorul lor, nu conteaza ordinea datelor deci puteam
folosi unordered_map datorita operatiilor de cautare si inserare care au loc
in O(1) spre deosebire de map care are O(log n)
-cand un client se conecteaza la server, verific daca se conecteaza pentru
prima oara (caz in care il bag in hash-mapul de useri si ii leg id-ul de file
descriptor), daca se reconecteaza (caz in care actualizez id-ul cu noul file
descriptor si afisez mesajele adunate in coada cat timp el a fost deconectat) 
sau daca e deja conectat (caz in care sunt doi useri conectati cu acelasi id) 
si in functie de cum a decurs conectarea, trimit feedback la client
-atunci cand se aboneaza un client la un topic, se adauga topicul in multimea
de topicuri (daca nu exista deja) si se adaug utilizatorul in vectorul aferent
optiunii de S&F pe care acesta a ales-o, din vectorii topicului
-atunci cand se dezaboneaza un client, se cauta clientul in vectorii userilor
si se scoate din vectorul aferent
-cand un client se deconecteaza (nu s-a primit niciun byte), inchid conexiunea
cu acesta
-atunci cand se da comanda exit, inchid conexiunea cu toti socketii

Posbile erori

-cand se conecteaza doi utilizatori simultan cu acelasi id, se inchide
conexiunea cu ultimul care s-a conectat
-cand se aboneaza un user la un topic la care e deja abonat (trimt mesaj de
eroare)
-cand se dezaboneaza un user de la un topic la care nu e abonat (trimit mesaj
de eroare)

Subscriber.cpp

-am creat sokcetul pentru conexiunea cu serverul, facand multiplexare I/O
-trimit id-ul userului catre server pentru validare
-clientul ruleaza intr-o bulca infinita, din care se iese folosind comanda exit
-am folosit o strucutra de request pentru comenzile de subscribe/unsubcrie, pe
care clientul o trimite la server, iar server ii trimite o structura de tip
reply, care ii spune daca s-a desfasurat actiunea cu succes sau nu
-daca se da comanda de subscribe, verific celelalte argumente daca sunt valide,
caz in care trimit o cerere catre server
-daca se da comanda de unsubscribe, trimit cerere catre server
-daca se da comanda de exit, ies din bucla infinita si inchid conexiunea cu
serverul
-cand se primeste un mesaj de la server, fac cast de la char* la tcp_msg*
si afisez continutul

Posibile erori

-cand userul are id-ul mai mare decat 10 caractere inchid clientul
-cand topicul are mai mult decat 50 de caractere
-cand userul se conecteaza pe un id care deja este conectat
-cand userul trimite o comanda invalida, acesta va fi anuntat si i se vor
afisa comenzile incorecte

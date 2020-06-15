# Protocoale de comunicatii:
# Laborator 8: Multiplexare
# Makefile

CFLAGS = -Wall -g

# Portul pe care asculta serverul (de completat)
PORT = 12345

# Adresa IP a serverului (de completat)
IP_SERVER = 127.0.0.1

build: server.cpp subscriber.cpp
	g++ -Wall -o server server.cpp -lm
	g++ -Wall -o subscriber subscriber.cpp -lm

# Ruleaza serverul
run_server:
	./server ${PORT}

clean:
	rm -f server subscriber

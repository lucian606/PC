#include "skel.h"
#include <stdlib.h>
#include "tree.h"

#define IP_OFF (sizeof(struct ether_header))
#define ICMP_OFF (IP_OFF + sizeof(struct iphdr))

struct route_table_entry {
	uint32_t prefix;
	uint32_t next_hop;
	uint32_t mask;
	int interface;
} __attribute__((packed));

struct arp_entry {
	__u32 ip;
	uint8_t mac[6];
};

struct route_table_entry *rtable;
int rtable_size;

struct arp_entry* arp_table;
int arp_table_len=0;

uint16_t ip_checksum(void* vdata,size_t length) {
	// Cast the data pointer to one that can be indexed.
	char* data=(char*)vdata;

	// Initialise the accumulator.
	uint64_t acc=0xffff;

	// Handle any partial block at the start of the data.
	unsigned int offset=((uintptr_t)data)&3;
	if (offset) {
		size_t count=4-offset;
		if (count>length) count=length;
		uint32_t word=0;
		memcpy(offset+(char*)&word,data,count);
		acc+=ntohl(word);
		data+=count;
		length-=count;
	}

	// Handle any complete 32-bit blocks.
	char* data_end=data+(length&~3);
	while (data!=data_end) {
		uint32_t word;
		memcpy(&word,data,4);
		acc+=ntohl(word);
		data+=4;
	}
	length&=3;

	// Handle any partial block at the end of the data.
	if (length) {
		uint32_t word=0;
		memcpy(&word,data,length);
		acc+=ntohl(word);
	}
	while (acc>>16) {
		acc=(acc&0xffff)+(acc>>16);
	}

	// If the data began at an odd byte address
	// then reverse the byte order to compensate.
	if (offset&1) {
		acc=((acc&0xff00)>>8)|((acc&0x00ff)<<8);
	}

	// Return the checksum in network byte order.
	return htons(~acc);
}


void parse_rtable() {
	FILE *f;
	int rtable_capacity=5;
	rtable_size=0;
	rtable=(struct route_table_entry*)malloc(5 * sizeof(struct route_table_entry));
	f=fopen("rtable.txt","r");
	char prefix[50], next_hop[50], mask[50], buffer[1000];
	int interface;
	if(f == NULL)
		perror("File can't be opened.\n");
	else {
		while (fgets(buffer, 1000, f)) { //Reading the current line
			sscanf(buffer, "%s%s%s%d", prefix, next_hop, mask, &interface);
			if(rtable_capacity == rtable_size) {
				rtable_capacity++;
				rtable=(struct route_table_entry*)realloc(rtable, rtable_capacity * 
						sizeof(struct route_table_entry));	
			} //Realloc if necessary
			inet_pton(AF_INET,prefix,&rtable[rtable_size].prefix);
			inet_pton(AF_INET,next_hop,&rtable[rtable_size].next_hop);
			inet_pton(AF_INET,mask,&rtable[rtable_size].mask);
			rtable[rtable_size].interface=interface;
			rtable_size++;
		}	//We parse the addresses from strings to unsigned integers
	}
	fclose(f);
} //Parses the rtable from the txt file into the rtable array

void parse_arp_table() {
	FILE *f;
	fprintf(stderr, "Parsing ARP table\n");
	f = fopen("arp_table.txt", "r");
	DIE(f == NULL, "Failed to open arp_table.txt");
	char line[100];
	int i = 0;
	for(i = 0; fgets(line, sizeof(line), f); i++) {
		char ip_str[50], mac_str[50];
		sscanf(line, "%s %s", ip_str, mac_str);
		fprintf(stderr, "IP: %s MAC: %s\n", ip_str, mac_str);
		arp_table[i].ip = inet_addr(ip_str);
		int rc = hwaddr_aton(mac_str, arp_table[i].mac);
		DIE(rc < 0, "invalid MAC");
	}
	arp_table_len = i;
	fclose(f);
	fprintf(stderr, "Done parsing ARP table.\n");
} //Parses the arp_table from the txt file into the arp_table array

struct route_table_entry *get_best_route(Tree *trie,__u32 dest_ip) {
	int index = -1;
	longestMatchingPrefix(trie, &index, htonl(dest_ip), 0);
	if(index == -1)
		return NULL;
	else
		return &rtable[index];
} //Finds the entry with the highest mask for which (entry.mask & dest_ip == entry.prefix)


struct arp_entry *get_arp_entry(__u32 ip) {
	for(int i=0; i < arp_table_len; i++)
		if(arp_table[i].ip == ip)
			return (&arp_table[i]);
    return NULL;
} //Finds the arp_entry for the ip

int is_interface(__u32 ip) {
	int ok = 0;
	int i;
	for(i = 0 ; i < ROUTER_NUM_INTERFACES; i++) {
		char checked_ip[30];
		inet_ntop(AF_INET, &ip, checked_ip, 30);
		if(strcmp(checked_ip, get_interface_ip(i)) == 0) {
			ok = 1;
			break;
		}
	}
	return ok;
} //Checks if an ip belongs to an interface

void init_packet(packet *pkt)
{
	memset(pkt->payload, 0, sizeof(pkt->payload));
	pkt->len = 0;
} //Initializes a new packet


int main(int argc, char *argv[])
{
	setvbuf(stdout, NULL, _IONBF, 0); //Clearing the output buffer
	packet m;
	
	int rc;
	
	init();
	parse_rtable();

    Tree *bit_trie = NULL;
    bit_trie = init_tree(bit_trie);
	
	for(int i = 0;  i < rtable_size; i++) {
        uint32_t x = htonl(rtable[i].prefix);
        bit_trie = add_node(bit_trie, i, x, get_number_of_bits(rtable[i].mask), 0);
    }	//Adding the entries to the bit_trie

	arp_table=(struct arp_entry*)malloc(sizeof(struct arp_entry)*100);
	parse_arp_table();

	while (1) {
		setvbuf(stdout, NULL, _IONBF, 0);
		rc = get_packet(&m);
		DIE(rc < 0, "get_message");

		struct ether_header *eth_hdr = (struct ether_header *)m.payload;

		if(ntohs(eth_hdr->ether_type) == ETHERTYPE_ARP)
			continue; //Throwing the package if it is an ARP PACKAGE

		struct iphdr *ip_hdr = (struct iphdr *)(m.payload + IP_OFF);
		struct icmphdr *icmp_hdr = (struct icmphdr*)(m.payload + ICMP_OFF);

		if(ip_checksum(ip_hdr, sizeof(struct iphdr)))
			continue; //Checking the checksum

		if(ip_hdr->ttl <= 1) {
			packet icmp_pkt;
			init_packet(&icmp_pkt);
			
			struct ether_header *icmp_eth_hdr = (struct ether_header *)icmp_pkt.payload;
			struct iphdr *icmp_ip_hdr = (struct iphdr *)(icmp_pkt.payload + IP_OFF);
			struct icmphdr *icmp_icmp_hdr = (struct icmphdr*)(icmp_pkt.payload + ICMP_OFF);
			
			icmp_eth_hdr->ether_type = htons(ETHERTYPE_IP);
			memcpy(icmp_eth_hdr->ether_dhost, eth_hdr->ether_shost, sizeof(eth_hdr->ether_shost));
			memcpy(icmp_eth_hdr->ether_shost, eth_hdr->ether_dhost, sizeof(eth_hdr->ether_dhost));
			
			icmp_ip_hdr->version = 4;
			icmp_ip_hdr->ihl = 5;
			icmp_ip_hdr->tos=0;
			icmp_ip_hdr->id = ip_hdr->id;
			icmp_ip_hdr->tot_len = htons(sizeof(struct iphdr) + sizeof(struct icmphdr));
			icmp_ip_hdr->ttl = 64;
			icmp_ip_hdr->protocol = IPPROTO_ICMP;
			icmp_ip_hdr->daddr = ip_hdr->saddr;
			icmp_ip_hdr->saddr = ip_hdr->daddr;
			icmp_ip_hdr->check = 0;
			icmp_ip_hdr->check = ip_checksum(icmp_ip_hdr, sizeof(struct iphdr));
			
			icmp_icmp_hdr->code = 0;
			icmp_icmp_hdr->type = ICMP_TIME_EXCEEDED;
			icmp_icmp_hdr->checksum = 0;
			icmp_icmp_hdr->un.echo.id = icmp_hdr->un.echo.id;
			icmp_icmp_hdr->checksum=ip_checksum(icmp_icmp_hdr, sizeof(struct icmphdr));

			icmp_pkt.len = sizeof(struct ether_header) + sizeof (struct iphdr) + sizeof(struct icmphdr);
			send_packet(m.interface, &icmp_pkt);
			continue;
		} //Checking if the ttl is >= 1, in that case we send an ICMP TIMEOUT MESSAGE

		
		struct route_table_entry* best=get_best_route(bit_trie ,ip_hdr->daddr);
		//Finding the best route efficiently using our trie 

		if (best == NULL) {
			packet icmp_pkt;
			init_packet(&icmp_pkt);

			struct ether_header *icmp_eth_hdr = (struct ether_header *)icmp_pkt.payload;
			struct iphdr *icmp_ip_hdr = (struct iphdr *)(icmp_pkt.payload + IP_OFF);
			struct icmphdr *icmp_icmp_hdr = (struct icmphdr*)(icmp_pkt.payload + ICMP_OFF);
			
			icmp_eth_hdr->ether_type = htons(ETHERTYPE_IP);
			memcpy(icmp_eth_hdr->ether_dhost, eth_hdr->ether_shost, sizeof(eth_hdr->ether_shost));
			memcpy(icmp_eth_hdr->ether_shost, eth_hdr->ether_dhost, sizeof(eth_hdr->ether_dhost));
			
			icmp_ip_hdr->version = 4;
			icmp_ip_hdr->ihl = 5;
			icmp_ip_hdr->tos=0;
			icmp_ip_hdr->id = ip_hdr->id;
			icmp_ip_hdr->tot_len = htons(sizeof(struct iphdr) + sizeof(struct icmphdr));
			icmp_ip_hdr->ttl = 64;
			icmp_ip_hdr->protocol = IPPROTO_ICMP;
			icmp_ip_hdr->daddr = ip_hdr->saddr;
			icmp_ip_hdr->saddr = ip_hdr->daddr;
			icmp_ip_hdr->check = 0;
			icmp_ip_hdr->check = ip_checksum(icmp_ip_hdr, sizeof(struct iphdr));
			
			icmp_icmp_hdr->code =0;
			icmp_icmp_hdr->type= ICMP_DEST_UNREACH;
			icmp_icmp_hdr->checksum = 0;
			icmp_icmp_hdr->un.echo.id = icmp_hdr->un.echo.id;
			icmp_icmp_hdr->checksum=ip_checksum(icmp_icmp_hdr, sizeof(struct icmphdr));

			icmp_pkt.len = sizeof(struct ether_header) + sizeof (struct iphdr) + sizeof(struct icmphdr);
			icmp_pkt.interface = m.interface;
			send_packet(m.interface, &icmp_pkt);
			continue;
		} //Sending an ICMP message in case there was no best route to be found

		if (icmp_hdr->type == ICMP_ECHO && is_interface(ip_hdr->daddr))  {
			
			packet icmp_pkt;
			init_packet(&icmp_pkt);

			struct ether_header *icmp_eth_hdr = (struct ether_header *)icmp_pkt.payload;
			struct iphdr *icmp_ip_hdr = (struct iphdr *)(icmp_pkt.payload + IP_OFF);
			struct icmphdr *icmp_icmp_hdr = (struct icmphdr*)(icmp_pkt.payload + ICMP_OFF);
			
			icmp_eth_hdr->ether_type = htons(ETHERTYPE_IP);
			memcpy(icmp_eth_hdr->ether_dhost, eth_hdr->ether_shost, sizeof(eth_hdr->ether_shost));
			memcpy(icmp_eth_hdr->ether_shost, eth_hdr->ether_dhost, sizeof(eth_hdr->ether_dhost));
			
			icmp_ip_hdr->version = 4;
			icmp_ip_hdr->ihl = 5;
			icmp_ip_hdr->tos=0;
			icmp_ip_hdr->id = ip_hdr->id;
			icmp_ip_hdr->tot_len = htons(sizeof(struct iphdr) + sizeof(struct icmphdr));
			icmp_ip_hdr->ttl = 64;
			icmp_ip_hdr->protocol = IPPROTO_ICMP;
			icmp_ip_hdr->daddr = ip_hdr->saddr;
			icmp_ip_hdr->saddr = ip_hdr->daddr;
			icmp_ip_hdr->check = 0;
			icmp_ip_hdr->check = ip_checksum(icmp_ip_hdr, sizeof(struct iphdr));
			
			icmp_icmp_hdr->code = 0;
			icmp_icmp_hdr->type = ICMP_ECHOREPLY;
			icmp_icmp_hdr->checksum = 0;
			icmp_icmp_hdr->un.echo.id = icmp_hdr->un.echo.id;
			icmp_icmp_hdr->checksum = ip_checksum(icmp_icmp_hdr, sizeof(struct icmphdr));

			icmp_pkt.len = sizeof(struct ether_header) + sizeof (struct iphdr) + sizeof(struct icmphdr);
			send_packet(m.interface, &icmp_pkt);
			continue;
		} //Sending an ICMP reply for an ICMP request

		ip_hdr->ttl--;
		ip_hdr->check=0;
		ip_hdr->check=ip_checksum(ip_hdr, sizeof(struct iphdr));
		//Updating the ttl and recalculating the checksum

		struct arp_entry* match=get_arp_entry(ip_hdr->daddr);
		memcpy(eth_hdr->ether_dhost, match->mac, 6);
		//Finding the matching ARP and updating Ethernet addresses
		
		send_packet(best->interface, &m);
		//Forwarding the packet to best_route->interface
	}

	free(rtable); //Clearing the route table
	bit_trie = deleteTree(bit_trie); //Clearing the bit trie
}
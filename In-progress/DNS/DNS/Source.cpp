#include <iostream>
#include <winsock.h>
#include <cstdint>

#define NDS_INET 1
#define DNS_A 1
#define DNS_QUERY (0 << 15)		/* 0 = query; 1 = response */ 
#define DNS_RESPONSE (1 << 15)
#define DNS_STDQUERY (0 << 11)	/* opcode - 4 bits */
#define DNS_AA (1 << 10)		/* authoritative answer*/
#define DNS_TC (1 << 9)			/* truncated */
#define DNS_RD (1 << 8)			/* recursion desired */
#define DNS_RA (1 << 7)			/* recursion available */
#define MAX_DNS_LEN 512

using namespace std;

#pragma pack(push, 1)
class queryHeader
{
public:
	USHORT qType;
	USHORT qClass;
};

class fixedDNSheader
{
public:
	USHORT ID;
	USHORT flags;
	USHORT questions;
	USHORT answers;
	USHORT authority;
	USHORT addition;
};

class fixedRR
{
public:
	USHORT name;
	USHORT type;
	USHORT class_content;
	int TTL;
	USHORT length_data;
};
#pragma pack(pop)

void generate_DNS_question(char *request, char *host)
{
	int num_copyed = 0;
	while (true)
	{
		char *dot_position = strchr(host, '.');
		if (dot_position == NULL)	break;
		int length = dot_position - host;
		request[num_copyed] = length;
		memcpy(request + num_copyed + 1, host, length);
		num_copyed += length+1;
		host = dot_position + 1;
	}
	request[num_copyed] = strlen(host);
	memcpy(request + num_copyed + 1, host, strlen(host));
}

void generate_DNS_request(char *host, char *DNS_request, int pkt_size)
{
	memset(DNS_request, 0, pkt_size*sizeof(char));
	fixedDNSheader *dns_header = (fixedDNSheader *)DNS_request;
	queryHeader *query_header = (queryHeader*)(DNS_request + pkt_size - sizeof(queryHeader));
	dns_header->ID = rand() % 65535;
	dns_header->flags = DNS_QUERY || DNS_RD || DNS_STDQUERY;
	dns_header->questions = htons(1);
	dns_header->answers = 0;
	dns_header->authority = 0;
	dns_header->addition = 0;
	query_header->qType = htons(DNS_A);
	query_header->qClass = htons(NDS_INET);
	generate_DNS_question(DNS_request + sizeof(fixedDNSheader), host);
}

class UDP_connection
{
public:
	bool UDP_init();
	bool UDP_send(char *IP_send_to, char *message_send, int message_length);
	int	 UDP_recv(char **buff);

private:
	SOCKET socket_UDP;
	struct sockaddr_in local, remote;
	bool   init_winsock();
};

bool UDP_connection::init_winsock()
{
	WSADATA wsaData;
	WORD wVersionRequested = MAKEWORD(2, 2);
	if (WSAStartup(wVersionRequested, &wsaData) != 0) {
		printf("faile with %d\n", WSAGetLastError());
		WSACleanup();
		return false;
	}
	else return true;
}

bool UDP_connection::UDP_send(char *IP_send_to, char *message_send, int message_length)
{
	memset(&remote, 0, sizeof(remote));
	remote.sin_family = AF_INET;
	remote.sin_addr.s_addr = inet_addr(IP_send_to);
	remote.sin_port = htons(53);
	if (sendto(socket_UDP, message_send, message_length, 0, (struct sockaddr*)&remote, sizeof(remote)) == SOCKET_ERROR)
		return false;
	return true;
}

bool UDP_connection::UDP_init()
{
	if (!init_winsock())
	{
		cout << "WinSock init faild";
		return false;
	}
	socket_UDP = socket(AF_INET, SOCK_DGRAM, 0);
	memset(&local, 0, sizeof(local));
	local.sin_family = AF_INET;
	local.sin_addr.s_addr = INADDR_ANY;
	local.sin_port = htons(0);
	if (bind(socket_UDP, (struct sockaddr*)&local, sizeof(local)) == SOCKET_ERROR)
	{
		cout << "Blind Socket Faild";
		return false;
	}
	else return true;
}

int UDP_connection::UDP_recv(char **buff)
{
	*buff = new char[MAX_DNS_LEN];
	struct sockaddr addr;
	int fromlen = sizeof(addr);
	int byte_count = recvfrom(socket_UDP, *buff, 512, 0, &addr, &fromlen);
	return byte_count;
}

void test()
{
	//char *DNS_address = "128.194.135.94";
	char *DNS_address = "8.8.8.8";
	string URL = "www.cnn.com";
	char query1[4] = { '0', '1', '0', '1' };
	unsigned char size = 3;
	char content1[3] = { 'w', 'w', 'w' };
	char content2[3] = { 'c', 'n', 'n' };
	char content3[3] = { 'c', 'o', 'm' };
	char header[12] = { 12, 12, '\1', '\0', '\0', '\1', '\0', '\0', '\0', '\0', '\0', '\0' };
	char query[] = { '\3', 'w', 'w', 'w', '\3', 'c', 'n', 'n', '\3', 'c', 'o', 'm', '\0', '\0', '\1', '\0', '\1' };
	char message[] = { 12, 12, '\1', '\0', '\0', '\1', '\0', '\0', '\0', '\0', '\0', '\0', '\3', 'w', 'w', 'w', '\3', 'c', 'n', 'n', '\3', 'c', 'o', 'm', '\0', '\0', '\1', '\0', '\1' };
}

void display_address(char *target, int index_init)
{
	int index = index_init;
	while (true)
	{
		int number = (unsigned char)target[index++];
		if (number == 0)	break;
		if (number != 192)//C0
		{
			for (int i = 0; i < number; i++)	printf("%c", target[index++]);
			printf(".");
		}
		else  index = (unsigned char)target[index];
	}
}

void display_IP(char *target, int index)
{
	printf("%d.%d.%d.%d\n", (unsigned char)target[index], (unsigned char)target[index+1], (unsigned char)target[index+2], (unsigned char)target[index+3]);
}

void display_data(char *data, int length, int init_index)
{
	int index = init_index;
	while (index < length)
	{
		fixedRR *first = (fixedRR *)(data + index);
		//printf("%d", (unsigned char)(*((char*)(first)+1)));
		printf("%d\n", (USHORT)htons(first->name));
		display_address(data, data[index + 1]);
		printf("%d\n", (USHORT)htons(first->type));
		if ((USHORT)htons(first->type) == 5)
			display_address(data, index + 12);
		else display_IP(data, index + 12);
		printf("%d\n", (USHORT)htons(first->class_content));
		printf("%d\n", htonl(first->TTL));
		printf("%d\n", (USHORT)htons(first->length_data));
		printf("%d\n", (unsigned)data[index + 1]);
		cout << endl;
		index += (unsigned)htons(first->length_data) + 12;
		cout << "index:";
		cout << index << endl;
		cout << endl;
		cout << endl;
	}

}

int main()
{
	char *buff;
	char *DNS_address = "8.8.8.8";
	UDP_connection UDP_connect;
	UDP_connect.UDP_init();
	char *host = "www.randomA.irl";
	int pktsize = strlen(host) + 2 + sizeof(fixedDNSheader) + sizeof(queryHeader);
	char *request = new char[pktsize];
	generate_DNS_request(host, request, pktsize);
	UDP_connect.UDP_send(DNS_address, request, pktsize);
	int byte_count = UDP_connect.UDP_recv(&buff);

	display_data(buff, byte_count, pktsize);
	for (int i = pktsize; i < byte_count; i++)
	{
		printf("%d   %d\n", i, (unsigned	char) buff[i]);
	}

	//display_address(buff, 77);
	getchar();
}


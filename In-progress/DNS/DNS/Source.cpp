#include <iostream>
#include <winsock.h>

using namespace std;

class DNS_header
{
public:
	char* generate_header(char type, unsigned int num_question)
	{
		generate_TXID();
	}

private:
	char TXID[2];
	char flag[2];
	char nQuestion[2];
	char nAnswers[2];
	char nAuthority[2];
	char nAdditional[2];
	void generate_TXID()
	{
		TXID[0] = rand() % 256;
		TXID[1] = rand() % 256;
	}
};

bool init_winsock()
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

bool open_UDP_socket(SOCKET *socket_UDP)
{
	*socket_UDP = socket(AF_INET, SOCK_DGRAM, 0);
	struct sockaddr_in local;
	memset(&local, 0, sizeof(local));
	local.sin_family = AF_INET;
	local.sin_addr.s_addr = INADDR_ANY;
	local.sin_port = htons(0);
	if (bind(*socket_UDP, (struct sockaddr*)&local, sizeof(local)) == SOCKET_ERROR)
		return false;
	else return true;
}

bool send_UDP_package(SOCKET *socket_UDP, char *IP_send_to, char *message_send, int message_length)
{
	struct sockaddr_in remote;
	memset(&remote, 0, sizeof(remote));
	remote.sin_family = AF_INET;
	remote.sin_addr.s_addr = inet_addr(IP_send_to);
	remote.sin_port = htons(53);
	if (sendto(*socket_UDP, message_send, message_length, 0, (struct sockaddr*)&remote, sizeof(remote)) == SOCKET_ERROR)
		return false;
	return true;
}

int main()
{
	/*
	char buff[512];
	//char *DNS_address = "128.194.135.94";
	char *DNS_address = "8.8.8.8";
	string URL = "www.cnn.com";
	unsigned short a = 1;
	char query1[4] = { '0', '1', '0', '1' };
	unsigned char size = 3;
	char content1[3] = { 'w', 'w', 'w' };
	char content2[3] = { 'c', 'n', 'n' };
	char content3[3] = { 'c', 'o', 'm' };
	char header[12] = { 12, 12, '\1', '\0', '\0', '\1', '\0', '\0', '\0', '\0', '\0', '\0'};
	char query[] = { '\3', 'w', 'w', 'w', '\3', 'c', 'n', 'n', '\3', 'c', 'o', 'm', '\0', '\0', '\1', '\0', '\1' };
	char message[] = { 12, 12, '\1', '\0', '\0', '\1', '\0', '\0', '\0', '\0', '\0', '\0', '\3', 'w', 'w', 'w', '\3', 'c', 'n', 'n', '\3', 'c', 'o', 'm', '\0', '\0', '\1', '\0', '\1' };
	cout << sizeof(message);

	init_winsock();
	SOCKET socket_UDP;
	struct sockaddr addr;
	int fromlen = sizeof(addr);
	bool status1 = open_UDP_socket(&socket_UDP);
	bool status2 = send_UDP_package(&socket_UDP, DNS_address, message, sizeof(message));
	int byte_count = recvfrom(socket_UDP, buff, sizeof(buff), 0, &addr, &fromlen);*/

	unsigned int a = 5;
	char b[2];
	b[0] = (char)(a & 0x0001);
	b[1] = (char)(a & 0x0010) >> 4;

}


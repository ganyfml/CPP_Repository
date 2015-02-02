#include <stdio.h>
#include <string.h>
#include <climits>
#include <winsock2.h>
#include <iostream>
#include <fstream>
#include <unordered_set>
#include "HTMLParserBase.h"

using namespace std;

bool parser_URL(char *address, char *host, char *request, int *port)
{
	char scheme_part_check[8] = "";
	strncpy(scheme_part_check, address, 7);
	if (strcmp(scheme_part_check, "http://") != 0)
	{
		printf("failed, invaild scheme\n");
		return false;
	}

	int query_position = (strchr(address + 7, '?') != NULL) ? strchr(address + 7, '?') - address : INT_MAX;
	int path_position = (strchr(address + 7, '/') != NULL) ? strchr(address + 7, '/') - address : INT_MAX;
	int fragment_position = (strchr(address + 7, '#') != NULL) ? strchr(address + 7, '#') - address : INT_MAX;
	int request_position = path_position > query_position ? query_position : path_position;

	if (request_position > fragment_position)
	{
		strncpy(host, address + 7, fragment_position - 7);
		strcpy(request, "/");
	}
	else if (request_position == fragment_position)
	{
		strcpy(host, address + 7);
		strcpy(request, "/");
	}
	else
	{
		if (fragment_position == INT_MAX)	strncpy(request, address + request_position, (strlen(address) - request_position));
		else strncpy(request, address + request_position, fragment_position - request_position);
		strncpy(host, address + 7, request_position - 7);
	}

	if (strchr(host, ':') != NULL)
	{
		int port_position = strchr(host, ':') - host;
		if (port_position == (strlen(host) - 1))	return -2;
		host = strtok(host, ":");
		*port = atoi(strtok(NULL, ":"));
		if (*port == 0)
		{
			printf("failed, invaild port\n");
			return false;
		}
	}
	else *port = 80;

	if (request[0] != '/')
	{
		char new_request[50] = "/";
		strcpy(request, strcat(new_request, request));
		delete(new_request);
	}
	if (request[strlen(request) - 1] == '\r') request[strlen(request) - 1] = NULL;
	return true;
}

void generate_HTTP_request(char *HTTP_request, char *host, char *request)
{
	strcpy(HTTP_request, "GET ");
	strcat(HTTP_request, request);
	strcat(HTTP_request, " HTTP/1.0\r\nUser-agent: crawler_CSCE612/1.1\r\nHost: ");
	strcat(HTTP_request, host);
	strcat(HTTP_request, "\r\nConnection: close\r\n\r\n");
}

void generate_robot_request(char *HTTP_request, char *host)
{
	strcpy(HTTP_request, "HEAD /robots.txt");
	strcat(HTTP_request, " HTTP/1.0\r\nUser-agent: crawler_CSCE612/1.1\r\nHost: ");
	strcat(HTTP_request, host);
	strcat(HTTP_request, "\r\nConnection: close\r\n\r\n");
}

void display_HTTP_header(char *HTTP_request)
{
	int index = 0;
	while (1)
	{
		char char_to_display = HTTP_request[index];
		if (char_to_display == '\r' && HTTP_request[index + 2] == '\r')	break;
		else
		{
			printf("%c", char_to_display);
			index++;
		}
	}
}

////////////////////////////////////////////////////////////////////////////////////////
//Some code from Preliminaries II (http://irl.cse.tamu.edu/courses/463/1-22-15.pdf)
//By Dr. Dmitri Loguinov
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

bool open_socket(SOCKET* sock)
{
	*sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (*sock == INVALID_SOCKET)
	{
		printf("faile with %d\n", WSAGetLastError());
		WSACleanup();
		return false;
	}
	else return true;
}

bool DNS_lookup(char* host, int port, hostent *remote, sockaddr_in* server, char *IP_address)
{
	printf("	Doing DNS... ");
	DWORD time1 = timeGetTime();
	DWORD time2;
	DWORD IP = inet_addr(host);
	if (IP == INADDR_NONE)
	{
		// if not a valid IP, then do a DNS lookup
		if ((remote = gethostbyname(host)) == NULL)
		{
			printf("faile with %d\n", WSAGetLastError());
			return false;
		}
		else // take the first IP address and copy into sin_addr
			memcpy((char *)&(server->sin_addr), remote->h_addr, remote->h_length);
		struct in_addr* IP_address_inaddr = (struct in_addr *) remote->h_addr;
		strcpy(IP_address, inet_ntoa(*IP_address_inaddr));
	}
	else
	{
		// if a valid IP, directly drop its binary version into sin_addr
		server->sin_addr.S_un.S_addr = IP;
		strcpy(IP_address, host);
	}
	server->sin_family = AF_INET;
	server->sin_port = htons(port);
	time2 = timeGetTime();
	printf("done in %d ms, found %s\n", time2 - time1, IP_address);
	return true;
}

bool connect_page(SOCKET *sock, sockaddr_in *server, char *HTTP_request, bool robot)
{
	DWORD time1 = timeGetTime();
	DWORD time2;
	if(!robot)	printf("	Connecting on page...");
	else printf("	Connectiong to robot...");
	if (connect(*sock, (struct sockaddr*) server, sizeof(struct sockaddr_in)) == SOCKET_ERROR)
	{
		printf("failed with %d\n", WSAGetLastError());
		return false;
	}

	if (send(*sock, HTTP_request, strlen(HTTP_request), 0) == SOCKET_ERROR)
	{
		printf("failed with %d\n", WSAGetLastError());
		return false;
	}
	time2 = timeGetTime();
	printf("done in %d ms\n", time2 - time1);
	return true;
}
//Some code from Preliminaries II (http://irl.cse.tamu.edu/courses/463/1-22-15.pdf)
//By Dr. Dmitri Loguinov
////////////////////////////////////////////////////////////////////////////////////////

char* load_page(SOCKET sock, bool robot_page)
{
	printf("	Loading...");
	DWORD time1 = timeGetTime();
	DWORD time2;
	int buffer_size = 1024;
	char recvbuf[512];
	memset(recvbuf, '\0', 512);
	char *recvHTML = (char*)malloc(buffer_size * sizeof(char));

	recv(sock, recvbuf, 512, 0);
	strcpy(recvHTML, recvbuf);
	if (!(recvHTML[0] == 'H' && recvHTML[1] == 'T' && recvHTML[2] == 'T' && recvHTML[3] == 'P' && recvHTML[4] == '/'))
	{
		printf("failed with non-HTTP header\n");
		free(recvHTML);
		return NULL;
	}

	fd_set timeout;
	struct timeval time_threshold;
	time_threshold.tv_sec = 10;
	time_threshold.tv_usec = 0;
	FD_ZERO(&timeout);

	while (1)
	{
		time2 = timeGetTime();
		if (time2 - time1 > 10000)
		{
			printf("failed with slow download -- loading page time larger then 10 seconds\n");
			free(recvHTML);
			return NULL;
		}
		FD_SET(sock, &timeout);
		memset(recvbuf, '\0', 512);
		int select_status = select(sock + 1, &timeout, NULL, NULL, &time_threshold);
		if (select_status > 0)
		{
			int length = recv(sock, recvbuf, 500, 0);
			if (length == -1)
			{
				printf("failed with %d\n", WSAGetLastError());
				free(recvHTML);
				return NULL;
			}
			else if (length == 0)	break;
			else
			{
				if (buffer_size - strlen(recvHTML) <= 512)
				{
					buffer_size *= 2;
					recvHTML = (char*)realloc(recvHTML, buffer_size * sizeof(char));
				}
				strcat(recvHTML, recvbuf);
				int max = (robot_page) ? 16385 : 2097152;
				if (strlen(recvHTML) > max)
				{
					printf("Max reached\n");
					free(recvHTML);
					return NULL;
				}
			}
		}
		else if (select_status == 0)
		{
			printf("Failed with slow download -- recv interval larger then 10 second\n");
			free(recvHTML);
			return NULL;
		}
		else
		{
			printf("failed with %d\n", WSAGetLastError());
			free(recvHTML);
			return NULL;
		}
		FD_ZERO(&timeout);
	}
	time2 = timeGetTime();
	if (strlen(recvHTML) == 0)
	{
		printf("failed with %d on recv\n", WSAGetLastError());
		free(recvHTML);
		return NULL;
	}
	printf("done in %d ms with %d bytes\n", time2 - time1, strlen(recvHTML));
	return recvHTML;
}

int parse_page(const char *host, char *recvHTML)
{
	printf("	Parsing page... ");
	int nLinks;
	DWORD time1 = timeGetTime();
	HTMLParserBase *parser = new HTMLParserBase;
	char base_URL[128] = "http://";
	strcat(base_URL, host);
	char *linkBuffer = parser->Parse(recvHTML, strlen(recvHTML), base_URL, (int)strlen(base_URL), &nLinks);
	if (nLinks < 0)
		nLinks = 0;
	DWORD time2 = timeGetTime();
	printf("done in %d ms with %d links\n", time2 - time1, nLinks);
	delete(parser);
	return nLinks;
}

char *wirte_file_to_memory(char *path)
{
	FILE *file = fopen(path, "rb");
	char * URL_file_content;
	if (file != NULL)
	{
		if (fseek(file, 0L, SEEK_END) == 0)
		{
			long file_size = ftell(file);
			if (file_size > 0)
			{
				URL_file_content = (char *)malloc(sizeof(char) * (file_size + 1));
				memset(URL_file_content, '\0', file_size + 1);
				if (fseek(file, 0L, SEEK_SET) == 0)
				{
					if (fread(URL_file_content, sizeof(char), file_size, file) > 0)
					{
						fclose(file);
						return URL_file_content;
					}
				}
				free(URL_file_content);
			}
		}
		fclose(file);
	}
	cout << "Unable to open file\n";
	return NULL;
}

bool check_uniqueness(unordered_set <string> *set, char *target)
{
	unsigned int host_set_size = set->size();
	set->insert(target);
	if (set->size() == host_set_size)
	{
		cout << "failed" << endl;
		return false;
	}
	else  cout << "passed" << endl;
	return true;
}

bool deal_with_robot(SOCKET connection_socket, char *host, sockaddr_in *server)
{
	open_socket(&connection_socket);
	char HTTP_request[512];
	generate_robot_request(HTTP_request, host);
	if(!connect_page(&connection_socket, server, HTTP_request, true))	return false;
	char *recvHTML = load_page(connection_socket, true);
	if (recvHTML == NULL)	return false;
	printf("	Verifying header... ");
	char robot_status[4] = { recvHTML[9], recvHTML[10], recvHTML[11], '\0' };
	printf("Status code %s\n", robot_status);
	if (robot_status[0] != '4')	return false;
	free(recvHTML);
	closesocket(connection_socket);
	return true;
}

bool deal_with_page(SOCKET connection_socket, char *host, char *request, sockaddr_in *server)
{
	open_socket(&connection_socket);
	char HTTP_request[2048];
	generate_HTTP_request(HTTP_request, host, request);
	if(!connect_page(&connection_socket, server, HTTP_request, false))	return false;
	char *recvHTML = load_page(connection_socket, false);
	if (recvHTML == NULL)	return false;
	printf("	Verifying header... ");
	char page_status[4] = { recvHTML[9], recvHTML[10], recvHTML[11], '\0' };
	printf("Status code %s\n", page_status);
	if (page_status[0] != '2')	return false;
	parse_page(host, recvHTML);
	free(recvHTML);
	closesocket(connection_socket);
}

void main()
{
	unordered_set <string> host_set;
	char *path = "URL.txt";
	char *URL_file_content = wirte_file_to_memory(path);
	if (URL_file_content == NULL)	return;
	char *address = strtok(URL_file_content, "\n");

	SOCKET connection_socket;
	init_winsock();
	open_socket(&connection_socket);
	bool first_come_to_loop = true;

	while (1)
	{
		if (!first_come_to_loop)
			address = strtok(NULL, "\n");
		else first_come_to_loop = false;

		char host[128] = "";
		char request[2048] = "";
		char IP_address[32];
		int port;

		if (address == NULL)	break;
		cout << "URL: " << address << endl;
		if (!parser_URL(address, host, request, &port)) continue;//invaild format
		cout << "	Checking host uniqueness...";
		if (!check_uniqueness(&host_set, host))	continue;
			
		hostent remote;
		sockaddr_in server;
		open_socket(&connection_socket);
		if(!DNS_lookup(host, port, &remote, &server, IP_address))	continue;
		cout << "	Checking IP uniqueness...";
		if (!check_uniqueness(&host_set, IP_address))	continue;
		closesocket(connection_socket);

		if (!deal_with_robot(connection_socket, host, &server))	continue;
		if (!deal_with_page(connection_socket, host, request, &server))	continue;
	}
	return;
}
#include "SenderSocket.h"

/*
#define STATUS_OK 0 // no error
#define ALREADY_CONNECTED 1 // second call to ss.Open() without closing connection
#define NOT_CONNECTED 2 // call to ss.Send()/Close() without ss.Open()
#define INVALID_NAME 3 // ss.Open() with targetHost that has no DNS entry
#define FAILED_SEND 4 // sendto() failed in kernel
#define TIMEOUT 5 // timeout
#define FAILED_RECV 6 // recvfrom() failed in kernel
#define MAGIC_PORT		22345		//The port number that receiver listened
#define MAX_PKT_SIZE	(1500-28)	//Maximum UDP packet size accepted by receiver
*/

int SenderSocket::Open(char *target_host, int magic_port, int sender_window, LinkProperties *p, DWORD time)
{
	port = magic_port;
	host = target_host;
	if (!UDP_init())	return FAILED_SEND;
	int try_count = 0;
	DWORD time_init = timeGetTime();
	while (try_count++ < 3)
	{
		long RTO_this_time = pow(2, try_count - 1);
		int status;
		std::string host_IP;
		DWORD send_time = timeGetTime();
		if ((status = UDP_send(host, port, generate_SYN(p), sizeof(SenderSynHeader), &host_IP)) != STATUS_OK)
		{
			if (status == INVALID_NAME)
			{
				std::cout << "[" << (float)(send_time - time_init + time) / (1000) << "] target " << host << " is invaild" << std::endl;
			}
			else std::cout << "[" << (float)(send_time - time_init + time) / (1000) << "] --> " << "failed sendto with " << WSAGetLastError() << std::endl;
			return status;
		}
		std::cout << "[" << (float)(send_time - time_init + time) / (1000) << "]" << " --> SYN 0 (attemp " << try_count << " of 3, RTO " << RTO_this_time << ") to " << host_IP << std::endl;
		fd_set fd;
		struct timeval time_threshold;
		time_threshold.tv_sec = RTO_this_time;
		time_threshold.tv_usec = 0;
		FD_ZERO(&fd);
		FD_SET(socket_UDP, &fd);
		int avaiable = select(0, &fd, NULL, NULL, &time_threshold);

		if (avaiable > 0)
		{
			char *buff;
			int byte_count = UDP_recv(&buff);
			DWORD recv_time = timeGetTime();
			if (byte_count == SOCKET_ERROR)
			{
				std::cout << "[" << (float)(recv_time - time_init + time) / (1000) << "] <-- " << "failed recvfrom with " << WSAGetLastError() << std::endl;
				return FAILED_RECV;
			}
			if (byte_count != sizeof(ReceiverHeader))
				continue;
			else
			{
				ReceiverHeader *SYN_ACK = (ReceiverHeader*)buff;
				if (SYN_ACK->ackSeq == 0 && SYN_ACK->flags.ACK == 1 && SYN_ACK->flags.FIN == 0
					&& SYN_ACK->flags.SYN == 1)
				{
					RTO = (recv_time - send_time) * 3 * 1000;
					std::cout << "[" << (float)(recv_time - time_init + time) / (1000) << "]" << " <-- SYN-ACK 0 window " << SYN_ACK->recvWnd << "; setting RTO to " << (float)RTO / (1e6) << std::endl;
					return STATUS_OK;
				}
				else continue;
			}
		}
		else  continue;
	}
	return TIMEOUT;
}

char *SenderSocket::generate_FIN()
{
	char* FIN_message = new char[sizeof(SenderDataHeader)];
	SenderDataHeader *FIN = (SenderDataHeader*)FIN_message;
	FIN->seq = 0;
	FIN->flags.ACK = 0;
	FIN->flags.SYN = 0;
	FIN->flags.FIN = 1;
	FIN->flags.reserved = 0;
	FIN->flags.magic = MAGIC_PROTOCOL;
	return FIN_message;
}


char *SenderSocket::generate_SYN(LinkProperties *p)
{
	char* SYN_message = new char[sizeof(SenderSynHeader)];
	SenderSynHeader *SYN = (SenderSynHeader*)SYN_message;
	SYN->lp = *p;
	SYN->sdh.seq = 0;
	SYN->sdh.flags.ACK = 0;
	SYN->sdh.flags.SYN = 1;
	SYN->sdh.flags.FIN = 0;
	SYN->sdh.flags.reserved = 0;
	SYN->sdh.flags.magic = MAGIC_PROTOCOL;
	return SYN_message;
}

int SenderSocket::Send(char *data, int data_length)
{
	return STATUS_OK;
}

int SenderSocket::Close(DWORD time)
{
	int try_count = 0;
	DWORD time_init = timeGetTime();
	while (try_count++ < 5)
	{
		long RTO_this_time = pow(2, try_count-1) * RTO;
		int status;
		std::string host_IP;
		DWORD send_time = timeGetTime();
		if ((status = UDP_send(host, port, generate_FIN(), sizeof(SenderDataHeader), &host_IP)) != STATUS_OK)
		{
			std::cout << "[" << (float)(send_time - time_init + time) / (1000) << "] --> " << "failed sendto with " << WSAGetLastError() << std::endl;
			return status;
		}
		std::cout << "[" << (float)(timeGetTime() - time_init + time) / (1000) << "]" << " --> FIN 0 (attemp " << try_count << " of 5, RTO " << (float)(RTO_this_time / (1e6)) << ") to " << host_IP << std::endl;
		fd_set fd;
		struct timeval time_threshold;
		time_threshold.tv_sec = 0;
		time_threshold.tv_usec = RTO_this_time;
		FD_ZERO(&fd);
		FD_SET(socket_UDP, &fd);
		int avaiable = select(0, &fd, NULL, NULL, &time_threshold);

		if (avaiable > 0)
		{
			char *buff;
			int byte_count = UDP_recv(&buff);
			if (byte_count == SOCKET_ERROR)
			{
				std::cout << "[" << (float)(timeGetTime() - time_init + time) / (1000) << "] <-- " << "failed recvfrom with " << WSAGetLastError() << std::endl;
				return FAILED_RECV;
			}
			if (byte_count != sizeof(ReceiverHeader))
				continue;
			else
			{
				ReceiverHeader *FIN_ACK = (ReceiverHeader*)buff;
				if (FIN_ACK->ackSeq == 0 && FIN_ACK->flags.ACK == 1 && FIN_ACK->flags.FIN == 1
					&& FIN_ACK->flags.SYN == 0)
				{
					std::cout << "[" << (float)(timeGetTime() - time_init + time) / (1000) << "]" << " <-- FIN-ACK 0 window " << FIN_ACK->recvWnd << std::endl;
					return STATUS_OK;
				}
				else continue;
			}
		}
		else  continue;
	}
	return TIMEOUT;
}

int SenderSocket::init_winsock()
{
	WSADATA wsaData;
	WORD wVersionRequested = MAKEWORD(2, 2);
	if (WSAStartup(wVersionRequested, &wsaData) != 0) {
		WSACleanup();
		return false;
	}
	else return true;
}

int SenderSocket::UDP_send(char *IP_send_to, int port, char *message_send, int message_length, std::string *Host_IP)
{
	memset(&remote, 0, sizeof(remote));
	remote.sin_family = AF_INET;
	remote.sin_port = htons(port);
	DWORD IP = inet_addr(IP_send_to);
	if (IP == INADDR_NONE)
	{
		hostent *remote_1;
		if ((remote_1 = gethostbyname(IP_send_to)) == NULL)
		{
			return INVALID_NAME;
		}
		else
		{
			struct in_addr* IP_address_inaddr = (struct in_addr *) remote_1->h_addr;
			*Host_IP = inet_ntoa(*IP_address_inaddr);
			remote.sin_addr.s_addr = inet_addr(Host_IP->c_str());
		}
	}
	else
	{
		remote.sin_addr.s_addr = inet_addr(IP_send_to);
		*Host_IP = IP_send_to;
	}

	if (sendto(socket_UDP, message_send, message_length, 0, (struct sockaddr*)&remote, sizeof(remote)) == SOCKET_ERROR)
	{
		std::cout << "UDP_send_ERROR: " << WSAGetLastError() << std::endl;
		return FAILED_SEND;
	}
	else return STATUS_OK;
}

bool SenderSocket::UDP_init()
{
	if (!init_winsock())
	{
		std::cout << "WinSock init faild";
		return false;
	}
	socket_UDP = socket(AF_INET, SOCK_DGRAM, 0);
	memset(&local, 0, sizeof(local));
	local.sin_family = AF_INET;
	local.sin_addr.s_addr = INADDR_ANY;
	local.sin_port = htons(0);
	if (bind(socket_UDP, (struct sockaddr*)&local, sizeof(local)) == SOCKET_ERROR)
	{
		std::cout << "Blind Socket Faild";
		return false;
	}
	else return true;
}

int SenderSocket::UDP_recv(char **buff)
{
	*buff = new char[sizeof(ReceiverHeader)];
	struct sockaddr addr;
	int fromlen = sizeof(addr);
	int byte_count = recvfrom(socket_UDP, *buff, sizeof(ReceiverHeader), 0, &addr, &fromlen);
	return byte_count;
}
#include "SenderSocket.h"

int SenderSocket::Open(char *target_host, int magic_port, int sender_window1, LinkProperties *p, DWORD time)
{
	sender_window = sender_window1;
	port = magic_port;
	host = target_host;
	link_property = *p;
	if (!UDP_init())	return FAILED_SEND;
	int try_count = 0;
	DWORD time_init = timeGetTime();
	while (try_count++ < 3)
	{
		long RTO_this_time = max(2*p->RTT*1e6,1e6);
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
		//std::cout << "[" << (float)(send_time - time_init + time) / (1000) << "]" << " --> SYN 0 (attemp " << try_count << " of 3, RTO " << (double)RTO_this_time*1e-6 << ") to " << host_IP << std::endl;
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
			DWORD recv_time = timeGetTime();
			if (byte_count == SOCKET_ERROR)
			{
				std::cout << "[" << (float)(recv_time - time_init + time) / (1000) << "] <-- " << "failed recvfrom with " << WSAGetLastError() << std::endl;
				return FAILED_RECV;
			}
			//TODO false packet received
			if (byte_count != sizeof(ReceiverHeader))
				continue;
			else
			{
				ReceiverHeader *SYN_ACK = (ReceiverHeader*)buff;
				if (SYN_ACK->ackSeq == 0 && SYN_ACK->flags.ACK == 1 && SYN_ACK->flags.FIN == 0
					&& SYN_ACK->flags.SYN == 1)
				{
					RTO = (recv_time - send_time) * 3 * 1000;
					//std::cout << "[" << (float)(recv_time - time_init + time) / (1000) << "]" << " <-- SYN-ACK 0 window " << SYN_ACK->recvWnd << "; setting RTO to " << (float)RTO / (1e6) << std::endl;
					return STATUS_OK;
				}
				//TODO false packet received
				else continue;
			}
		}
		//Timeout for this try
		else  continue;
	}
	return TIMEOUT;
}

char *SenderSocket::generate_FIN()
{
	char* FIN_message = new char[sizeof(SenderDataHeader)];
	SenderDataHeader *FIN = (SenderDataHeader*)FIN_message;
	FIN->seq = sequence_number;
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

char *SenderSocket::generate_Data_message(char *data, int data_length)
{
	if (data_length + sizeof(SenderDataHeader) > MAX_PKT_SIZE)	return NULL;
	char* data_message = new char[sizeof(SenderDataHeader)+data_length];
	SenderDataHeader *data_header = (SenderDataHeader*)data_message;
	data_header->seq = sequence_number++;
	data_header->flags.ACK = 0;
	data_header->flags.SYN = 0;
	data_header->flags.FIN = 0;
	data_header->flags.reserved = 0;
	data_header->flags.magic = MAGIC_PROTOCOL;
	memcpy(data_message+sizeof(SenderDataHeader),data,data_length);
	return data_message;
}

void SenderSocket::update_parameter_for_RTO_calculation(DWORD sample_RTT1)
{
	sample_RTT = (double)sample_RTT1/1000;
	if (prev_estRTT == 0)
		prev_estRTT = sample_RTT;
	else prev_estRTT = 0.875 * prev_estRTT + 0.125 * sample_RTT;
	if (prev_devRTT == -1)
		prev_devRTT = 0;
	else  prev_devRTT = 0.75 * prev_devRTT + 0.25 * abs(sample_RTT - prev_estRTT);
	//std::cout << sample_RTT << " " << prev_estRTT << " " << prev_devRTT << std::endl;
}

DWORD SenderSocket::calculate_RTO()
{
	if (sample_RTT == 0 && prev_estRTT == 0 && prev_devRTT == -1)
		return RTO;//first time
	double estRTT = 0.875 * prev_estRTT + 0.125 * sample_RTT;
	double devRTT = 0.75 * prev_devRTT + 0.25 * abs(sample_RTT - estRTT);
	double RTO_ret = estRTT + 4 * max(devRTT, 0.010);
	current_status.estRTT = estRTT;
	return RTO_ret*1e6;
}

int SenderSocket::Send(char *data, int data_length)
{
	char *data_message = generate_Data_message(data, data_length);
	if (data_message == NULL)	return FAILED_SEND;

	int try_count = 0;
	int dupACK = 0;
	bool retransmission = false;
	bool refresh_timer = true;
	DWORD time_init = timeGetTime();
	DWORD timer_left = 0;
	DWORD RTO_temp;
	DWORD send_time = timeGetTime();
	while (try_count < 50)
	{
		if (refresh_timer)
		{
			RTO = calculate_RTO();
			RTO_temp = retransmission ? RTO_temp * 2 : RTO;
			int status;
			std::string host_IP;
			//std::cout << "-->" << ((SenderDataHeader *)data_message)->seq << " With RTO: " << (double)RTO_temp * (1.0e-6) << std::endl;
			if ((status = UDP_send(host, port, data_message, (sizeof(SenderDataHeader) + data_length), &host_IP)) != STATUS_OK)
			{
				//std::cout << "[" << (float)(send_time - time_init + time) / (1000) << "] --> " << "failed sendto with " << WSAGetLastError() << std::endl;
				return status;
			}
		}		//std::cout << "[" << (float)(timeGetTime() - time_init + time) / (1000) << "]" << " --> FIN 0 (attemp " << try_count << " of 5, RTO " << (float)(RTO_this_time / (1e6)) << ") to " << host_IP << std::endl;

		fd_set fd;
		struct timeval time_threshold;
		time_threshold.tv_sec = 0;
		time_threshold.tv_usec = refresh_timer ? RTO_temp : timer_left;
		FD_ZERO(&fd);
		FD_SET(socket_UDP, &fd);
		int avaiable = select(0, &fd, NULL, NULL, &time_threshold);

		if (avaiable > 0)
		{
			char *buff;
			int byte_count = UDP_recv(&buff);
			DWORD time_recv_something = timeGetTime();
			timer_left = time_threshold.tv_usec - time_recv_something;
			if (byte_count == SOCKET_ERROR)
			{
				//std::cout << "[" << (float)(timeGetTime() - time_init + time) / (1000) << "] <-- " << "failed recvfrom with " << WSAGetLastError() << std::endl;
				return FAILED_RECV;
			}
			if (byte_count != sizeof(ReceiverHeader))
			{
				//corrupt response
				refresh_timer = false;
				continue;
			}
			else
			{
				ReceiverHeader *Data_ACK = (ReceiverHeader*)buff;
				//std::cout << "<--" << ((ReceiverHeader *)Data_ACK)->ackSeq << " with recvWnd: " << ((ReceiverHeader *)Data_ACK)->recvWnd << " with time passed: " << (time_recv_something - send_time)*1e-3 << std::endl;
				if (Data_ACK->flags.ACK == 1 && Data_ACK->flags.FIN == 0
					&& Data_ACK->flags.SYN == 0)
				{
					if (Data_ACK->ackSeq == sequence_number)
					{
						//Vaild ACK, expect next data
						if(!retransmission)	update_parameter_for_RTO_calculation(time_recv_something - send_time);
						current_status.effective_window = min(sender_window, Data_ACK->recvWnd);
						return STATUS_OK;
					}
					else if (Data_ACK->ackSeq == sequence_number-1)
					{
						//Vaild ACK, but expect this data again
						dupACK++;
						if (dupACK == 3)
						{
							//Do fast retransmission
							current_status.num_packet_fast_retrans++;
							refresh_timer = true;
							retransmission = true;
							try_count++;
						}
						else
						{
							refresh_timer = false;
						}
						continue;
					}
				}
				else
				{
					//still corrupt response
					refresh_timer = false;
					continue;
				}
			}
		}
		else
		{
			//Timeout for this time, do the retransmission
			current_status.num_packet_timeout++;
			retransmission = true;
			refresh_timer = true;
			try_count++;
		}
	}
	return TIMEOUT;
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
		//std::cout << "[" << (float)(timeGetTime() - time_init + time) / (1000) << "]" << " --> FIN 0 (attemp " << try_count << " of 5, RTO " << (float)(RTO_this_time / (1e6)) << ") to " << host_IP << std::endl;
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
				if (FIN_ACK->flags.ACK == 1 && FIN_ACK->flags.FIN == 1
					&& FIN_ACK->flags.SYN == 0)
				{
					std::cout << "[" << (float)(timeGetTime() - time_init + time) / (1000) << "]" << " <-- FIN-ACK " << FIN_ACK->ackSeq<< " window " << std::hex << FIN_ACK->recvWnd << std::endl;
					checkSum = FIN_ACK->recvWnd;
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
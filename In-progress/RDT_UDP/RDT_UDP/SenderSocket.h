#ifndef SENDERSOCKET_H
#define SENDERSOCKET_H

#include <winsock.h>
#include <iostream>
#include <string>
#include "PacketHeader.h"

#define STATUS_OK			0 // no error
#define ALREADY_CONNECTED	1 // second call to ss.Open() without closing connection
#define NOT_CONNECTED		2 // call to ss.Send()/Close() without ss.Open()
#define INVALID_NAME		3 // ss.Open() with targetHost that has no DNS entry
#define FAILED_SEND			4 // sendto() failed in kernel
#define TIMEOUT				5 // timeout
#define FAILED_RECV			6 // recvfrom() failed in kernel
#define MAGIC_PORT			22345		//The port number that receiver listened
#define MAX_PKT_SIZE		(1500-28)	//Maximum UDP packet size accepted by receiver

class Sender_Status
{
public:
	int num_packet_timeout = 0;
	int num_packet_fast_retrans = 0;
	int effective_window = 0;
	int speed_goodput = 0;
	double estRTT = 0;
	Sender_Status(){ memset(this, 0, sizeof(Sender_Status)); }
};

class SenderSocket{
public:
	int Open(char *target_host, int magic_port, int sender_window, LinkProperties *p, DWORD time);
	int Send(char *data, int data_length);
	int Close(DWORD time);
	DWORD sequence_number = 0;
	Sender_Status current_status;
	DWORD checkSum;
private:
	LinkProperties link_property;
	int sender_window;
	double sample_RTT = 0;
	double prev_devRTT = -1;
	double prev_estRTT = 0;
	long RTO = 1e6;
	int current_ACK_number = 0;
	int port;
	char *host;
	SOCKET socket_UDP;
	struct sockaddr_in local, remote;
	int init_winsock();
	int UDP_send(char *IP_send_to, int port, char *message_send, int message_length, std::string *IP);
	bool UDP_init();
	int UDP_recv(char **buff);
	char *generate_SYN(LinkProperties *p);
	char *generate_FIN();
	char *generate_Data_message(char *data, int data_length);
	DWORD calculate_RTO();
	void update_parameter_for_RTO_calculation(DWORD sample_RTT1);
};

#endif

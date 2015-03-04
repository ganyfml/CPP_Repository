#include <iostream>
#include <winsock.h>
#include <cstdint>
#include <unordered_set>
#include <string>
#include <cstring>
#include <time.h>

#define MAX_ATTEMPTS 3
#define NDS_INET 1
#define DNS_A 1
#define DNS_PTR 12
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
	unsigned int TTL;
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
		num_copyed += length + 1;
		host = dot_position + 1;
	}
	request[num_copyed] = strlen(host);
	memcpy(request + num_copyed + 1, host, strlen(host));
}

void generate_DNS_request(char *host, char *DNS_request, int pkt_size, USHORT TXID)
{
	cout << "Lookup   : " << host << endl;
	memset(DNS_request, 0, pkt_size*sizeof(char));
	fixedDNSheader *dns_header = (fixedDNSheader *)DNS_request;
	queryHeader *query_header = (queryHeader*)(DNS_request + pkt_size - sizeof(queryHeader));
	dns_header->ID = htons(TXID);
	dns_header->flags = DNS_QUERY || DNS_RD || DNS_STDQUERY;
	dns_header->questions = htons(1);
	dns_header->answers = 0;
	dns_header->authority = 0;
	dns_header->addition = 0;
	query_header->qClass = htons(NDS_INET);
	if (inet_addr(host) == -1)
	{
		query_header->qType = htons(DNS_A);
		cout << "Query    : " << host << " , type " << DNS_A << " , TXID 0x" << std::hex << TXID << endl;
		generate_DNS_question(DNS_request + sizeof(fixedDNSheader), host);
	}
	else
	{
		string new_host;
		char *pos_next = strchr(host, '.');
		char *pos_prev = host;
		while (pos_next != NULL)
		{
			char temp[10] = "";
			memcpy(temp, pos_prev, pos_next - pos_prev);
			new_host.insert(0, temp);
			new_host.insert(0, ".");
			pos_prev = pos_next + 1;
			pos_next = strchr(pos_prev, '.');
		}
		char temp[10] = "";
		memcpy(temp, pos_prev, host + strlen(host) - pos_prev);
		new_host.insert(0, temp);
		new_host.append(".in-addr.arpa");
		new_host.c_str();
		query_header->qType = htons(DNS_PTR);
		cout << "Query    : " << new_host << " , type " << DNS_PTR << " , TXID " << std::hex << TXID << endl;
		generate_DNS_question(DNS_request + sizeof(fixedDNSheader), (char *)new_host.c_str());
	}
}

class UDP_connection
{
public:
	bool UDP_send(char *IP_send_to, char *message_send, int message_length)
	{
		memset(&remote, 0, sizeof(remote));
		remote.sin_family = AF_INET;
		remote.sin_addr.s_addr = inet_addr(IP_send_to);
		remote.sin_port = htons(53);
		if (sendto(socket_UDP, message_send, message_length, 0, (struct sockaddr*)&remote, sizeof(remote)) == SOCKET_ERROR)
			return false;
		return true;
	}
	bool UDP_init()
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
	int UDP_recv(char **buff)
	{
		*buff = new char[MAX_DNS_LEN];
		struct sockaddr addr;
		int fromlen = sizeof(addr);
		int byte_count = recvfrom(socket_UDP, *buff, 512, 0, &addr, &fromlen);
		return byte_count;
	}
	SOCKET get_UDP_socket()
	{
		return socket_UDP;
	}

private:
	SOCKET socket_UDP;
	struct sockaddr_in local, remote;
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
};

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


/*
bool display_data(char *data, int length, int init_index)
{
int index = init_index;
while (index < length)
{
fixedRR *first = (fixedRR *)(data + index);
//printf("%d\n", (USHORT)htons(first->name));
cout << "Name:";
if (!read_address(data, data[index + 1], length))
return false;
printf("Data Type: %d\n", (USHORT)htons(first->type));
if ((USHORT)htons(first->type) == 5)
{
if (!read_address(data, index + 12, length))
return false;
}
else
{
cout << "IP:";
display_IP(data, index + 12);
}
printf("Class: %d\n", (USHORT)htons(first->class_content));
printf("TTL: %d\n", htonl(first->TTL));
printf("Data length: %d\n", (USHORT)htons(first->length_data));
cout << endl;
index += (unsigned)htons(first->length_data) + 12;
cout << endl;
}
return true;
}*/

class DNS_reponse
{
	struct DNS_response_element
	{
		int num_content;
		vector<string>	address;
		vector<string>	name;
		vector<unsigned int> DNS_type;
		vector<unsigned int> DNS_class;
		vector<unsigned int> TTL;
	};

public:
	DNS_reponse(char *buff, int byte_count)
	{
		question_length = 0;
		response = buff;
		response_length = byte_count;
		fit_DNSheader();
	}

	bool check_TX_ID(USHORT send_ID)
	{
		if (send_ID != ID)	return false;
		else return true;
	}

	bool check_R_code()
	{
		unsigned char R = flag & 0xF;
		if (R == 0)
		{
			cout << "Succeeded with Rcode = 0" << endl;
			return true;
		}
		else if (R == 1)
		{
			cout << "Failed with Rcode = " << (int)R << ", format error" << endl;
			return true;
		}
		else if(R == 2)
		{
			cout << "Failed with Rcode = " << (int)R << ", server fail" << endl;
			return true;
		}
		else if(R == 3)
		{
			cout << "Failed with Rcode = " << (int)R << ", no such name" << endl;
			return true;
		}
		else
		{
			cout << "invaild record: truncated fixed RR header" << endl;
			return false;
		}
	}

	bool parse_data()
	{
		get_question();
		if(!get_answer_authrity_addition())	return false;
		else return true;
	}

	void display_stat()
	{
		cout << "TXID: 0x" << std::hex << ID << " Flag: 0x " << std::hex << flag;
		cout << " questions: " << question.num_content;
		cout << " answers: " << answer.num_content;
		cout << " authority: " << addition.num_content;
		cout << " addition: " << authority.num_content << endl;
		if (question.num_content > 0)
		{
			cout << "-----------------[Question]-----------------" << endl;
			for (int i = 0; i < question.num_content; i++)
			{
				cout << question.address.at(i) << " Type: " << question.DNS_type.at(i) << " Class: " << dec << question.DNS_class.at(i) << endl;
			}
		}
		if (answer.num_content > 0)
		{
			cout << "-----------------[Answer]-----------------" << endl;
			for (int i = 0; i < answer.num_content; i++)
			{
				cout << answer.name.at(i) << " " << get_type_name(answer.DNS_type.at(i)) << " " << answer.address.at(i) << " TTL: " << (int)answer.TTL.at(i) << endl;
			}
		}
		if (authority.num_content > 0)
		{
			cout << "-----------------[Authority]-----------------" << endl;
			for (int i = 0; i < authority.num_content; i++)
			{
				cout << authority.name.at(i) << " " << get_type_name(authority.DNS_type.at(i)) << " " << authority.address.at(i) << " TTL: " << (int)authority.TTL.at(i) << endl;
			}
		}
		if (addition.num_content > 0)
		{
			cout << "-----------------[Addition]-----------------" << endl;
			for (int i = 0; i < addition.num_content; i++)
			{
				cout << addition.name.at(i) << " " << get_type_name(addition.DNS_type.at(i)) << " " << addition.address.at(i) << " TTL: " << (int)addition.TTL.at(i) << endl;
			}
		}
	}

private:
	int question_length;
	int response_length;
	char *response;
	USHORT flag;
	USHORT ID;
	struct DNS_response_element question;
	struct DNS_response_element answer;
	struct DNS_response_element authority;
	struct DNS_response_element addition;

	void fit_DNSheader()
	{
		fixedDNSheader *read_result = (fixedDNSheader*)(response);
		flag = ntohs(read_result->flags);
		ID = ntohs(read_result->ID);
		question.num_content = ntohs(read_result->questions);
		answer.num_content = ntohs(read_result->answers);
		addition.num_content = ntohs(read_result->addition);
		authority.num_content = ntohs(read_result->authority);
	}

	string get_type_name(unsigned int a)
	{
		if (a == 1)	return "A";
		else if (a == 2) return "NS";
		else if (a == 5) return "CNAME";
		else if (a == 12) return "PTR";
		else return "Other Type";
	}

	void get_question()
	{
		int index = sizeof(fixedDNSheader);
		for (int i = 0; i < question.num_content; i++)
		{
			string temp_address = read_address(response, index, response_length);
			if (temp_address.length() != 0)
				question.address.push_back(temp_address);
			else question.address.push_back("Invaild address");
			queryHeader *query_header_temp = (queryHeader*)(response + index + temp_address.size() + 2);
			question.DNS_class.push_back(ntohs(query_header_temp->qClass));
			question.DNS_type.push_back(ntohs(query_header_temp->qType));
			index += temp_address.size() + 2 + sizeof(queryHeader);
		}
		question_length = index - sizeof(fixedDNSheader);
	}

	bool parse_RR_format(int *index, char *response, int response_length, struct DNS_response_element *element)
	{
		fixedRR *first = (fixedRR *)(response + *index);
		string temp_name = read_address(response, *index, response_length);
		if (!check_data_vaild(temp_name))	return false;
		element->name.push_back(temp_name);
		string temp_address;
		if (htons(first->type) == 5)
		{
			temp_address = read_address(response, *index + sizeof(fixedRR), response_length);
			if(!check_data_vaild(temp_address))	return false;
		}
		else
		{
			temp_address = read_IP(response, *index + sizeof(fixedRR));
		}
		element->address.push_back(temp_address);
		element->TTL.push_back(htonl(first->TTL));
		element->DNS_class.push_back(htons(first->class_content));
		element->DNS_type.push_back(htons(first->type));
		*index += htons(first->length_data) + sizeof(fixedRR);
		if (*index > response_length)
		{
			cout << "invalid record: value length beyond packet";
			return false;
		}
		return true;
	}

	bool get_answer_authrity_addition()
	{
		int index = sizeof(fixedDNSheader) + question_length;
		for (int i = 0; i < answer.num_content; i++)
		{
			if (!parse_RR_format(&index, response, response_length, &answer)) return false;
		}
		for (int i = 0; i < authority.num_content; i++)
		{
			if (!parse_RR_format(&index, response, response_length, &authority)) return false;
		}
		for (int i = 0; i < addition.num_content; i++)
		{
			if(!parse_RR_format(&index, response, response_length, &addition))	return false;
		}
		return true;
	}

	string read_IP(char *target, int index)
	{
		string IP;
		for (int i = 0; i < 4; i++)
		{
			IP.append(to_string((unsigned char)target[index + i]));
			IP.append(".");
		}
		IP.resize(IP.length() - 1);
		return IP;
	}

	bool check_data_vaild(string test)
	{
		if (test == "-1")
		{
			cout << "invaild record: jump beyond packet boundary";
			return false;
		}
		if (test == "-2")
		{
			cout << "invaild record: truncated name";
			return false;
		}
		if (test == "-3")
		{
			cout << "invaild record: truncated fixed RR header";
			return false;
		}
		if (test == "-4")
		{
			cout << "invaild record: truncated jump offset";
			return false;
		}
		if (test == "-5")
		{
			cout << "invaild record: jump into fixed header";
			return false;
		}
		if (test == "-6")
		{
			cout << "invaild record: jump loop";
			return false;
		}
		if (test == "-7")
		{
			cout << "invaild record: value length beyond packet";
			return false;
		}
		if (test == "-8")
		{
			cout << "invaild record: not enough record";
			return false;
		}
		else return true;
	}


	string read_address(char *target, int index_init, int total_data_length)
	{
		int index = index_init;
		string address;
		unordered_set<int> index_history;
		index_history.insert(index);
		while (true)
		{
			int number = (unsigned char)target[index++];
			if (number == 0)
			{
				break;
			}
			if (number < 192)
			{
				for (int i = 0; i < number; i++)
				{
					if (index + i>total_data_length)	return "-7";//value length beyond packet;
					if (target[index + i] == 0)
						return "-2";//truncated name
					address.append(target + index + i, 1);
				}
				address.append(".");
				index += number;
			}
			else
			{
				index = (unsigned char)target[index];
				if (index == 0)
					return "-4";//truncated jump offset;
			}
			//Check self-loop and out of bound
			if (index >= total_data_length)	
			{
				return "-1";//jumpy beyond packet poundary
			}
			if (index < sizeof(fixedDNSheader))
			{
				return "-5";
			}//jump into fixed header
			int old_length = index_history.size();
			index_history.insert(index);
			int new_length = index_history.size();
			if (new_length == old_length)
			{
				return "-6";//jump loop
			}
		}
		address.resize(address.length() - 1);
		return address;
	}
};

int main()
{
	/*
	if (argc != 3)
	{
		cout << "Invild argument" << endl;
		cout << "argument format: [lookup string] [server]" << endl;
		return 1;
	}
	*/
	char *host = "yahoo.com";
	char *server = "8.8.8.8";
	char *buff;
	UDP_connection UDP_connect;
	UDP_connect.UDP_init();
	int pktsize;
	if (inet_addr(host) == -1)
		pktsize = strlen(host) + 2 + sizeof(fixedDNSheader) + sizeof(queryHeader);
	else pktsize = strlen(host) + 2 + sizeof(fixedDNSheader) + sizeof(queryHeader) + 12;
	char *request = new char[pktsize];

	srand(time(NULL));
	rand();
	USHORT TXID = rand() % 65535;
	generate_DNS_request(host, request, pktsize, TXID);
	cout << "Server   : " << server << endl;
	cout << "*******************************************" << endl;

	int try_count = 0;
	int byte_count = 0;
	while (try_count++ < MAX_ATTEMPTS)
	{
		DWORD time1 = timeGetTime();
		cout << "Attempting " << try_count << " with " << pktsize << " bytes...";
		UDP_connect.UDP_send(server, request, pktsize);
		fd_set fd;

		struct timeval time_threshold;
		time_threshold.tv_sec = 10;
		time_threshold.tv_usec = 0;

		FD_ZERO(&fd);
		FD_SET(UDP_connect.get_UDP_socket(), &fd);
		int avaiable = select(0, &fd, NULL, NULL, &time_threshold);

		if (avaiable > 0)
		{
			DWORD time2 = timeGetTime();
			byte_count = UDP_connect.UDP_recv(&buff);
			cout << "response in " << dec << time2 - time1 << " ms with " << byte_count << " bytes" << endl;
			if (byte_count < sizeof(fixedDNSheader))
			{
				cout << "Smaller then fixed header!";
				return 1;
			}
			DNS_reponse reponse(buff, byte_count);
			if (!reponse.check_TX_ID(TXID))
			{
				cout << "TXID not match!! Program exit" << endl;
				return 1;
			}
			if (!reponse.check_R_code())	return 1;
			if (!reponse.parse_data())	return 1;
			reponse.display_stat();
			return 0;
		}
		else
		{
			DWORD time2 = timeGetTime();
			cout << "timeout in " << time2 - time1 << " ms" << endl;
		}
	}
}

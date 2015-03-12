#include <winsock.h>
#include <iostream>
#include "SenderSocket.h"

int main(int argc, char ** argv)
{
	if (argc != 8)
	{
		std::cout << "RDT_UDP.exe [destination host] [buffer size] [sender window] [RTT] [loss_rate_forward] [loss_rate_return_path] [speed]" << std::endl;
		return -1;
	}

	LinkProperties p;
	char *target_host = argv[1];
	//char *target_host = "127.0.0.1";
	int power = atof(argv[2]);
	int sender_window = atof(argv[3]);
	p.bufferSize = 1;
	p.pLoss[FORWARD_PATH] = atof(argv[5]);
	p.pLoss[RETURN_PATH] = atof(argv[6]);
	p.RTT = atof(argv[4]);
	p.speed = 1e6 * atof(argv[7]);
	if ((p.speed/1e6) >10000 || p.speed <= 0 || p.RTT <= 0 || p.RTT > 30 || p.pLoss[FORWARD_PATH] >= 1 || p.pLoss[FORWARD_PATH] < 0 || p.pLoss[RETURN_PATH] >= 1 || p.pLoss[RETURN_PATH] < 0 || power <= 0 || sender_window <= 0)
	{
		std::cout << "invild parameter detected!, program exits" << std::endl;
		return -1;
	}

	DWORD program_init_time = timeGetTime();

	std::cout << "Main:  sender W = " << sender_window << ", RTT " << p.RTT << " sec, lose " << p.pLoss[FORWARD_PATH] << " / " << p.pLoss[RETURN_PATH]  << ", link " << p.speed/1e6 << " Mbps"<< std::endl;
	UINT64 dwordBufSize = (UINT64)1 << power;
	DWORD *dwordBuf = new DWORD[dwordBufSize];
	std::cout << "Main:  initializing DWORD array with 2^" << power << " elements...";
	DWORD time1 = timeGetTime();
	for (int i = 0; i < dwordBufSize; i++)	dwordBuf[i] = i;
	std::cout << "done in " << timeGetTime() - time1 << " ms" << std::endl;
	int status;
	SenderSocket ss;

	if ((status = ss.Open(target_host, MAGIC_PORT, sender_window, &p, timeGetTime() - program_init_time)) != STATUS_OK)
	{
		std::cout << "Main:  Connect failed with status " << status << std::endl;
		return -1;
	}

	char *charBuf = (char*)dwordBuf;
	UINT64 byteBufferSize = dwordBufSize << 2;

	std::cout << "Main:  Connect to " << target_host << " in " << (float)(timeGetTime() - program_init_time) / 1000 << " sec, pkt: " << MAX_PKT_SIZE << " bytes" << std::endl;

	UINT64 off = 0;
	while (off < byteBufferSize)
	{
		int bytes = min(byteBufferSize - off, MAX_PKT_SIZE - sizeof(SenderDataHeader));
		if ((status = ss.Send(charBuf+off,bytes)) != STATUS_OK)
		{
			//TODO
		}
		off += bytes;
	}

	if ((status = ss.Close(timeGetTime() - program_init_time)) != STATUS_OK)
	{
		std::cout << "Main:  Connect failed with status " << status << std::endl;
		return -1;
	}
	std::cout << "Main:  transfer finished in " << (float)(timeGetTime() - program_init_time) / 1000 << " seconds"<< std::endl;
	return 0;
}
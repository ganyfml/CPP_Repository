#include <winsock.h>
#include <iostream>
#include "SenderSocket.h"
#include "CheckSum.h"
#include <iomanip>

const bool debug_mode = false;

class Parameters {
public:
	HANDLE display_thread_finished;
	HANDLE trans_finished;
	DWORD starting_time;
	SenderSocket *sender_socket;
};

DWORD WINAPI StatsRun(LPVOID pParam)
{
	Parameters *p = ((Parameters*)pParam);
	DWORD init_time = p->starting_time;
	int count = 2;
	int package_since_last_trans = 0;
	DWORD last_report_time = timeGetTime();
	std::cout << std::setprecision(3);
	while (WaitForSingleObject(p->trans_finished, 2000) == WAIT_TIMEOUT)
	{
		DWORD this_report_time = timeGetTime();
		std::cout << "[" << (this_report_time - init_time)*1e-3 << "]     ";
		std::cout << "B       " << p->sender_socket->sequence_number;
		std::cout << "(" << " " << (p->sender_socket->sequence_number * (MAX_PKT_SIZE - sizeof(SenderDataHeader))) * 1e-6 << " MB)";
		std::cout << " " << p->sender_socket->sequence_number + 1;
		std::cout << " T " << p->sender_socket->current_status.num_packet_timeout;
		std::cout << " F " << p->sender_socket->current_status.num_packet_fast_retrans;
		std::cout << " W " << p->sender_socket->current_status.effective_window;
		std::cout << " S " << ((8 * (MAX_PKT_SIZE - sizeof(SenderDataHeader)) * (p->sender_socket->sequence_number - 1 - package_since_last_trans)) / ((this_report_time - last_report_time)*1e-3))*1e-6 << " Mbps";
		std::cout << " RTT " << p->sender_socket->current_status.estRTT;
		std::cout << std::endl;
		package_since_last_trans = p->sender_socket->sequence_number - 1;
		last_report_time = this_report_time;
	}
	SetEvent(p->display_thread_finished);
	return 0;
}

int main(int argc, char ** argv)
{
	LinkProperties p;
	char *target_host;
	int sender_window;
	int power;
	Parameters p_thread;
	p_thread.display_thread_finished = CreateEvent(NULL, true, false, NULL);
	p_thread.trans_finished = CreateEvent(NULL, true, false, NULL);

	if (!debug_mode)
	{
		if (argc != 8)
		{
			std::cout << "RDT_UDP.exe [destination host] [buffer size] [sender window] [RTT] [loss_rate_forward] [loss_rate_return_path] [speed]" << std::endl;
			return -1;
		}
		target_host = argv[1];
		power = atof(argv[2]);
		sender_window = atof(argv[3]);
		p.bufferSize = 1;
		p.RTT = atof(argv[4]);
		p.pLoss[FORWARD_PATH] = atof(argv[5]);
		p.pLoss[RETURN_PATH] = atof(argv[6]);
		p.speed = 1e6 * atof(argv[7]);

	}
	else
	{
		target_host = "127.0.0.1";
		//target_host = "s8.irl.cs.tamu.edu";
		power = 15;
		sender_window = 1;
		p.RTT = 0.1;
		p.bufferSize = 1;
		p.pLoss[FORWARD_PATH] = 0.1;
		p.pLoss[RETURN_PATH] = 0;
		p.speed = 1e6 * 14;
	}

	if ((p.speed / 1e6) > 10000 || p.speed <= 0 || p.RTT <= 0 || p.RTT > 30 || p.pLoss[FORWARD_PATH] >= 1 || p.pLoss[FORWARD_PATH] < 0 || p.pLoss[RETURN_PATH] >= 1 || p.pLoss[RETURN_PATH] < 0 || power <= 0 || sender_window <= 0)
	{
		std::cout << "invild parameter detected!, program exits" << std::endl;
		return -1;
	}

	DWORD program_init_time = timeGetTime();
	std::cout << "Main:  sender W = " << sender_window << ", RTT " << p.RTT << " sec, lose " << p.pLoss[FORWARD_PATH] << " / " << p.pLoss[RETURN_PATH] << ", link " << p.speed / 1e6 << " Mbps" << std::endl;
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
	p_thread.starting_time = program_init_time;
	p_thread.sender_socket = &ss;
	HANDLE handles_1 = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)StatsRun, &p_thread, 0, NULL);
	DWORD sending_start_time = timeGetTime();
	while (off < byteBufferSize)
	{
		int bytes = min(byteBufferSize - off, MAX_PKT_SIZE - sizeof(SenderDataHeader));
		if ((status = ss.Send(charBuf + off, bytes)) != STATUS_OK)
		{
			std::cout << "Sending failed" << std::endl;
			return -1;
		}
		off += bytes;
	}

	SetEvent(p_thread.trans_finished);
	WaitForSingleObject(p_thread.display_thread_finished, INFINITE);

	if ((status = ss.Close(timeGetTime() - program_init_time)) != STATUS_OK)
	{
		std::cout << "Main:  Connect failed with status " << status << std::endl;
		return -1;
	}
	Checksum cs;
	DWORD check = cs.CRC32((unsigned char *)charBuf, byteBufferSize);
	std::cout << "Main:  transfer finished in " << (float)(timeGetTime() - sending_start_time) / 1000 << " seconds" << ", " << (ss.sequence_number - 1) * 8 * (MAX_PKT_SIZE - sizeof(SenderDataHeader))* 1e-3 / ((float)(timeGetTime() - sending_start_time) / 1000) << " Kbps, checksum " << std::hex << check << std::endl;
	std::cout << "Main:  estRTT " << ss.current_status.estRTT << ", ideal rate: " << (double)(ss.current_status.effective_window / ss.current_status.estRTT) * 8 * (MAX_PKT_SIZE - sizeof(SenderDataHeader)) * 1e-3 << " Kbps " << std::endl;
	if (check != ss.checkSum)
		std::cout << "Checksum not match! Transmission failed!" << std::endl;
	return 0;
}
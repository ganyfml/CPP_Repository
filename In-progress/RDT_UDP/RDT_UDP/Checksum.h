#ifndef CHECK_H
#define CHECK_H

#include <winsock.h>

class Checksum
{
public:
	Checksum();
	DWORD CRC32(unsigned char *buf, size_t len);
private:
	DWORD crc_table[256];
};

#endif
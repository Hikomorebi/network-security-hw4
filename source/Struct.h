#ifndef  DATA_STRUCTURE_H
#define  DATA_STRUCTURE_H
#include "common.h"

struct TCPConThrParam
{
	string HostIP;
	unsigned BeginPort;
	unsigned EndPort;
};

struct TCPConHostThrParam
{
	string HostIP;
	unsigned HostPort;
};

struct UDPThrParam
{
	string HostIP;
	unsigned BeginPort;
	unsigned EndPort;
	unsigned LocalHostIP;
};

struct UDPScanHostThrParam
{
	string HostIP;
	unsigned HostPort;
    unsigned LocalPort;
	unsigned LocalHostIP;
};

struct TCPSYNThrParam
{
	string HostIP;
	unsigned BeginPort;
	unsigned EndPort;
	unsigned LocalHostIP;
};

struct TCPSYNHostThrParam
{
	string HostIP;
	unsigned HostPort;
    unsigned LocalPort;
	unsigned LocalHostIP;
};

struct TCPFINThrParam
{
	string HostIP;
	unsigned BeginPort;
	unsigned EndPort;
	unsigned LocalHostIP;
};

struct TCPFINHostThrParam
{
	string HostIP;
	unsigned HostPort;
    unsigned LocalPort;
	unsigned LocalHostIP;
};

struct ipicmphdr 
{ 
	struct iphdr ip; 
	struct icmphdr icmp; 
}; 

struct pseudohdr   
{  
	unsigned long saddr; 
	unsigned long daddr; 
	char useless; 
	unsigned char protocol; 
	unsigned short length; 
};

#endif

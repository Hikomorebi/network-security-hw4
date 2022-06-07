#include "common.h"
#include "Struct.h"
#include "HelpFunc.h"

int TCPFinThrdNum;

// locker
pthread_mutex_t TCPFinPrintlocker = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t TCPFinScanlocker = PTHREAD_MUTEX_INITIALIZER;

extern unsigned short in_cksum(unsigned short *ptr, int nbytes);
void *Thread_TCPFINHost(void *param)
{
	struct TCPFINHostThrParam *p;
	string HostIP, SrcIP, DstIP, LocalIP;
	unsigned HostPort, LocalPort, SrcPort, DstPort, LocalHostIP;

	struct sockaddr_in FINScanHostAddr, FromAddr, FinRevAddr;
	struct in_addr in_LocalhostIP;

	int FinSock, FinRevSock;
	int len, FromAddrLen;

	char sendbuf[8192];
	char recvbuf[8192];

	struct timeval TpStart, TpEnd;
	float TimeUse;

	p = (struct TCPFINHostThrParam *)param;
	HostIP = p->HostIP;
	HostPort = p->HostPort;
	LocalPort = p->LocalPort;
	LocalHostIP = p->LocalHostIP;

	memset(&FINScanHostAddr, 0, sizeof(FINScanHostAddr));
	FINScanHostAddr.sin_family = AF_INET;
	FINScanHostAddr.sin_addr.s_addr = inet_addr(&HostIP[0]);
	FINScanHostAddr.sin_port = htons(HostPort);

	FinSock = socket(PF_INET, SOCK_RAW, IPPROTO_TCP);
	if (FinSock < 0)
	{
		pthread_mutex_lock(&TCPFinPrintlocker);
		cout << "Can't creat raw socket !" << endl;
		pthread_mutex_unlock(&TCPFinPrintlocker);
	}

	FinRevSock = socket(PF_INET, SOCK_RAW, IPPROTO_TCP);
	if (FinRevSock < 0)
	{
		pthread_mutex_lock(&TCPFinPrintlocker);
		cout << "Can't creat raw socket !" << endl;
		pthread_mutex_unlock(&TCPFinPrintlocker);
	}

	struct pseudohdr *ptcph = (struct pseudohdr *)sendbuf;
	struct tcphdr *tcph = (struct tcphdr *)(sendbuf + sizeof(struct pseudohdr));

	ptcph->saddr = LocalHostIP;
	ptcph->daddr = inet_addr(&HostIP[0]);
	ptcph->useless = 0;
	ptcph->protocol = IPPROTO_TCP;
	ptcph->length = htons(sizeof(struct tcphdr));

	tcph->th_sport = htons(LocalPort);
	tcph->th_dport = htons(HostPort);
	tcph->th_seq = htonl(123456);
	tcph->th_ack = 0;
	tcph->th_x2 = 0;
	tcph->th_off = 5;
	tcph->th_flags = TH_FIN;
	tcph->th_win = htons(65535);
	tcph->th_sum = 0;
	tcph->th_urp = 0;
	tcph->th_sum = in_cksum((unsigned short *)ptcph, 20 + 12);

	len = sendto(FinSock, tcph, 20, 0, (struct sockaddr *)&FINScanHostAddr, sizeof(FINScanHostAddr));
	if (len < 0)
	{
		pthread_mutex_lock(&TCPFinPrintlocker);
		cout << "Send TCP FIN Packet failed !" << endl;
		pthread_mutex_unlock(&TCPFinPrintlocker);
	}

	if (fcntl(FinRevSock, F_SETFL, O_NONBLOCK) == -1)
	{
		pthread_mutex_lock(&TCPFinPrintlocker);
		cout << "Set socket in non-blocked model failed !" << endl;
		pthread_mutex_unlock(&TCPFinPrintlocker);
	}

	FromAddrLen = sizeof(struct sockaddr_in);

	gettimeofday(&TpStart, NULL);
	do
	{
		len = recvfrom(FinRevSock, recvbuf, sizeof(recvbuf), 0, (struct sockaddr *)&FromAddr, (socklen_t *)&FromAddrLen);
		if (len > 0)
		{
			SrcIP = inet_ntoa(FromAddr.sin_addr);
			if (SrcIP == HostIP)
			{
				struct ip *iph = (struct ip *)recvbuf;
				int i = iph->ip_hl * 4;
				struct tcphdr *tcph = (struct tcphdr *)&recvbuf[i];

				SrcIP = inet_ntoa(iph->ip_src);
				DstIP = inet_ntoa(iph->ip_dst);

				in_LocalhostIP.s_addr = LocalHostIP;
				LocalIP = inet_ntoa(in_LocalhostIP);

				unsigned SrcPort = ntohs(tcph->th_sport);
				unsigned DstPort = ntohs(tcph->th_dport);

				if (HostIP == SrcIP && LocalIP == DstIP && SrcPort == HostPort && DstPort == LocalPort)
				{
					if (tcph->th_flags == 0x14)
					{
						pthread_mutex_lock(&TCPFinPrintlocker);
						cout << "Host: " << SrcIP << " Port: " << ntohs(tcph->th_sport) << " closed !" << endl;
						pthread_mutex_unlock(&TCPFinPrintlocker);
						break;
					}
				}
				
			}
		}

		gettimeofday(&TpEnd, NULL);
		TimeUse = (1000000 * (TpEnd.tv_sec - TpStart.tv_sec) + (TpEnd.tv_usec - TpStart.tv_usec)) / 1000000.0;
		if (TimeUse < 5)
		{
			continue;
		}
		else
		{
			pthread_mutex_lock(&TCPFinPrintlocker);
			cout << "Host: " << HostIP << " Port: " << HostPort << " open !" << endl;
			pthread_mutex_unlock(&TCPFinPrintlocker);
			break;
		}
	} while (true);

	delete p;

	close(FinSock);
	close(FinRevSock);

	pthread_mutex_lock(&TCPFinScanlocker);
	TCPFinThrdNum--;
	pthread_mutex_unlock(&TCPFinScanlocker);
}

void *Thread_TCPFinScan(void *param)
{
	struct TCPFINThrParam *p;

	string HostIP;
	unsigned BeginPort, EndPort, TempPort, LocalPort, LocalHostIP;

	pthread_t listenThreadID, subThreadID;
	pthread_attr_t attr, lattr;
	int ret;

	p = (struct TCPFINThrParam *)param;
	HostIP = p->HostIP;
	BeginPort = p->BeginPort;
	EndPort = p->EndPort;
	LocalHostIP = p->LocalHostIP;

	TCPFinThrdNum = 0;
	LocalPort = 1024;

	for (TempPort = BeginPort; TempPort <= EndPort; TempPort++)
	{

		struct TCPFINHostThrParam *pTCPFINHostParam = new TCPFINHostThrParam;
		pTCPFINHostParam->HostIP = HostIP;
		pTCPFINHostParam->HostPort = TempPort;
		pTCPFINHostParam->LocalPort = TempPort + LocalPort;
		pTCPFINHostParam->LocalHostIP = LocalHostIP;

		pthread_attr_init(&attr);
		pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

		ret = pthread_create(&subThreadID, &attr, Thread_TCPFINHost, pTCPFINHostParam);
		if (ret == -1)
		{
			cout << "Create the TCP FIN Scan Host failed !" << endl;
		}

		pthread_attr_destroy(&attr);

		pthread_mutex_lock(&TCPFinScanlocker);
		TCPFinThrdNum++;
		pthread_mutex_unlock(&TCPFinScanlocker);

		while (TCPFinThrdNum > 100)
		{
			sleep(3);
		}
	}

	while (TCPFinThrdNum != 0)
	{
		sleep(1);
	}

	cout << "TCP FIN scan thread exit !" << endl;
	pthread_exit(NULL);
}

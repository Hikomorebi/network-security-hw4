#include "common.h"
#include "Struct.h"
#include "HelpFunc.h"
int TCPSynThrdNum;

pthread_mutex_t TCPSynPrintlocker = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t TCPSynScanlocker = PTHREAD_MUTEX_INITIALIZER;

extern unsigned short in_cksum(unsigned short *ptr, int nbytes);
void *Thread_TCPSYNHost(void *param)
{
	struct TCPSYNHostThrParam *p;
	string HostIP;
	unsigned HostPort, LocalPort, LocalHostIP;
	int SynSock;
	int len;
	char sendbuf[8192];
	char recvbuf[8192];
	struct sockaddr_in SYNScanHostAddr;

	//获得目标主机的 IP 地址和扫描端口号，以及本机的 IP 地址和端口
	p = (struct TCPSYNHostThrParam *)param;
	HostIP = p->HostIP;
	HostPort = p->HostPort;
	LocalPort = p->LocalPort;
	LocalHostIP = p->LocalHostIP;

	//设置 TCP SYN 扫描的套接字地址
	memset(&SYNScanHostAddr, 0, sizeof(SYNScanHostAddr));
	SYNScanHostAddr.sin_family = AF_INET;
	SYNScanHostAddr.sin_addr.s_addr = inet_addr(&HostIP[0]);
	SYNScanHostAddr.sin_port = htons(HostPort);

	//创建套接字
	SynSock = socket(PF_INET, SOCK_RAW, IPPROTO_TCP);
	if (SynSock < 0)
	{
		pthread_mutex_lock(&TCPSynPrintlocker);
		cout << "Can't creat raw socket !" << endl;
		pthread_mutex_unlock(&TCPSynPrintlocker);
	}

	//填充 TCP 头
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
	tcph->th_flags = TH_SYN;
	tcph->th_win = htons(65535);
	tcph->th_sum = 0;
	tcph->th_urp = 0;
	tcph->th_sum = in_cksum((unsigned short *)ptcph, 20 + 12);
	int try_time = 10;
	//发送 TCP SYN 数据包
	pthread_mutex_lock(&TCPSynScanlocker);
	len = sendto(SynSock, tcph, 20, 0, (struct sockaddr *)&SYNScanHostAddr, sizeof(SYNScanHostAddr));
	if (len < 0)
	{
		pthread_mutex_lock(&TCPSynPrintlocker);
		cout << "Send TCP SYN Packet failed !" << endl;
		pthread_mutex_unlock(&TCPSynPrintlocker);
	}

	//接收目标主机的 TCP 响应数据包
	while (true)
	{
		len = read(SynSock, recvbuf, 8192);
		if (len <= 0)
		{
			pthread_mutex_lock(&TCPSynPrintlocker);
			cout << "Read TCP SYN Packet failed !" << endl;
			pthread_mutex_unlock(&TCPSynPrintlocker);
		}
		else
		{
			struct ip *iph = (struct ip *)recvbuf;
			int i = iph->ip_hl * 4;
			struct tcphdr *tcph = (struct tcphdr *)&recvbuf[i];

			string SrcIP = inet_ntoa(iph->ip_src);
			string DstIP = inet_ntoa(iph->ip_dst);
			struct in_addr in_LocalhostIP;
			in_LocalhostIP.s_addr = LocalHostIP;
			string LocalIP = inet_ntoa(in_LocalhostIP);

			unsigned SrcPort = ntohs(tcph->th_sport);
			unsigned DstPort = ntohs(tcph->th_dport);

			if (HostIP == SrcIP && LocalIP == DstIP && SrcPort == HostPort && DstPort == LocalPort)
			{
				if (tcph->th_flags == 0x14)
				{
					pthread_mutex_lock(&TCPSynPrintlocker);
					cout << "Host: " << SrcIP << " Port: " << ntohs(tcph->th_sport) << " closed !" << endl;
					pthread_mutex_unlock(&TCPSynPrintlocker);
					break;
				}
				if (tcph->th_flags == 0x12)
				{
					pthread_mutex_lock(&TCPSynPrintlocker);
					cout << "Host: " << SrcIP << " Port: " << ntohs(tcph->th_sport) << " open !" << endl;
					pthread_mutex_unlock(&TCPSynPrintlocker);
					break;
				}
			}
		}
	}
	delete p;
	close(SynSock);
	pthread_mutex_unlock(&TCPSynScanlocker);
	pthread_mutex_lock(&TCPSynScanlocker);
	TCPSynThrdNum--;
	pthread_mutex_unlock(&TCPSynScanlocker);
}

void *Thread_TCPSynScan(void *param)
{
	struct TCPSYNThrParam *p;

	string HostIP;
	unsigned BeginPort, EndPort, TempPort, LocalPort, LocalHostIP;

	pthread_t listenThreadID, subThreadID;
	pthread_attr_t attr, lattr;
	int ret;

	//获得目标主机的 IP 地址和扫描的起始端口号，终止端口号，以及本机的 IP 地址
	p = (struct TCPSYNThrParam *)param;
	HostIP = p->HostIP;
	BeginPort = p->BeginPort;
	EndPort = p->EndPort;
	LocalHostIP = p->LocalHostIP;

	//循环遍历扫描端口
	TCPSynThrdNum = 0;
	LocalPort = 1024;

	for (TempPort = BeginPort; TempPort <= EndPort; TempPort++)
	{
		//设置子线程参数
		struct TCPSYNHostThrParam *pTCPSYNHostParam = new TCPSYNHostThrParam;
		pTCPSYNHostParam->HostIP = HostIP;
		pTCPSYNHostParam->HostPort = TempPort;
		pTCPSYNHostParam->LocalPort = TempPort + LocalPort;
		pTCPSYNHostParam->LocalHostIP = LocalHostIP;

		//将子线程设置为分离状
		pthread_attr_init(&attr);
		pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

		//创建子线程
		ret = pthread_create(&subThreadID, &attr, Thread_TCPSYNHost, pTCPSYNHostParam);
		if (ret == -1)
		{
			cout << "Create the TCP SYN Scan Host thread failed !" << endl;
		}
		pthread_attr_destroy(&attr);
		pthread_mutex_lock(&TCPSynScanlocker);
		TCPSynThrdNum++;
		pthread_mutex_unlock(&TCPSynScanlocker);

		while (TCPSynThrdNum > 100)
		{
			sleep(3);
		}
	}
	while (TCPSynThrdNum != 0)
	{
		sleep(1);
	}

	cout << "TCP SYN scan thread exit !" << endl;
	pthread_exit(NULL);
}

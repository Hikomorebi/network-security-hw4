#include "common.h"
#include "Struct.h"
#include "HelpFunc.h"

int TCPConThrdNum;

pthread_mutex_t TCPConPrintlocker=PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t TCPConScanlocker=PTHREAD_MUTEX_INITIALIZER;

void* Thread_TCPconnectHost(void* param)
{
	struct TCPConHostThrParam *p;
	string HostIP;
	unsigned HostPort;

    int ConSock;
	struct sockaddr_in HostAddr; 
	int ret;

    p=(struct TCPConHostThrParam*)param;
    HostIP = p->HostIP;
	HostPort = p->HostPort;

	ConSock = socket(AF_INET,SOCK_STREAM,0);
	if(ConSock < 0)
	{
		pthread_mutex_lock(&TCPConPrintlocker);
		cout<<"Create TCP connect Socket failed! "<<endl;
		pthread_mutex_unlock(&TCPConPrintlocker);
	}
    

	memset(&HostAddr,0,sizeof(HostAddr));
	HostAddr.sin_family = AF_INET;
	HostAddr.sin_addr.s_addr = inet_addr(&HostIP[0]);
	HostAddr.sin_port = htons(HostPort);

    ret = connect(ConSock,(struct sockaddr*)&HostAddr,sizeof(HostAddr));
	if(ret==-1)
	{
		pthread_mutex_lock(&TCPConPrintlocker);
	    cout<<"Host: "<<HostIP<<" Port: "<<HostPort<<" closed ! "<<endl;
		pthread_mutex_unlock(&TCPConPrintlocker);
	}
	else
	{
		pthread_mutex_lock(&TCPConPrintlocker);
        cout<<"Host: "<<HostIP<<" Port: "<<HostPort<<" open ! "<<endl;
		pthread_mutex_unlock(&TCPConPrintlocker);
	}

	delete p;
    close(ConSock);
	
	pthread_mutex_lock(&TCPConScanlocker);
        TCPConThrdNum--;
	pthread_mutex_unlock(&TCPConScanlocker);

}

void* Thread_TCPconnectScan(void* param)
{
    struct TCPConThrParam *p;

	string HostIP;
	unsigned BeginPort,EndPort,TempPort;
	
	pthread_t subThreadID;
	pthread_attr_t attr;
	int ret;
    

	p=(struct TCPConThrParam*)param;
	HostIP = p->HostIP;
	BeginPort = p->BeginPort;
	EndPort = p->EndPort;

    TCPConThrdNum = 0;

	for (TempPort=BeginPort;TempPort<=EndPort;TempPort++) 
	{

        TCPConHostThrParam *pConHostParam = new TCPConHostThrParam;
        pConHostParam->HostIP = HostIP;
		pConHostParam->HostPort = TempPort;
		

		pthread_attr_init(&attr);
        pthread_attr_setdetachstate(&attr,PTHREAD_CREATE_DETACHED);
        

		ret=pthread_create(&subThreadID,&attr,Thread_TCPconnectHost,pConHostParam);
		if (ret==-1) 
		{
			cout<<"Can't create the TCP connect Host thread !"<<endl;
		}
		

		pthread_attr_destroy(&attr);

		pthread_mutex_lock(&TCPConScanlocker);
        TCPConThrdNum++;
		pthread_mutex_unlock(&TCPConScanlocker);

		while (TCPConThrdNum>100)
		{
			sleep(3);
		}
	}

	while (TCPConThrdNum != 0)
	{
		sleep(1);
	}
    cout<<"TCP Connect Scan thread exit !"<<endl;
	pthread_exit(NULL);
}

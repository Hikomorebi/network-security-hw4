#include "common.h"
#include "Struct.h"
#include "HelpFunc.h"

extern void* Thread_TCPconnectScan(void*);
extern void* Thread_UDPScan(void* );
extern void* Thread_TCPSynScan(void* );
extern void* Thread_TCPFinScan(void* );
int main(int argc, char *argv[])
{ // argc=外部命令参数的个数，argv[]存放各参数
    string HostIP;
    unsigned BeginPort, EndPort, LocalHostIP;
    int ret;

    struct TCPConThrParam TCPConParam;
    struct UDPThrParam UDPParam;
    struct TCPSYNThrParam TCPSynParam;
    struct TCPFINThrParam TCPFinParam;
    pthread_t ThreadID;
    unordered_map<string, int> mapOp = {
        {"-c", 1},
        {"-s", 2},
        {"-f", 3},
        {"-u", 4},
    };
    if (argc < 2)
    {
        cout << "参数错误，argc = " << argc << endl;
        return -1;
    }
    string op = argv[1];
    if (op == "-h")
    {
        cout << "Scaner:usage：" << "\t" << "[-h] --help information" << endl
         << "\t\t" << "[-c] --TCP connect scan" << endl
         << "\t\t" << "[-s] --TCP syn scan" << endl
         << "\t\t" << "[-f] --TCP fin scan" << endl
         << "\t\t" << "[-u] --UDP scan" << endl;
        exit(0);
    }
    cout << "Please input IP address of a Host:";
    cin >> HostIP;

    if (inet_addr(&(HostIP[0])) == INADDR_NONE)
    {
        cout << "IP address wrong!" << endl;
        return -1;
    }
    cout << "Please input the range of port..." << endl;
    cout << "Begin Port:";
    cin >> BeginPort;
    cout << "End Port:";
    cin >> EndPort;

    if (IsPortOK(BeginPort, EndPort))
    {
        cout << "Scan Host " << HostIP << " port " << BeginPort << "~" << EndPort << " ..." << endl;
    }
    else
    {
        cout << "The range of port wrong!" << endl;
        return -1;
    }
    LocalHostIP = GetLocalHostIP();
    if (Ping(HostIP, LocalHostIP) == false)
    {
        cout << "Ping Host " << HostIP << " failed, stop scan it !" << endl;
        return -1;
    }
    if (mapOp.find(op) != mapOp.end())
    {
        switch (mapOp[op])
        {
        case 1: // 进行 TCP connect 扫描
            cout << "Begin TCP connect scan..." << endl;
            TCPConParam.HostIP = HostIP;
            TCPConParam.BeginPort = BeginPort;
            TCPConParam.EndPort = EndPort;
            ret = pthread_create(&ThreadID, NULL, Thread_TCPconnectScan, &TCPConParam);
            if (ret == -1)
            {
                cout << "Can't create the TCP connect scan thread !" << endl;
                return -1;
            }
            ret = pthread_join(ThreadID, NULL);
            if (ret != 0)
            {
                cout << "call pthread_join function failed !" << endl;
                return -1;
            }
            else
            {
                cout << "TCP Connect Scan finished !" << endl;
                return 0;
            }
            break;
        case 2: // 进行 TCP SYN 扫描
            cout << "Begin TCP SYN scan..." << endl;
            // create thread for TCP SYN scan
            TCPSynParam.HostIP = HostIP;
            TCPSynParam.BeginPort = BeginPort;
            TCPSynParam.EndPort = EndPort;
            TCPSynParam.LocalHostIP = LocalHostIP;
            ret = pthread_create(&ThreadID, NULL, Thread_TCPSynScan, &TCPSynParam);
            if (ret == -1)
            {
                cout << "create the TCP SYN scan thread failed !" << endl;
                return -1;
            }

            ret = pthread_join(ThreadID, NULL);
            if (ret != 0)
            {
                cout << "call pthread_join function failed !" << endl;
                return -1;
            }
            else
            {
                cout << "TCP SYN Scan finished !" << endl;
                return 0;
            }
            break;
        case 3: // 进行 TCP FIN 扫描
            cout << "Begin TCP FIN scan..." << endl;
            TCPFinParam.HostIP = HostIP;
            TCPFinParam.BeginPort = BeginPort;
            TCPFinParam.EndPort = EndPort;
            TCPFinParam.LocalHostIP = LocalHostIP;
            ret = pthread_create(&ThreadID, NULL, Thread_TCPFinScan, &TCPFinParam);
            if (ret == -1)
            {
                cout << "Can't create the TCP FIN scan thread !" << endl;
                return -1;
            }

            ret = pthread_join(ThreadID, NULL);
            if (ret != 0)
            {
                cout << "call pthread_join function failed !" << endl;
                return -1;
            }
            else
            {
                cout << "TCP FIN Scan finished !" << endl;
                return 0;
            }
            break;
        case 4: // 进行 UDP 扫描
            cout << "Begin UDP scan..." << endl;
            // create thread for UDP scan
            UDPParam.HostIP = HostIP;
            UDPParam.BeginPort = BeginPort;
            UDPParam.EndPort = EndPort;
            UDPParam.LocalHostIP = LocalHostIP;
            ret = pthread_create(&ThreadID, NULL, Thread_UDPScan, &UDPParam);
            if (ret == -1)
            {
                cout << "Can't create the UDP scan thread !" << endl;
                return -1;
            }

            ret = pthread_join(ThreadID, NULL);
            if (ret != 0)
            {
                cout << "call pthread_join function failed !" << endl;
                return -1;
            }
            else
            {
                cout << "UDP Scan finished !" << endl;
                return 0;
            }
            break;
        default:
            break;
        }
    }

    return 0;
}

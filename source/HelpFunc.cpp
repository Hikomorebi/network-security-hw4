#include "HelpFunc.h"

unsigned short in_cksum(unsigned short *ptr, int nbytes)
{
    register long sum;
    u_short oddbyte;
    register u_short answer;

    sum = 0;
    while (nbytes > 1)
    {
        sum += *ptr++;
        nbytes -= 2;
    }

    if (nbytes == 1)
    {
        oddbyte = 0;
        *((u_char *)&oddbyte) = *(u_char *)ptr;
        sum += oddbyte;
    }

    sum = (sum >> 16) + (sum & 0xffff);
    sum += (sum >> 16);
    answer = ~sum;

    return (answer);
}

bool IsPortOK(unsigned bPort, unsigned ePort)
{
    if (bPort > ePort)
        return false;
    else
    {
        if (bPort < 1 || bPort > 65535 || ePort < 1 || ePort > 65535)
            return false;
        else
            return true;
    }
}

unsigned int GetLocalHostIP(void)
{
    FILE *fd;
    char buf[20] = {0x00};

    fd = popen("/sbin/ifconfig | grep inet | grep -v 127 | awk '{print $2}' | cut -d \":\" -f 2", "r");
    if (fd == NULL)
    {
        fprintf(stderr, "cannot get source ip -> use the -f option\n");
        exit(-1);
    }
    fscanf(fd, "%20s", buf);
    return (inet_addr(buf));
}

bool Ping(string HostIP, unsigned LocalHostIP)
{
    string SrcIP, DstIP, LocalIP;
    int PingSock, on, ret, Addrlen;
    unsigned short LocalPort;
    struct sockaddr_in PingHostAddr, FromAddr;
    struct in_addr in_LocalhostIP;
    char *SendBuf;
    char RecvBuf[1024];
    unsigned short SendBufSize;

    struct iphdr *ip;
    struct icmphdr *icmp;
    struct ip *Recvip;
    struct icmp *Recvicmp;
    struct timeval *tp;

    struct timeval TpStart, TpEnd;
    float TimeUse;

    bool flags;

    PingSock = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
    if (PingSock < 0)
    {
        cout << "Creat Ping socket failed !" << endl;
        return false;
    }
    LocalPort = 10086;

    on = 1;
    ret = setsockopt(PingSock, 0, IP_HDRINCL, &on, sizeof(on));
    if (ret < 0)
    {
        cout << "Bind Ping socket failed !" << endl;
        return false;
    }

    SendBufSize = sizeof(struct iphdr) + sizeof(struct icmphdr) + sizeof(struct timeval);
    SendBuf = (char *)malloc(SendBufSize);

    memset(SendBuf, 0, sizeof(SendBuf));

    ip = (struct iphdr *)SendBuf;
    ip->ihl = 5;
    ip->version = 4;
    ip->tos = 0;
    ip->tot_len = htons(SendBufSize);
    ip->id = rand();
    ip->ttl = 64;
    ip->frag_off = 0x40;
    ip->protocol = IPPROTO_ICMP;
    ip->check = 0;
    ip->saddr = LocalHostIP;
    ip->daddr = inet_addr(&HostIP[0]);

    icmp = (struct icmphdr *)(ip + 1);
    icmp->type = ICMP_ECHO;
    icmp->code = 0;
    icmp->un.echo.id = htons(LocalPort);
    icmp->un.echo.sequence = 0;

    tp = (struct timeval *)&SendBuf[28];
    gettimeofday(tp, NULL);

    icmp->checksum = in_cksum((u_short *)icmp, sizeof(struct icmphdr) + sizeof(struct timeval));

    PingHostAddr.sin_family = AF_INET;
    PingHostAddr.sin_addr.s_addr = inet_addr(&HostIP[0]);
    Addrlen = sizeof(struct sockaddr_in);

    ret = sendto(PingSock, SendBuf, SendBufSize, 0, (struct sockaddr *)&PingHostAddr, sizeof(PingHostAddr));
    if (ret < 0)
    {
        cout << "Send ping packet failed !" << endl;
        return false;
    }

    if (fcntl(PingSock, F_SETFL, O_NONBLOCK) == -1)
    {
        cout << "Set socket in non-blocked model fail !" << endl;
        return false;
    }

    gettimeofday(&TpStart, NULL);
    flags = false;
    do
    {

        ret = recvfrom(PingSock, RecvBuf, 1024, 0, (struct sockaddr *)&FromAddr, (socklen_t *)&Addrlen);
        if (ret > 0)
        {
            Recvip = (struct ip *)RecvBuf;
            Recvicmp = (struct icmp *)(RecvBuf + (Recvip->ip_hl * 4));

            SrcIP = inet_ntoa(Recvip->ip_src);
            DstIP = inet_ntoa(Recvip->ip_dst);

            in_LocalhostIP.s_addr = LocalHostIP;
            LocalIP = inet_ntoa(in_LocalhostIP);

            if (SrcIP == HostIP && DstIP == LocalIP && Recvicmp->icmp_type == ICMP_ECHOREPLY)
            {
                cout << "Ping Host " << HostIP << " Successfully !" << endl;
                flags = true;
                break;
            }
        }
        gettimeofday(&TpEnd, NULL);
        TimeUse = (1000000 * (TpEnd.tv_sec - TpStart.tv_sec) + (TpEnd.tv_usec - TpStart.tv_usec)) / 1000000.0;
        if (TimeUse < 3)
        {
            continue;
        }
        else
        {
            flags = false;
            break;
        }

    } while (true);

    return flags;
}
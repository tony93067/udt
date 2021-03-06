#ifndef WIN32
   #include <unistd.h>
   #include <cstdlib>
   #include <cstring>
   #include <netdb.h>
#else
   #include <winsock2.h>
   #include <ws2tcpip.h>
   #include <wspiapi.h>
#endif
#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <unistd.h>
#include <udt.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/times.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/mman.h>


#include "cc.h"


using namespace std;

void* handle_client(void*);

// define control data
#define START_TRANS "start"
#define END_TRANS "end"

// define UNITS
#define UNITS_G 1000000000
#define UNITS_M 1000000
#define UNITS_K 1000
#define UNITS_BYTE_TO_BITS 8

// define packet size
#define NUM_PACKET_LENGTH 1000

// define DEFAULT_PORT
#define CONTROL_DEFAULT_PORT "9000"
#define DATA_DEFAULT_PORT "8999"

// define DEFAULT_EXECUTE_TIME
#define DEFAULT_EXECUTE_TIME 1

// define SERVER_IP
#define SERVER_IP "140.117.171.182"
/* gloabal variables */;
int output_interval = 5000000; // 5sec

// compute execution time
clock_t old_time, new_time;
struct tms time_start,time_end;//use for count executing time
double ticks;
// number of clients
int total_number_clients = 0;
int seq_client = 1;
// number of packets
int num_packets = 0;
// port array for data_socket
string *port_data_socket;
int port_seq = 0;
int MSS = 0;
float ttl_s = 0.0f; // second
int execute_time_sec = DEFAULT_EXECUTE_TIME;
int num_client = 0;
// used to get mmap return address
void* file_addr;
#ifndef WIN32
void* monitor(void*);
#else
DWORD WINAPI monitor(LPVOID);
#endif

int main(int argc, char* argv[])
{
    
   /*if ((1 != argc) && (((2 != argc) && (3 != argc) && (4 != argc) && (5 != argc) && (6 != argc)) || (0 == atoi(argv[1]))))
   {
      cout << "usage: ./udtserver [server_port] [execute_time(sec)] [num_client] [output_interval(sec)] [ttl(msec)]" << endl;
      return 0;
   }*/
   if ((1 != argc) && (((2 != argc) && (3 != argc) && (4 != argc) && (5 != argc) && (6 != argc)) || (0 == atoi(argv[1]))))
   {
      cout << "usage: ./udtserver [server_port] [MSS] [num_client]" << endl;
      return 0;
   }

   // use this function to initialize the UDT library
   UDT::startup();

   addrinfo hints;
   addrinfo* res;

   memset(&hints, 0, sizeof(struct addrinfo));

   hints.ai_flags = AI_PASSIVE;
   hints.ai_family = PF_INET;
   hints.ai_socktype = SOCK_STREAM;
   //hints.ai_socktype = SOCK_DGRAM;
    
   string service_control(CONTROL_DEFAULT_PORT);
  
    if(4 == argc)
    {
        MSS = atoi(argv[2]);
        cout << "Setting Parameter :" << endl;
        cout << "MSS : " << MSS << endl;
        num_client = atoi(argv[3]);
        // port_data_socket = (string*)malloc(num_client * sizeof(string));
        port_data_socket = new string[num_client];
        // create port
        int tmp_port = 5100;
        char tmp_port_char[10];
        for(int j = 0; j < num_client; j++)
        {
            sprintf(tmp_port_char, "%d", tmp_port);
            port_data_socket[j] = tmp_port_char;
            tmp_port++;
        }


    // decide service_port
    service_control = argv[1];

    cout << "port: " << argv[1] << ", MSS size: " << argv[2] << endl;
   }

   if (0 != getaddrinfo(SERVER_IP, service_control.c_str(), &hints, &res))
   {
      cout << "illegal port number or port is busy.\n" << endl;
      return 0;
   }
     
   // exchange control packet
   UDTSOCKET serv = UDT::socket(res->ai_family, res->ai_socktype, res->ai_protocol);

   // UDT Options
   //UDT::setsockopt(serv, 0, UDT_CC, new CCCFactory<CUDPBlast>, sizeof(CCCFactory<CUDPBlast>));
   //UDT::setsockopt(serv, 0, UDT_MSS, new int(9000), sizeof(int));
   //UDT::setsockopt(serv, 0, UDT_RCVBUF, new int(10000000), sizeof(int));
   //UDT::setsockopt(serv, 0, UDP_RCVBUF, new int(10000000), sizeof(int));
   UDT::setsockopt(serv, 0, UDT_REUSEADDR, new bool(false), sizeof(bool));

   if (UDT::ERROR == UDT::bind(serv, res->ai_addr, res->ai_addrlen))
   {
      cout << "bind: " << UDT::getlasterror().getErrorMessage() << endl;
      return 0;
   }

   freeaddrinfo(res);

   cout << "server is ready at port: " << service_control << endl;

   if (UDT::ERROR == UDT::listen(serv, num_client))
   {
      cout << "listen: " << UDT::getlasterror().getErrorMessage() << endl;
      return 0;
   }

   sockaddr_storage clientaddr;
   int addrlen = sizeof(clientaddr);

   UDTSOCKET recver;

   while (true)
   {
      if (UDT::INVALID_SOCK == (recver = UDT::accept(serv, (sockaddr*)&clientaddr, &addrlen)))
      {
         cout << "accept: " << UDT::getlasterror().getErrorMessage() << endl;
         return 0;
      };

      char clienthost[NI_MAXHOST];
      char clientservice[NI_MAXSERV];
      getnameinfo((sockaddr *)&clientaddr, addrlen, clienthost, sizeof(clienthost), clientservice, sizeof(clientservice), NI_NUMERICHOST|NI_NUMERICSERV);
      cout << "\n\nnew connection: " << clienthost << ":" << clientservice << endl;
      // create thread to handle clients
      pthread_t p1;
      if(pthread_create(&p1, NULL, handle_client, new UDTSOCKET(recver)) != 0)
      {
        cout << "pthread_create error!!" << endl;
        exit(1);
      }
   }

   UDT::close(recver);
   UDT::close(serv);

   return 1;
}

void *handle_client(void *arg)
{
    UDTSOCKET recver = *((UDTSOCKET *)arg);
    // packets
    int total_send_packets = 0;
    int total_send_size = 0;
    //int sec = 0;
    //int interval = 0;
    
    total_number_clients++;
    
    cout << "total_number_clients: " << total_number_clients << endl;
   
    int rs;
    char control_data[sizeof(START_TRANS)];

    // receive control msg (SOCK_STREAM)
    if (UDT::ERROR == (rs = UDT::recv(recver, control_data, sizeof(control_data), 0)))
    {
        cout << "recv:" << UDT::getlasterror().getErrorMessage() << endl;
        exit(1);
    }

    // if control_data is START_TRANS
    if(strcmp(control_data, START_TRANS) == 0) 
    {
        cout << "received control data: START_TRANS" << endl;
    }

    int ss = 0;

    // send seq number to client
    char seq_client_char[NUM_PACKET_LENGTH];

    // convert int into string
    sprintf(seq_client_char,"%d", seq_client); 
    cout << "Client Seq: " << seq_client_char << endl;
    if(UDT::ERROR == (ss = UDT::send(recver, seq_client_char, sizeof(seq_client_char), 0)))
    {
        cout << "send:" << UDT::getlasterror().getErrorMessage() << endl;
        exit(1); 
    }
    seq_client++;

    // send port of data socket to client 
    port_seq %= num_client;
    //cout << "elements in port_data_socket: " << sizeof(port_data_socket) << endl;
    if(UDT::ERROR == (ss = UDT::send(recver, (char *)port_data_socket[port_seq].c_str(), sizeof(port_data_socket[port_seq].c_str()), 0)))
    {
        cout << "send:" << UDT::getlasterror().getErrorMessage() << endl;
        exit(1); 
    }
  
    string service_data(DATA_DEFAULT_PORT);
    if(ss > 0)
    {
        cout << "port: " << (char *)port_data_socket[port_seq].c_str() << ", sizeof(port[port_seq]): " << sizeof(port_data_socket[port_seq].c_str()) << endl << endl;
        service_data = port_data_socket[port_seq]; 
        port_seq++;
    }

	//cout << "service_data " << service_data.c_str() << endl;
	fflush(stdout);
    /* create data tranfer socket(using partial reliable message mode) */
    addrinfo hints;
    addrinfo* res;
   
    // reset hints
    memset(&hints, 0, sizeof(struct addrinfo));

    hints.ai_flags = AI_PASSIVE;
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
 
    if (0 != getaddrinfo(SERVER_IP, service_data.c_str(), &hints, &res))
    {
        cout << "illegal port number or port is busy.\n" << endl;
        exit(1);
    }
    // exchange data packet
    UDTSOCKET serv_data = UDT::socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    // UDT Options
    //UDT::setsockopt(serv, 0, UDT_CC, new CCCFactory<CUDPBlast>, sizeof(CCCFactory<CUDPBlast>));
    if(UDT::ERROR == UDT::setsockopt(serv_data, 0, UDT_MSS, new int(MSS), sizeof(int)))
    {
        cout << "set UDT MSS error" << endl;
        exit(1);
    }else
    {
        cout << "set MSS : " << MSS << endl;
    }
    //UDT::setsockopt(serv, 0, UDT_RCVBUF, new int(10000000), sizeof(int));
    //UDT::setsockopt(serv, 0, UDP_RCVBUF, new int(10000000), sizeof(int));
    //UDT::setsockopt(serv_data, 0, UDT_REUSEADDR, new bool(false), sizeof(bool));
    
    int sndbuf = 0;
    int oplen = sizeof(int);
    if (UDT::ERROR == UDT::getsockopt(serv_data, 0, UDT_SNDBUF, (char*)&sndbuf, &oplen))
    {
        cout << "getsockopt error" << endl;
    }else
    {
        cout << "UDT Send Buffer size : " << sndbuf << endl;
    }
    sndbuf = 0;
    if (UDT::ERROR == UDT::getsockopt(serv_data, 0, UDT_RCVBUF, (char*)&sndbuf, &oplen))
    {
        cout << "getsockopt error" << endl;
    }else
    {
        cout << "UDT RECV Buffer size : " << sndbuf << endl;
    }
    sndbuf = 0;
    if (UDT::ERROR == UDT::getsockopt(serv_data, 0, UDP_SNDBUF, (char*)&sndbuf, &oplen))
    {
        cout << "getsockopt error" << endl;
    }else
    {
        cout << "UDP Send Buffer size : " << sndbuf << endl;
    }
    sndbuf = 0;
    if (UDT::ERROR == UDT::getsockopt(serv_data, 0, UDP_RCVBUF, (char*)&sndbuf, &oplen))
    {
        cout << "getsockopt error" << endl;
    }else
    {
        cout << "UDP RECV Buffer size : " << sndbuf << endl;
    }
    if (UDT::ERROR == UDT::bind(serv_data, res->ai_addr, res->ai_addrlen))
    {
        cout << "bind(serv_data): " << UDT::getlasterror().getErrorMessage() << endl;
        exit(1);
    }
        
    freeaddrinfo(res);

    if (UDT::ERROR == UDT::listen(serv_data, num_client))
    {
        cout << "listen(serv_data): " << UDT::getlasterror().getErrorMessage() << endl;
        exit(1);
    }

    sockaddr_storage clientaddr2;
    int addrlen2 = sizeof(clientaddr2);

    UDTSOCKET recver2;
    
    if (UDT::INVALID_SOCK == (recver2 = UDT::accept(serv_data, (sockaddr*)&clientaddr2, &addrlen2)))
    {
        cout << "accept(serv_data): " << UDT::getlasterror().getErrorMessage() << endl;
        exit(1);
    }
    int i = 0;
    int fd;
    int ssize = 0;
    char send_buf[MSS];
    struct stat sb;
    fd = open("/home/tony/??????code/??????code/file.txt", O_RDONLY, S_IRWXU);
    // get file info
    if(fstat(fd, &sb) == -1)
    {
        printf("fstat error\n");
        exit(1);
    }
    // map file to memory
    file_addr = mmap(NULL, sb.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
    if(file_addr == MAP_FAILED)
    {
        printf("mmap error\n");
        exit(1);
    }
    int transmit_size = 0;
    transmit_size = sb.st_size;
    ssize = 0;
    while(1)
    {
        // clear temporary buffer
        memset(send_buf, '\0', MSS);
        // record start time when receive first data packet
        if(i == 0)
        {
            //start time
            if((old_time = times(&time_start)) == -1)
            {
                printf("time error\n");
                exit(1);
            }
            pthread_create(new pthread_t, NULL, monitor, &recver2);
        }
        
        //Resent:
        if(ssize < sb.st_size)
        {
            if(UDT::ERROR == (ss = UDT::send(recver2, (char *)file_addr + ssize, transmit_size, 0))) 
            {
                cout << "send:" << UDT::getlasterror().getErrorMessage() << endl;
                exit(1);
            }
            else
            {
                //cout << "send size : " << ss << endl;
                ssize += ss;
                //cout << "ssize : " << ssize << endl;
                transmit_size -= ss;
                //if((sb.st_size - ssize) < MSS)
                //    MSS = sb.st_size -ssize;
            }
        }
        if(ssize == sb.st_size)
            break;
        total_send_size = ssize;    
        i++;
    }
    // close file
    close(fd);
    //finish time
    if((new_time = times(&time_end)) == -1)
    {
        printf("time error\n");
        exit(1);
    }

    printf("\n[Result]:\n");
    cout << "Client Seq: " << seq_client_char << endl;
    //executing time
    ticks=sysconf(_SC_CLK_TCK);
    double execute_time = (new_time - old_time)/ticks;
    printf("Execute Time: %2.2f\n", execute_time);
    cout << "Total Send Packets: " << total_send_packets << endl;
    cout << "Total Send Size: " << total_send_size << endl;
    
    double send_rate_bytes = total_send_size / execute_time;
    cout << "send_rate_bytes " << send_rate_bytes << endl;
    if(send_rate_bytes >= UNITS_G)
    {
        send_rate_bytes /= UNITS_G;
        printf("UDT Sending Rate: %2.2f (GBytes/s)\n", send_rate_bytes);
        //printf("UDT Sending Rate: %2.2f (GBytes/s)\n\n", send_rate_bytes / UNITS_BYTE_TO_BITS);
    }
    else if(send_rate_bytes >= UNITS_M)
    {
        send_rate_bytes /= UNITS_M;
        printf("UDT Sending Rate: %2.2f (MBytes/s)\n", send_rate_bytes);
        //printf("UDT Sending Rate: %2.2f (MBytes/s)\n\n", send_rate_bytes / UNITS_BYTE_TO_BITS);
    }
    else if(send_rate_bytes >= UNITS_K)
    {
        send_rate_bytes /= UNITS_K;
        printf("UDT Sending Rate: %2.2f (KBytes/s)\n", send_rate_bytes);
        //printf("UDT Sending Rate: %2.2f (KBytes/s)\n\n", send_rate_bytes / UNITS_BYTE_TO_BITS);
    }
    else
    {
        printf("UDT Sending Rate: %2.2f (Bytes/s)\n", send_rate_bytes);
        //printf("UDT Sending Rate: %2.2f (Bytes/s)\n\n", send_rate_bytes / UNITS_BYTE_TO_BITS);
    }

    
    // receive END_TRANS packet's ACK from client, then close all connection
    char control_data3[sizeof(END_TRANS)]; 
    rs = 0;
    
    printf("wait END_TRANS(Client Seq: %s)\n", seq_client_char);
    if (UDT::ERROR == (rs = UDT::recv(recver, control_data3, sizeof(control_data3), 0)))
    {
        cout << "recv:" << UDT::getlasterror().getErrorMessage() << endl;
        exit(1);
    } 
    printf("finish waiting END_TRANS(Client Seq: %s)\n", seq_client_char);
    if((rs > 0) && (strcmp(control_data3,END_TRANS) == 0))
    {
        total_number_clients--;
        cout << "number of clients: " << total_number_clients << endl;
        printf("get END_TRANS(Client Seq: %s)\n", seq_client_char);
        
        UDT::close(serv_data);
        UDT::close(recver2);
    }

    return NULL;
}

#ifndef WIN32
void* monitor(void* s)
#else
DWORD WINAPI monitor(LPVOID s)
#endif
{
    UDTSOCKET u = *(UDTSOCKET*)s;

    UDT::TRACEINFO perf;
    int monitor_fd;
    char str[100];
    
    // record monitor data
    monitor_fd = open("monitor.txt", O_RDWR | O_CREAT | O_APPEND, S_IRWXU);
    memset(str, '\0', 100);
    strcpy(str, "SendRate(Mb/s)\tRTT(ms)\tCWnd\tPktSndPeriod(us)\tRecvACK\tRecvNAK\n");
	if(write(monitor_fd, str, strlen(str)) < 0)
    {
        cout << "write error" << endl;
        exit(1);
    }
    cout << "SendRate(Mb/s)\tRTT(ms)\tCWnd\tPktSndPeriod(us)\tRecvACK\tRecvNAK" << endl;
    while (true)
    {
        memset(str, '\0', 100);
        #ifndef WIN32
            sleep(1);
        #else
            Sleep(1000);
        #endif
            
        if (UDT::ERROR == UDT::perfmon(u, &perf))
        {
            cout << "perfmon: " << UDT::getlasterror().getErrorMessage() << endl;
            break;
        }
        sprintf(str, "%f\t\t%f\t%d\t%f\t\t\t%d\t%d\n", perf.mbpsSendRate, 
            perf.msRTT, perf.pktCongestionWindow, perf.usPktSndPeriod, perf.pktRecvACK, perf.pktRecvNAK);
        if(write(monitor_fd, str, strlen(str)) < 0)
        {
            cout << "write error" << endl;
            exit(1);
        }
        cout << perf.mbpsSendRate << "\t\t" 
            << perf.msRTT << "\t" 
            << perf.pktCongestionWindow << "\t" 
            << perf.usPktSndPeriod << "\t\t\t" 
            << perf.pktRecvACK << "\t" 
            << perf.pktRecvNAK << endl;
    }
    if(write(monitor_fd, "\n", sizeof("\n")) < 0)
    {
        cout << "write error" << endl;
        exit(1);
    }
    close(monitor_fd);
    #ifndef WIN32
        return NULL;
    #else
        return 0;
    #endif
}
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
#include <fstream>


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

// define buffer size
#define BUFFER_SIZE 10000

// define DEFAULT_PORT
#define CONTROL_DEFAULT_PORT "9000"
#define DATA_DEFAULT_PORT "8999"

// define DEFAULT_EXECUTE_TIME
#define DEFAULT_EXECUTE_TIME 1

// define SERVER_IP
#define SERVER_IP "140.117.171.182"

// compute execution time
clock_t old_time, new_time;
struct tms time_start,time_end;//use for count executing time
double ticks;

// number of clients
int total_number_clients = 0;
int seq_client = 1;

// Recive Buffer
char recv_buf[BUFFER_SIZE];

// port array for data_socket
string *port_data_socket;
int port_seq = 0;
int MSS = 0;
float ttl_s = 0.0f; // second
int execute_time_sec = DEFAULT_EXECUTE_TIME;
int num_client = 0;

// indicate congestion control method
int mode = 0;

// congestion control method
char method[15];

// used to get mmap return address
void* file_addr;

//temp variable
char* temp_MSS;
char* temp_BK_TCP;

// Background TCP Number
int background_TCP_number = 0;
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
   if (argc != 6)
   {
      cout << "usage: ./udtserver [server_port] [MSS] [num_client] [mode(1:UDT, 2:CTCP, 3:CUDPBlast)] [Background TCP Number]" << endl;
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
    
   string service_control(CONTROL_DEFAULT_PORT);
  
    if(6 == argc)
    {
        temp_MSS = argv[2];
        temp_BK_TCP = argv[5];
        MSS = atoi(argv[2]);
        mode = atoi(argv[4]);
        cout << "Setting Parameter :" << endl;
        cout << "MSS : " << MSS  << " Mode : " << mode << endl;
        num_client = atoi(argv[3]);
        background_TCP_number = atoi(argv[5]);

        port_data_socket = new string[num_client];
        // create port
        int tmp_port = 5100;
        char tmp_port_char[15] = {0};
        for(int j = 0; j < num_client; j++)
        {
            sprintf(tmp_port_char, "%d", tmp_port);
            port_data_socket[j] = tmp_port_char;
            tmp_port++;
        }

        memset(method, '\0', sizeof(method));
        if(mode == 1)
            strcpy(method, "UDT");
        else if(mode == 2)
            strcpy(method, "CTCP");
        else
            strcpy(method, "CUDPBlast");


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
    
    //int sec = 0;
    //int interval = 0;

    // use to get getsockopt return variable and value
    //int sndbuf = 0;
    //int oplen = sizeof(int);
    
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
    char seq_client_char[1000];

    // convert int into string, seq_client from 1
    sprintf(seq_client_char,"%d", seq_client); 
    cout << "Client Seq: " << seq_client_char << endl;
    if(UDT::ERROR == (ss = UDT::send(recver, seq_client_char, sizeof(seq_client_char), 0)))
    {
        cout << "send:" << UDT::getlasterror().getErrorMessage() << endl;
        exit(1); 
    }
    seq_client++;

    // send port number to client 
    port_seq %= num_client;
    if(UDT::ERROR == (ss = UDT::send(recver, (char *)port_data_socket[port_seq].c_str(), sizeof(port_data_socket[port_seq].c_str()), 0)))
    {
        cout << "send:" << UDT::getlasterror().getErrorMessage() << endl;
        exit(1); 
    }
  
    string service_data(DATA_DEFAULT_PORT);
    if(ss > 0)
    {
        cout << "port: " << (char *)port_data_socket[port_seq].c_str() << endl;
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

    //----------------------------------------------------------------------------------------
    // exchange data packet
    UDTSOCKET server_data = UDT::socket(res->ai_family, res->ai_socktype, res->ai_protocol);


    // UDT Options
    if(mode == 1)
    {
        cout << "Using default Congestion Control Method UDT" << endl;

    }else if(mode == 2)
    {
        UDT::setsockopt(server_data, 0, UDT_CC, new CCCFactory<CTCP>, sizeof(CCCFactory<CTCP>));
        cout << "Setting Congestion Control Method CTCP" << endl;
    }else if(mode == 3)
    {
        UDT::setsockopt(server_data, 0, UDT_CC, new CCCFactory<CUDPBlast>, sizeof(CCCFactory<CUDPBlast>));
        cout << "Setting Congestion Control Method CUDPBlast" << endl;
    }
    if(UDT::ERROR == UDT::setsockopt(server_data, 0, UDT_MSS, new int(MSS), sizeof(int)))
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
    


    // set receive timeout 5 second
    /*
    if (UDT::ERROR == UDT::setsockopt(server_data, 0, UDT_RCVTIMEO, new int(5000), sizeof(int)))
    {
        cout << "set RCV timeout error" << endl;
    }else
    {
        UDT::getsockopt(server_data, 0, UDT_RCVTIMEO, (char *)&sndbuf, &oplen);
        cout << "Set RCV timeout : "<< sndbuf << endl;
    }*/
    
    /*if (UDT::ERROR == UDT::getsockopt(serv_data, 0, UDT_SNDBUF, (char*)&sndbuf, &oplen))
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
    */

    // bind
    if (UDT::ERROR == UDT::bind(server_data, res->ai_addr, res->ai_addrlen))
    {
        cout << "bind(server_data): " << UDT::getlasterror().getErrorMessage() << endl;
        exit(1);
    }
    freeaddrinfo(res);

    // listen
    if (UDT::ERROR == UDT::listen(server_data, num_client))
    {
        cout << "listen(server_data): " << UDT::getlasterror().getErrorMessage() << endl;
        exit(1);
    }

    // accept
    sockaddr_storage clientaddr2;
    int addrlen2 = sizeof(clientaddr2);

    UDTSOCKET client_data;
    
    if (UDT::INVALID_SOCK == (client_data = UDT::accept(server_data, (sockaddr*)&clientaddr2, &addrlen2)))
    {
        cout << "accept(server_data): " << UDT::getlasterror().getErrorMessage() << endl;
        exit(1);
    }
    // get the file size
    memset(recv_buf, '\0',sizeof(recv_buf));
    if(UDT::ERROR == UDT::recv(client_data, (char *)recv_buf, sizeof(recv_buf), 0))
    {
        cout << "recv:" << UDT::getlasterror().getErrorMessage() << endl;
        //cout << rsize << endl;
    }
    int file_size = atoi(recv_buf);
    //-------------------------------------------------------------------------------------------
    // Data Receiving
    int rsize = 0;
    int j = 0; // used to set start time
    int fd; // use to open file
    int total_recv_size = 0; // record receiver data size
    fd = open("file.txt", O_RDWR | O_CREAT | O_TRUNC, S_IRWXU);
    while(1)
    {
        // reset recv_buf.data
        memset(recv_buf, '\0',sizeof(recv_buf));
        // 未收到資料時回傳 -1
        if(UDT::ERROR == (rsize = UDT::recv(client_data, (char *)recv_buf, sizeof(recv_buf), 0))) 
        {
            cout << "recv:" << UDT::getlasterror().getErrorMessage() << endl;
            //cout << rsize << endl;
        }
        else
        {
            //cout << "rsize : " << rsize << endl;
            // record start time when receive first data packet
            if(j == 0)
            {
                // record start time
                if((old_time = times(&time_start)) == -1)
                {
                    cout << "time error" << endl;
                    exit(1);
                }
                // create thread to monitor socket(client_data)
                pthread_create(new pthread_t, NULL, monitor, &client_data);
            }
            
            if(write(fd, recv_buf, rsize) == -1)
            {
                cout << "write error" << endl;
                exit(1);
            }
            total_recv_size += rsize;
            //cout << "total_recv_size " << total_recv_size << endl;
        }
        if(total_recv_size == file_size)
        {
            // 接收完成, 關閉檔案
            cout << "receiving finish & close file" << endl;
            break;
        }
        j++;

    }
    // close file
    close(fd);
    //finish time
    if((new_time = times(&time_end)) == -1)
    {
        printf("time error\n");
        exit(1);
    }
    memset(recv_buf, '\0', sizeof(recv_buf));
    strcpy(recv_buf, "END");

    // let sender know receiver receving finish
    if(UDT::ERROR == UDT::send(client_data, (char *)recv_buf, sizeof(recv_buf), 0))
    {
        cout << "recv:" << UDT::getlasterror().getErrorMessage() << endl;
    }
    printf("\n[Result]:\n");
    cout << "Client Seq: " << seq_client_char << endl;

    ticks = sysconf(_SC_CLK_TCK);
    double execute_time = (new_time - old_time)/ticks;
    printf("Execute Time: %2.2f\n", execute_time);
    cout << "Total Receive Size: " << total_recv_size << endl;
    
    double receive_rate_bytes = total_recv_size / execute_time;
    cout << "receive_rate_bytes " << receive_rate_bytes << endl;

    if(receive_rate_bytes >= UNITS_G)
    {
        receive_rate_bytes /= UNITS_G;
        printf("UDT Receiving Rate: %2.2f (GBytes/s)\n", receive_rate_bytes);
        //printf("UDT Sending Rate: %2.2f (GBytes/s)\n\n", send_rate_bytes / UNITS_BYTE_TO_BITS);
    }
    else if(receive_rate_bytes >= UNITS_M)
    {
        receive_rate_bytes /= UNITS_M;
        printf("UDT Receiving Rate: %2.2f (MBytes/s)\n", receive_rate_bytes);
        //printf("UDT Sending Rate: %2.2f (MBytes/s)\n\n", send_rate_bytes / UNITS_BYTE_TO_BITS);
    }
    else if(receive_rate_bytes >= UNITS_K)
    {
        receive_rate_bytes /= UNITS_K;
        printf("UDT Receiving Rate: %2.2f (KBytes/s)\n", receive_rate_bytes);
        //printf("UDT Sending Rate: %2.2f (KBytes/s)\n\n", send_rate_bytes / UNITS_BYTE_TO_BITS);
    }
    else
    {
        printf("UDT Receiving Rate: %2.2f (Bytes/s)\n", receive_rate_bytes);
        //printf("UDT Sending Rate: %2.2f (Bytes/s)\n\n", send_rate_bytes / UNITS_BYTE_TO_BITS);
    }
    fstream fout;
    char result_addr[50] = {0};
    if(mode == 1)
    {
        strcat(result_addr, "UDT_Recv_Result_");
        strcat(result_addr, "MSS");
        strcat(result_addr, temp_MSS);
        strcat(result_addr, "_TCP");
        strcat(result_addr, temp_BK_TCP);
        strcat(result_addr, ".csv");
        fout.open(result_addr, ios::out|ios::app);
    }
    else if(mode ==2)
    {
        strcat(result_addr, "CTCP_Recv_Result_");
        strcat(result_addr, "MSS");
        strcat(result_addr, temp_MSS);
        strcat(result_addr, "_TCP");
        strcat(result_addr, temp_BK_TCP);
        strcat(result_addr, ".csv");
        fout.open(result_addr, ios::out|ios::app);
    }
    else if(mode == 3)
    {
        strcat(result_addr, "CUDPBlast_Recv_Result_");
        strcat(result_addr, "MSS");
        strcat(result_addr, temp_MSS);
        strcat(result_addr, "_TCP");
        strcat(result_addr, temp_BK_TCP);
        strcat(result_addr, ".csv");
        fout.open(result_addr, ios::out|ios::app);
    }
   
     // record result
    fout << endl << endl;
    fout << "Method," << method << endl;
    fout << "BK TCP Number," << background_TCP_number << endl;
    fout << "MSS," << MSS << endl;
    fout << "執行時間," << execute_time << endl;
    fout << endl << endl;
    fout.close();
    // receive END_TRANS packet's ACK from client, then close all connection
    /*char control_data3[sizeof(END_TRANS)]; 
    rs = 0;
    
    printf("wait END_TRANS(Client Seq: %s)\n", seq_client_char);
    if (UDT::ERROR == (rs = UDT::recv(recver, control_data3, sizeof(control_data3), 0)))
    {
        cout << "recv:" << UDT::getlasterror().getErrorMessage() << endl;
        exit(1);
    } 
    printf("finish waiting END_TRANS(Client Seq: %s)\n", seq_client_char);
    */
    //if((rs > 0) && (strcmp(control_data3,END_TRANS) == 0))
    //{
    //    total_number_clients--;
    //    cout << "number of clients: " << total_number_clients << endl;
    //    printf("get END_TRANS(Client Seq: %s)\n", seq_client_char);

    //}

    return NULL;
}

#ifndef WIN32
void* monitor(void* s)
#else
DWORD WINAPI monitor(LPVOID s)
#endif
{
    UDTSOCKET u = *(UDTSOCKET*)s;
    char result_addr[50] = {0};
    fstream fout;
    int zero_times = 0;
    UDT::TRACEINFO perf;
    if(mode == 1)
    {
        strcat(result_addr, "Receiver_UDT_Monitor_");
        strcat(result_addr, "MSS");
        strcat(result_addr, temp_MSS);
        strcat(result_addr, "_TCP");
        strcat(result_addr, temp_BK_TCP);
        strcat(result_addr, ".csv");
        fout.open(result_addr, ios::out|ios::app);
    }
    else if(mode == 2)
    {
        strcat(result_addr, "Receiver_CTCP_Monitor_");
        strcat(result_addr, "MSS");
        strcat(result_addr, temp_MSS);
        strcat(result_addr, "_TCP");
        strcat(result_addr, temp_BK_TCP);
        strcat(result_addr, ".csv");
        fout.open(result_addr, ios::out|ios::app);
    }
    else if (mode == 3)
    {
        strcat(result_addr, "Receiver_CUDPBlast_Monitor_");
        strcat(result_addr, "MSS");
        strcat(result_addr, temp_MSS);
        strcat(result_addr, "_TCP");
        strcat(result_addr, temp_BK_TCP);
        strcat(result_addr, ".csv");
        fout.open(result_addr, ios::out|ios::app);
    }
    fout << endl << endl;
    fout << "Method," << method << endl;
    fout << "BK TCP Number," << background_TCP_number << endl;
    // record monitor data
    //monitor_fd = open("monitor.txt", O_RDWR | O_CREAT | O_APPEND, S_IRWXU);
    //sprintf(str, "MSS : %d\nSendRate(Mb/s)\tRTT(ms)\tCWnd\tPktSndPeriod(us)\tRecvACK\tRecvNAK\n", MSS);
    fout << "MSS," << MSS << endl;
    fout << "SendRate(Mb/s)," << "ReceiveRate(Mb/s)," << "Recv Loss," <<"RTT(ms)," << "CWnd," << "FlowWindow," << "PktSndPeriod(us)," << "RecvACK," << "RecvNAK," << "EstimatedBandwidth(Mb/s)" << endl;
    //cout << "SendRate(Mb/s)\tRTT(ms)\tCWnd\tPktSndPeriod(us)\tRecvACK\tRecvNAK" << endl;
    while (true)
    {
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
        fout << perf.mbpsSendRate << "," << perf.mbpsRecvRate << "," << perf.pktRcvLoss << "," << perf.msRTT << "," << perf.pktCongestionWindow << ","
            << perf.pktFlowWindow << "," << perf.usPktSndPeriod << "," << perf.pktRecvACK << "," << perf.pktRecvNAK << "," << perf.mbpsBandwidth << endl;
        if(perf.mbpsRecvRate == 0)
        {
            zero_times++;
        }else
        {
            zero_times = 0;
        }
        if(zero_times >= 5)
            break;
        /*cout << perf.mbpsSendRate << "\t\t" 
            << perf.msRTT << "\t" 
            << perf.pktCongestionWindow << "\t" 
            << perf.usPktSndPeriod << "\t\t\t" 
            << perf.pktRecvACK << "\t" 
            << perf.pktRecvNAK << endl;*/
    }
    fout << endl << endl;
    fout.close();
    #ifndef WIN32
        return NULL;
    #else
        return 0;
    #endif
}
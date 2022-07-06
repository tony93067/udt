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
#include <udt.h>
#include <sys/times.h>
#include "cc.h"


using namespace std;

#define DIE(x) perror(x),exit(1)

#ifndef WIN32
// void* recvdata(void*);
void* handle_client(void*);
void* udt_thread(void*);
void* tcp_thread(void*);
void *send_packet(void*);

#else
// DWORD WINAPI recvdata(LPVOID);
#endif

// define control data
#define START_TRANS "start"
#define END_TRANS "end"
// #define END_TRANS_ACK "end_ack"

// define number of packets
#define DEFAULT_NUM_PACKETS 40
// define UNITS
#define UNITS_G 1000000000
#define UNITS_M 1000000
#define UNITS_K 1000
#define UNITS_BYTE_TO_BITS 8

// define packet size
#define PACKET_SIZE 100
#define NUM_PACKET_LENGTH 1000

#define TCP_DEFAULT_PORT "8000"

// define DEFAULT_PORT
#define CONTROL_DEFAULT_PORT "9000"
#define DATA_DEFAULT_PORT "8999"

// define DEFAULT_TTL(3s)
#define DEFAULT_TTL 3000

// define DEFAULT_EXECUTE_TIME
#define DEFAULT_EXECUTE_TIME 1

/* gloabal variables */;
/*
struct monitor_arg {
  int seq_client;
  int ttl_ms;
  int total_send_pkt;
};
*/
struct buffer {
  int seq;
  char data[PACKET_SIZE];
};

/* argvs */
string service_data_port(DATA_DEFAULT_PORT);
string service_control(CONTROL_DEFAULT_PORT);
string tcp_service_port(TCP_DEFAULT_PORT);
int execute_time_sec = DEFAULT_EXECUTE_TIME;
int num_client = 0;
int ttl_ms = DEFAULT_TTL; // milli-second
int output_interval = 5000000; // 5sec

// bind tcp & udt start send point to be the same
bool is_udt_ready = false;
bool is_tcp_ready = false;

bool is_udt_finish = false;
bool is_tcp_finish = false;

// compute execution time
clock_t old_time_udt, new_time_udt, old_time_tcp, new_time_tcp;
double udt_execute_time;
double tcp_execute_time;
// struct tms time_start,time_end;//use for count executing time
double ticks;
// number of clients
int total_number_clients = 0;
// int seq_client = 1;
// number of packets
int num_packets = 0;
// port array for data_socket
// string port_data_socket[] = {"5666", "5667", "5668", "5669", "5700"};
string *port_data_socket;
int port_seq = 0;
float ttl_s = 0.0f; // second


int main(int argc, char* argv[])
{
   if ((1 != argc) && (((2 != argc) && (3 != argc) && (4 != argc) && (5 != argc) && (6 != argc) && (7 != argc) && (8 != argc)) || (0 == atoi(argv[1]))))
   {
      cout << "usage: ./udt_tcp_server [udt_control_port] [execute_time(sec)] [num_client] [output_interval(sec)] [ttl(msec)] [tcp_port] [udt_data_port]" << endl;
      return 0;
   }

   
//   string service_control(CONTROL_DEFAULT_PORT);

   // default => 40pkt/s
   num_packets = DEFAULT_NUM_PACKETS;
  if(8 == argc)
   {
     ttl_ms = atoi(argv[5]);
     cout << "ttl_ms(ms): " << ttl_ms << endl;
     output_interval = atoi(argv[4]) * UNITS_M; // 1us * 1Ms => 1s (because usleep use u as unit)
     cout << "output_interval: " << output_interval << endl;
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
       // printf("port: %s\n", port_data_socket[j].c_str()); 
     }

     // decide num_packets
     execute_time_sec = atoi(argv[2]);
     // E.g. 10min => 10 * 60 = 600s => 600 * 40(pkt/s) = 24000(pkt) / 10min
     num_packets *= execute_time_sec;

     // decide service_port
     service_control = argv[1];
     service_data_port = argv[7];
     tcp_service_port = argv[6];

     cout << "TCP port: " << tcp_service_port.c_str()  << endl;
     cout << "UDT Data Port: " << service_data_port.c_str() << endl;
     cout << "UDT Control port: " << service_control.c_str() << ", num_packets: " << num_packets << endl;
   }
   else if(7 == argc)
   {
     ttl_ms = atoi(argv[5]);
     cout << "ttl_ms(ms): " << ttl_ms << endl;
     output_interval = atoi(argv[4]) * UNITS_M; // 1us * 1Ms => 1s (because usleep use u as unit)
     cout << "output_interval: " << output_interval << endl;
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
       // printf("port: %s\n", port_data_socket[j].c_str()); 
     }

     // decide num_packets
     execute_time_sec = atoi(argv[2]);
     // E.g. 10min => 10 * 60 = 600s => 600 * 40(pkt/s) = 24000(pkt) / 10min
     num_packets *= execute_time_sec;

     // decide service_port
     service_control = argv[1];
     tcp_service_port = argv[6];

     cout << "TCP port: " << tcp_service_port.c_str()  << endl;
     cout << "UDT port: " << service_control.c_str() << ", num_packets: " << num_packets << endl;
   }
   else if(6 == argc)
   {
     ttl_ms = atoi(argv[5]);
     cout << "ttl_ms(ms): " << ttl_ms << endl;
     output_interval = atoi(argv[4]) * UNITS_M; // 1us * 1Ms => 1s (because usleep use u as unit)
     cout << "output_interval: " << output_interval << endl;
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
       // printf("port: %s\n", port_data_socket[j].c_str()); 
     }

     // decide num_packets
     execute_time_sec = atoi(argv[2]);
     // E.g. 10min => 10 * 60 = 600s => 600 * 40(pkt/s) = 24000(pkt) / 10min
     num_packets *= execute_time_sec;

     // decide service_port
     service_control = argv[1];

     cout << "UDT port: " << service_control.c_str() << ", num_packets: " << num_packets << endl;
   }
   else if(5 == argc)
   { 
     output_interval = atoi(argv[4]) * UNITS_M; // 1us * 1Ms => 1s (because usleep use u as unit)
     cout << "output_interval: " << output_interval << endl;
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
       // printf("port: %s\n", port_data_socket[j].c_str()); 
     }

     // decide num_packets
     execute_time_sec = atoi(argv[2]);
     // E.g. 10min => 10 * 60 = 600s => 600 * 40(pkt/s) = 24000(pkt) / 10min
     num_packets *= execute_time_sec;

     // decide service_port
     service_control = argv[1];

     cout << "port: " << service_control.c_str() << ", num_packets: " << num_packets << endl;
   }
   else if(4 == argc)
   {

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
       // printf("port: %s\n", port_data_socket[j].c_str()); 
     }

     // decide num_packets
     execute_time_sec = atoi(argv[2]);
     // E.g. 10min => 10 * 60 = 600s => 600 * 40(pkt/s) = 24000(pkt) / 10min
     num_packets *= execute_time_sec;

     // decide service_port
     service_control = argv[1];

     cout << "port: " << service_control.c_str() << ", num_packets: " << num_packets << endl;
   }
   else if(3 == argc)
   {
     // decide num_packets
     execute_time_sec = atoi(argv[2]);
     // E.g. 10min => 10 * 60 = 600s => 600 * 40(pkt/s) = 24000(pkt) / 10min
     num_packets *= execute_time_sec;

     // decide service_port
     service_control = argv[1];

     cout << "port: " << service_control.c_str() << ", num_packets: " << num_packets << endl;
   }
   else if (2 == argc)
   {
      // decide service_port
      service_control = argv[1];
      cout << "num_packets: " << num_packets << endl;
   }
  

   // create udt thread
   pthread_t udtthread;
   pthread_create(&udtthread, NULL, udt_thread, NULL);

   // create tcp thread
   pthread_t tcpthread;
   pthread_create(&tcpthread, NULL, tcp_thread, NULL);

   while(!is_tcp_finish && !is_udt_finish)
     ;

   return 1;
}

void* udt_thread(void* a)
{

   struct buffer send_buf;
   
   // use this function to initialize the UDT library
   UDT::startup();

   addrinfo hints;
   addrinfo* res;

   memset(&hints, 0, sizeof(struct addrinfo));

   hints.ai_flags = AI_PASSIVE;
   hints.ai_family = AF_INET;
   hints.ai_socktype = SOCK_STREAM;
   //hints.ai_socktype = SOCK_DGRAM;

   if (0 != getaddrinfo(NULL, service_control.c_str(), &hints, &res))
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
   //UDT::setsockopt(serv, 0, UDT_REUSEADDR, new bool(false), sizeof(bool));

   if (UDT::ERROR == UDT::bind(serv, res->ai_addr, res->ai_addrlen))
   {
      cout << "bind: " << UDT::getlasterror().getErrorMessage() << endl;
      return 0;
   }

   freeaddrinfo(res);

   cout << "\nUDT Control Server is ready at port: " << service_control << endl;

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
      cout << "Packet Size: " << PACKET_SIZE << endl; 
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
   
   is_udt_finish = true;
   if(is_tcp_finish)
     exit(0);

}

void* tcp_thread(void* b)
{
   static struct sockaddr_in server;
   int sd,cd; 
   int reuseaddr = 1;
   socklen_t client_len = sizeof(struct sockaddr_in);
//   int i = 0,j = 0;
//   int send_packet = 0;
   pthread_t p1;
   char service_port[NUM_PACKET_LENGTH];
   int ret = 0;
   
   //open socket
   sd = socket(AF_INET,SOCK_STREAM,0);
   if(sd < 0)
   {
     DIE("socket");
   }

   strcpy(service_port,tcp_service_port.c_str());

   printf("\n\nTCP Server is ready on Port: %s, num_packets: %d\n",service_port,num_packets);

   /* Initialize address. */
   server.sin_family = AF_INET;
   server.sin_port = htons(atoi(service_port));
   server.sin_addr.s_addr = htonl(INADDR_ANY);

   //reuse address
   setsockopt(sd,SOL_SOCKET,SO_REUSEADDR,&reuseaddr,sizeof(reuseaddr));

   //bind
   if(bind(sd,(struct sockaddr *)&server,sizeof(server)) < 0)
   {
     DIE("bind");
   }

   //listen
   if(listen(sd,1) < 0)
   {
     DIE("listen");
   }
   
   while(1)
   {
   //accept
/*   if((cd = accept(sd,(struct sockaddr *)&server,&client_len)) == -1)
   {
     DIE("accept");
   }
*/
//    pthread_mutex_lock(&work); 
     cd = accept(sd,(struct sockaddr *)&server,&client_len);
//    pthread_mutex_unlock(&work);
//     run_times = atoi(argv[1]);


//   printf("Start Sending!\nPacket Data Size: %d\n",(int)sizeof(buffer));
   /*create thread to handle each client, one receiver use one different protocol*/
     if((ret = pthread_create(&p1, NULL, send_packet, (void*)&cd)) != 0)
     {
       fprintf(stderr, "can't create p1 thread:%s\n", strerror(ret));
       exit(1);
     }
   }
   //close connection
   close(sd);
 
}

void *handle_client(void *arg)
{
  UDTSOCKET recver = *((UDTSOCKET *)arg);
  // packets
  int total_send_packets = 0;
  int total_send_size = 0;
  int sec = 0;
  int interval = 0;
  struct tms time_start,time_end;//use for count executing time
  struct buffer send_buf;

  // struct monitor_arg args;
//  int seq_client = total_number_clients;

  total_number_clients++;
  
  cout << "total_number_clients: " << total_number_clients << endl;
   
  int rs;
  char control_data[sizeof(START_TRANS)];

  // receive control msg (SOCK_STREAM)
  if (UDT::ERROR == (rs = UDT::recv(recver, control_data, sizeof(control_data), 0)))
  // receive control msg (SOCK_DGRAM)
  // if(UDT::ERROR == (rs = UDT::recvmsg(recver, control_data, sizeof(control_data))))
  {
     cout << "recv:" << UDT::getlasterror().getErrorMessage() << endl;
     exit(1);
  }
/*      else
      {
         cout << "rs: " << rs << endl;
      }
*/
  // if control_data is START_TRANS
  if(strcmp(control_data, START_TRANS) == 0) 
  {
    cout << "received control data: START_TRANS" << endl;
    // break;
  }

  // send NUM_PACKETS to client
  int ss = 0;
  char total_num_packets[NUM_PACKET_LENGTH];

  // convert int into string
  sprintf(total_num_packets,"%d",num_packets); 
  cout << "num_packets: " << num_packets << endl;
  if(UDT::ERROR == (ss = UDT::send(recver, total_num_packets, sizeof(total_num_packets), 0)))
  {
    cout << "send:" << UDT::getlasterror().getErrorMessage() << endl;
    exit(1); 
  }
  if(ss > 0)
  {
//        cout << "ss: " << ss << endl;
//        cout << "send NUM_PACKETS"  << endl;
//        break;
  }

  /*
  // send ttl to client
  char ttl_ms_char[NUM_PACKET_LENGTH];

  // args.ttl_ms = ttl_ms;
  // convert int into string
  sprintf(ttl_ms_char,"%d",ttl_ms); 
  cout << "TTL(ms): " << ttl_ms_char << endl;
  if(UDT::ERROR == (ss = UDT::send(recver, ttl_ms_char, sizeof(ttl_ms_char), 0)))
  {
    cout << "send:" << UDT::getlasterror().getErrorMessage() << endl;
    exit(1); 
  }
  if(ss > 0)
  {
//        cout << "ss: " << ss << endl;
     cout << "send ttl_ms"  << endl;
//        break;
  }
*/

/*
      #ifndef WIN32
         pthread_t rcvthread;
         pthread_create(&rcvthread, NULL, recvdata, new UDTSOCKET(recver));
         pthread_detach(rcvthread);
      #else
         CreateThread(NULL, 0, recvdata, new UDTSOCKET(recver), 0, NULL);
      #endif
*/
//   }
  // send port of data socket to client 
  // port_seq %= num_client;
  // cout << "elements in port_data_socket: " << sizeof(port_data_socket) << endl;
  if(UDT::ERROR == (ss = UDT::send(recver, (char *)service_data_port.c_str(), sizeof(service_data_port.c_str()), 0)))
  {
    cout << "send:" << UDT::getlasterror().getErrorMessage() << endl;
    exit(1); 
  }
  
  string service_data(DATA_DEFAULT_PORT); 
  if(ss > 0)
  {
    cout << "UDT Data Port: " << (char *)service_data_port.c_str() << ", sizeof(service_data_port.c_str()): " << sizeof(service_data_port.c_str()) << endl;
    service_data = service_data_port; 
    // port_seq++;
  }


  /* create data tranfer socket(using partial reliable message mode) */
  addrinfo hints;
  addrinfo* res;
   
  // reset hints
  memset(&hints, 0, sizeof(struct addrinfo));

  hints.ai_flags = AI_PASSIVE;
  hints.ai_family = AF_INET;
  hints.ai_socktype = SOCK_DGRAM;
 
  if (0 != getaddrinfo(NULL, service_data.c_str(), &hints, &res))
  {
    cout << "illegal port number or port is busy.\n" << endl;
    exit(1);
  }

  // exchange data packet
  UDTSOCKET serv_data = UDT::socket(res->ai_family, res->ai_socktype, res->ai_protocol);
   // UDT Options
   //UDT::setsockopt(serv, 0, UDT_CC, new CCCFactory<CUDPBlast>, sizeof(CCCFactory<CUDPBlast>));
   //UDT::setsockopt(serv, 0, UDT_MSS, new int(8999), sizeof(int));
   //UDT::setsockopt(serv, 0, UDT_RCVBUF, new int(10000000), sizeof(int));
   //UDT::setsockopt(serv, 0, UDP_RCVBUF, new int(10000000), sizeof(int));
   //UDT::setsockopt(serv_data, 0, UDT_REUSEADDR, new bool(false), sizeof(bool));

  if (UDT::ERROR == UDT::bind(serv_data, res->ai_addr, res->ai_addrlen))
  {
    cout << "bind(serv_data): " << UDT::getlasterror().getErrorMessage() << endl;
    exit(1);
  }

  freeaddrinfo(res);

  cout << "UDT Data Server is ready at port: " << service_data << endl;

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

  // send UDT packet
  is_udt_ready = true;
  // wait TCP to be ready
  while(!is_tcp_ready);
  printf("UDT is ready to send data!!!\n\n");
   
  // char send_data[PACKET_SIZE];
  int i = 0;
  int ssize = 0;
  int seq = 1;
  for(i = 0; i < num_packets; i++) {
   
    // record start time when receive first data packet
    if(i == 0)
    {
      //start time
      if((old_time_udt = times(&time_start)) == -1)
      {
        printf("time error\n");
        exit(1);
      }
    }

    // set seq
    send_buf.seq = seq;
    seq++;
    // cout << "send_buf.seq = " << send_buf.seq << endl;

    // set data
    memset(send_buf.data, '1', sizeof(send_buf.data));

    // send data
    //if(UDT::ERROR == (ssize = UDT::send(recver2, send_data, sizeof(send_data), 0)))
    
   // cout << "sizeof(send_buf): " << sizeof(send_buf) << endl;
    if(UDT::ERROR == (ssize = UDT::sendmsg(recver2, (char *)&send_buf, sizeof(send_buf), ttl_ms, false))) 
    {
      cout << "sendmsg:" << UDT::getlasterror().getErrorMessage() << endl;
      exit(1);
    }
    else
    {
      /*
      if(i == 0)
      {
        // create thread to monitor each server thread
        pthread_create(new pthread_t, NULL, monitor, &args);
      }
      */
//       cout << "send_size: " << ssize << endl;

      total_send_packets++;
      // args.total_send_pkt = total_send_packet;
      total_send_size += (ssize - sizeof(send_buf.seq));

    }
      
    // 40 => 1s, 12000 => 5min(300s)
    // output Result every 5 min
    // if(sec == 40 * 2)
    if(sec == 40 * 5 * 60)
    {
      printf("\n\n\n[UDT Result]:\n");
      printf("TTL(ms): %d\n", ttl_ms);
      interval++;
      printf("Interval: %d\n", interval);
      printf("UDT Total Send Packets: %d\n", total_send_packets);
      sec = 0;
    }

    // after sending, sleep 0.025s
    usleep(25000);
    sec++;
  }
   
  //finish time
  if((new_time_udt = times(&time_end)) == -1)
  {
    printf("time error\n");
    exit(1);
  }
 
/*
   // send control data
   char control_data2[sizeof(END_TRANS)];
   // set control data
   strcpy(control_data2, END_TRANS);
//   cout << "control_data2: " << control_data2 << endl;
  

   // send control data (SOCK_STREAM)
   if(UDT::ERROR == UDT::send(recver, control_data2, sizeof(control_data2), 0))
   // send control data (SOCK_DGRAM)
   // if(UDT::ERROR == UDT::sendmsg(recver2, control_data2, sizeof(control_data2), 3, false))
   {
      cout << "send:" << UDT::getlasterror().getErrorMessage() << endl;
   }
   else
   {
      cout << "send control data(END_TRANS)" << endl;
   }
*/
   printf("\n\n\n[UDT Finish Result]:\n");
   //executing time
   ticks=sysconf(_SC_CLK_TCK);
   udt_execute_time = (new_time_udt - old_time_udt)/ticks;
   printf("UDT Execute Time: %2.2f\n", udt_execute_time);
   cout << "UDT Total Send Packets: " << total_send_packets << endl;
   cout << "UDT Total Send Size: " << total_send_size << endl;
   
   double send_rate_bits = (total_send_packets * PACKET_SIZE * UNITS_BYTE_TO_BITS) / udt_execute_time;
   if(send_rate_bits >= UNITS_G)
   {
     send_rate_bits /= UNITS_G;
     printf("UDT Sending Rate: %2.2f (Gbits/s)\n", send_rate_bits);
     printf("UDT Sending Rate: %2.2f (GBytes/s)\n\n", send_rate_bits / UNITS_BYTE_TO_BITS);
   }
   else if(send_rate_bits >= UNITS_M)
   {
     send_rate_bits /= UNITS_M;
     printf("UDT Sending Rate: %2.2f (Mbits/s)\n", send_rate_bits);
     printf("UDT Sending Rate: %2.2f (MBytes/s)\n\n", send_rate_bits / UNITS_BYTE_TO_BITS);
   }
   else if(send_rate_bits >= UNITS_K)
   {
     send_rate_bits /= UNITS_K;
     printf("UDT Sending Rate: %2.2f (Kbits/s)\n", send_rate_bits);
     printf("UDT Sending Rate: %2.2f (KBytes/s)\n\n", send_rate_bits / UNITS_BYTE_TO_BITS);
   }
   else
   {
     printf("UDT Sending Rate: %2.2f (bits/s)\n", send_rate_bits);
     printf("UDT Sending Rate: %2.2f (Bytes/s)\n\n", send_rate_bits / UNITS_BYTE_TO_BITS);
   }

   
   // receive END_TRANS packet's ACK from client, then close all connection
   char control_data3[sizeof(END_TRANS)]; 
   rs = 0;
   
   printf("UDT wait END_TRANS\n");
   if (UDT::ERROR == (rs = UDT::recv(recver, control_data3, sizeof(control_data3), 0)))
   // receive control msg (SOCK_DGRAM)
   // if(UDT::ERROR == (rs = UDT::recvmsg(recver2, control_data3, sizeof(control_data3))))
   {
      cout << "recv:" << UDT::getlasterror().getErrorMessage() << endl;
      exit(1);
   } 
   printf("UDT finish waiting END_TRANS\n");
   if((rs > 0) && (strcmp(control_data3,END_TRANS) == 0))
   {
      total_number_clients--;
      cout << "UDT number of clients: " << total_number_clients << endl;
      printf("UDT get END_TRANS\n");
      
      UDT::close(serv_data);
      UDT::close(recver2);
/*
      if(total_number_clients == 0)
      {
        cout << "close all connections" << endl;
//        UDT::close(serv_data);
//        UDT::close(recver);
        // UDT::close(serv);
        // use this function to release the UDT library
        // UDT::cleanup();
      }
*/
   }

   return NULL;

}

void *send_packet(void *arg)
{
  int i = 0;
  int j = 0;
  struct buffer send_buf;
  int send_size = 0;
  int send_packet = 0;
  int total_send_size = 0;
  int cd = *((int *)arg);
  int no_client = num_client;
  char total_num_packets[NUM_PACKET_LENGTH]; 
  
  // compute execution time
//  clock_t old_time, new_time;
  struct tms time_start,time_end;//use for count executing time
//  double ticks;

  //send num_packets
  sprintf(total_num_packets,"%d",num_packets);
  int ss = 0;
  if((ss = send(cd,total_num_packets,sizeof(total_num_packets),0)) < 0)
  {
    DIE("send");
  }
  if(ss > 0)
  {
    printf("TCP Client %d: Send num_packets: %s\n",no_client,total_num_packets);
  }


  //int cd = (int *)&arg;
  num_client++;
 
  is_tcp_ready = true;
  // wait UDT to be ready
  while(!is_udt_ready)
    printf("wait UDT\n");
  
  printf("TCP Client %d: Start Sending Packet!\n",no_client);

   /*make packet & send packet*/
   int seq = 1;
   for (j = 0; j < num_packets ; j++)//number of packet
   {
      memset(send_buf.data,'1',sizeof(send_buf.data));//fill data in buffer
      
      // set seq
      send_buf.seq = seq;
      seq++;

      //send packet
      if((send_size = send(cd,(char *)&send_buf,sizeof(send_buf),0)) < 0)
      {
        DIE("send");
      }
      
      if(send_size > 0)
      {
        if(j == 0)
	{
	  //start time
          if((old_time_tcp = times(&time_start)) == -1)
          {
            DIE("times");
          }
	}
        send_packet++;
        total_send_size += (send_size - sizeof(send_buf.seq));
      }

      // sleep 1s, use to control sending rate
      usleep(25000);
   }
   //finish time
   if((new_time_tcp = times(&time_end)) == -1)
   {
     DIE("times");
   }
   /*************/
   printf("\n[TCP Result]\n");
   ticks=sysconf(_SC_CLK_TCK);
   tcp_execute_time = (new_time_tcp - old_time_tcp)/ticks;
   printf("TCP Execute Time: %2.2f\n", tcp_execute_time);
   printf("TCP Client %d: Total Send Packets: %d\n",no_client,send_packet);
   printf("TCP Client %d: Total Send Size: %d\n", no_client, total_send_size);
   printf("TCP Client %d: Packet send sucessfully!\n\n",no_client);	
   double send_rate_bits = (send_packet * PACKET_SIZE * UNITS_BYTE_TO_BITS) / tcp_execute_time;
   if(send_rate_bits >= UNITS_G)
   {
     send_rate_bits /= UNITS_G;
     printf("TCP Sending Rate: %2.2f (Gbits/s)\n", send_rate_bits);
     printf("TCP Sending Rate: %2.2f (GBytes/s)\n\n", send_rate_bits / UNITS_BYTE_TO_BITS);
   }
   else if(send_rate_bits >= UNITS_M)
   {
     send_rate_bits /= UNITS_M;
     printf("TCP Sending Rate: %2.2f (Mbits/s)\n", send_rate_bits);
     printf("TCP Sending Rate: %2.2f (MBytes/s)\n\n", send_rate_bits / UNITS_BYTE_TO_BITS);
   }
   else if(send_rate_bits >= UNITS_K)
   {
     send_rate_bits /= UNITS_K;
     printf("TCP Sending Rate: %2.2f (Kbits/s)\n", send_rate_bits);
     printf("TCP Sending Rate: %2.2f (KBytes/s)\n\n", send_rate_bits / UNITS_BYTE_TO_BITS);
   }
   else
   {
     printf("TCP Sending Rate: %2.2f (bits/s)\n", send_rate_bits);
     printf("TCP Sending Rate: %2.2f (Bytes/s)\n\n", send_rate_bits / UNITS_BYTE_TO_BITS);
   }

   is_tcp_finish = true;
   //close connection
   close(cd);
   // pthread_exit(0);
   if(is_udt_finish)
   {
     exit(0);
   }
}


/*
// seq_client, ttl, total_send_pkt
void* monitor(void* arg)
{
  struct monitor_arg args = *((struct monitor_arg *) arg);
  int seq_client = args.seq_client;
  int ttl = args.ttl_ms;
  int total_send_pkt = args.total_send_pkt;
  int interval = 0;
  
  while(true) 
  {
    usleep(output_interval);
    printf("\n\n[Result]:\n");
    printf("Client Seq: %d\n", seq_client);
    printf("TTL(ms): %d\n", ttl);
    interval++;
    printf("Interval: %d\n", interval);
    printf("Total Send Packets: %d\n", total_send_pkt);
  }

  return 0;

}
*/

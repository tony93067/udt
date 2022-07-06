#ifndef WIN32
   #include <unistd.h>
   #include <cstdlib>
   #include <cstring>
   #include <netdb.h>
   #include <stdio.h>
#else
   #include <winsock2.h>
   #include <ws2tcpip.h>
   #include <wspiapi.h>
#endif
#include <iostream>
#include <udt.h>
#include <sys/times.h>

#include "cc.h"

using namespace std;

// define control data
#define START_TRANS "start"
#define END_TRANS "end"

// define UNITS
#define UNITS_G 1000000000
#define UNITS_M 1000000
#define UNITS_K 1000
#define UNITS_BYTE_TO_BITS 8

// define packet size(bytes)
#define PACKET_SIZE 100
#define NUM_PACKET_LENGTH 1000

// define DEFAULT_PORT
#define CONTROL_DEFAULT_PORT "9000"
#define DATA_DEFAULT_PORT "8999"

/* global variables */
struct buffer {
    int seq;
    char data[PACKET_SIZE];
};
// compute execution time
clock_t old_time, new_time;
struct tms time_start,time_end;//use for count executing time
double ticks;
// packets
int total_recv_packets = 0;
int tmp_total_recv_packets = 0;// use for mode 2 to compute interval num of packets
int final_total_recv_packets = 0;
int total_recv_size = 0;
int tmp_total_recv_size = 0;
int final_total_recv_size = 0;
int num_packets = 0;
int seq_client = 0;
int ttl_ms = 0;
double execute_time;
double tmp_execute_time = 0;
double final_execute_time = 0.0f; 
double throughput_bits = 0.0f; 
string port_data_socket;
// finish recving flag
bool finish_recv = false;
// timer
int timer_counter = 1;
int total_timer_count = 0;
// finish connection
bool timeout = false;

int num_timeo = 0;
int continuous_timeo = 0;
// int recv_timeo = 5000;

int mode;
int output_interval = 5000000; // 5sec
//socket ID
UDTSOCKET client_control;
UDTSOCKET client_data;
struct buffer recv_buf;

// use to check if pkt is in-order or out-of-order when arrive receiver side
int current_seq;
int num_out_of_order = 0;

#ifndef WIN32
void* monitor(void*);
void* recv_control_data(void*);
void* start_timer(void*);
void reset_timer();
void print_throughput(double);
void close_connection();
#else
DWORD WINAPI monitor(LPVOID);
#endif

int main(int argc, char* argv[])
{
   if ((5 != argc) || (0 == atoi(argv[2])) || (atoi(argv[4]) != 1 && atoi(argv[4]) != 2))
   {
      cout << "usage: ./udtclient [server_ip] [server_port] [output_interval(sec)] [mode(1 or 2)]" << endl;
      return 0;
   }

   output_interval = atoi(argv[3]) * UNITS_M; // 1us * 1Ms => 1s (because usleep use u as unit)
   cout << "output_interval: " << output_interval << endl;

   mode = atoi(argv[4]);
   cout << "Choose mode: " << mode << endl;

   // use this function to initialize the UDT library
   UDT::startup();

   struct addrinfo hints, *local, *peer;

   memset(&hints, 0, sizeof(struct addrinfo));

   hints.ai_flags = AI_PASSIVE;
   hints.ai_family = AF_INET;
   hints.ai_socktype = SOCK_STREAM;
   // hints.ai_socktype = SOCK_DGRAM;

   if (0 != getaddrinfo(NULL, argv[2], &hints, &local))
   {
      cout << "incorrect network address.\n" << endl;
      return 0;
   }
  
   // exchange control packet
   client_control = UDT::socket(local->ai_family, local->ai_socktype, local->ai_protocol);

   // UDT Options
   //UDT::setsockopt(client, 0, UDT_CC, new CCCFactory<CUDPBlast>, sizeof(CCCFactory<CUDPBlast>));
   //UDT::setsockopt(client_control, 0, UDT_MSS, new int(9000), sizeof(int));
   //UDT::setsockopt(client, 0, UDT_SNDBUF, new int(10000000), sizeof(int));
   //UDT::setsockopt(client, 0, UDP_SNDBUF, new int(10000000), sizeof(int));

   freeaddrinfo(local);

   cout << "connect to Server: " << argv[1] << ", port: " << argv[2] << endl;

   if (0 != getaddrinfo(argv[1], argv[2], &hints, &peer))
   {
      cout << "incorrect server/peer address. " << argv[1] << ":" << argv[2] << endl;
      return 0;
   }

   // connect to the server, implict bind
   if (UDT::ERROR == UDT::connect(client_control, peer->ai_addr, peer->ai_addrlen))
   {
      cout << "connect: " << UDT::getlasterror().getErrorMessage() << endl;
      return 0;
   }

   freeaddrinfo(peer);

   int ss;
   char control_data[sizeof(START_TRANS)];

   strcpy(control_data,START_TRANS);
   
   // send control packet(SOCK_STREAM)
   if(UDT::ERROR == (ss = UDT::send(client_control, control_data, sizeof(control_data), 0)))
   {
     cout << "send:" << UDT::getlasterror().getErrorMessage() << endl;
     exit(1);
   }

   // receive num_packets_recv
   int rs = 0;
   char num_packets_recv[NUM_PACKET_LENGTH];
   if (UDT::ERROR == (rs = UDT::recv(client_control, num_packets_recv, sizeof(num_packets_recv), 0)))
   {
     cout << "recv:" << UDT::getlasterror().getErrorMessage() << endl;
     exit(1);
   }
  
   if(rs > 0)
   {
     num_packets = atoi(num_packets_recv);
     cout << "rs(num_packets_recv): " << rs << endl;
     cout << "num_packets: " << num_packets << endl; 
   }

   // receive seq_client_recv
   char seq_client_recv[NUM_PACKET_LENGTH];
   if (UDT::ERROR == (rs = UDT::recv(client_control, seq_client_recv, sizeof(seq_client_recv), 0)))
   {
     cout << "recv:" << UDT::getlasterror().getErrorMessage() << endl;
     exit(1);
   }  
   if(rs > 0)
   {
     seq_client = atoi(seq_client_recv);
     cout << "rs(seq_client_recv): " << rs << endl;
     cout << "seq_client: " << seq_client << endl;
   }

   // receive ttl_ms_recv
   char ttl_ms_recv[NUM_PACKET_LENGTH];
   if (UDT::ERROR == (rs = UDT::recv(client_control, ttl_ms_recv, sizeof(ttl_ms_recv), 0)))
   {
     cout << "recv:" << UDT::getlasterror().getErrorMessage() << endl;
     exit(1);
   }
  
   if(rs > 0)
   {
     ttl_ms = atoi(ttl_ms_recv);
     cout << "rs(ttl_ms_recv): " << rs << endl;
     cout << "ttl_ms: " << ttl_ms << endl;
   }

   
   // receive port_data_socket
   if (UDT::ERROR == (rs = UDT::recv(client_control, (char *)port_data_socket.c_str(), sizeof(port_data_socket.c_str()), 0)))
   {
     cout << "recv:" << UDT::getlasterror().getErrorMessage() << endl;
     exit(1);
   }
   
   string service_data(DATA_DEFAULT_PORT); 
   if(rs > 0)
   {
     cout << "rs(port_data_socket): " << rs << endl;
     cout << "port_data_socket: " << port_data_socket.c_str() << endl;
     service_data = port_data_socket;
     cout << "service_data: " << service_data.c_str() << endl;
   }

   /* create data tranfer socket(using partial reliable message mode) */
   // reset hints
   memset(&hints, 0, sizeof(struct addrinfo));

   hints.ai_flags = AI_PASSIVE;
   hints.ai_family = AF_INET;
   hints.ai_socktype = SOCK_DGRAM;

   if (0 != getaddrinfo(NULL, service_data.c_str(), &hints, &local))
   {
      cout << "incorrect network address.\n" << endl;
      return 0;
   }
  
   // exchange data packet
   client_data = UDT::socket(local->ai_family, local->ai_socktype, local->ai_protocol);

   // UDT Options
   //UDT::setsockopt(client_data, 0, UDT_RCVTIMEO, &recv_timeo, sizeof(int));
   //UDT::setsockopt(client_data, 0, UDT_CC, new CCCFactory<CUDPBlast>, sizeof(CCCFactory<CUDPBlast>));
   //UDT::setsockopt(client_data, 0, UDT_MSS, new int(8999), sizeof(int));
   //UDT::setsockopt(client_data, 0, UDT_SNDBUF, new int(10000000), sizeof(int));
   //UDT::setsockopt(client_data, 0, UDP_SNDBUF, new int(10000000), sizeof(int));
   
   freeaddrinfo(local);
   
   if (0 != getaddrinfo(argv[1], service_data.c_str(), &hints, &peer))
   {
      cout << "incorrect server/peer address. " << argv[1] << ":" << argv[2] << endl;
      return 0;
   }

   // connect to the server, implict bind
   if (UDT::ERROR == UDT::connect(client_data, peer->ai_addr, peer->ai_addrlen))
   {
      cout << "connect(client_data): " << UDT::getlasterror().getErrorMessage() << endl;
      return 0;
   }
   freeaddrinfo(peer);

   cout << "connect to Server: " << argv[1] << ", port: " << service_data.c_str() << endl;

   int rsize = 0;
   int i = 0;
  
   // create thread to enable timer
   pthread_t timerthread;
   int limit_time = 10;
   pthread_create(&timerthread, NULL, start_timer, &limit_time);
   for(i = 0; i < num_packets; i++)
   {

     // reset recv_buf.data
     memset(recv_buf.data, 0,sizeof(recv_buf.data));

     if(UDT::ERROR == (rsize = UDT::recvmsg(client_data, (char *)&recv_buf, sizeof(recv_buf)))) 
     {
       cout << "recvmsg:" << UDT::getlasterror().getErrorMessage() << endl;
       exit(1);
     }
     else
     {
       reset_timer();
        
       // record start time when receive first data packet
       if(i == 0)
       {
          // create thread to monitor socket(client_data)
          pthread_create(new pthread_t, NULL, monitor, &client_data);


         //start time
         if((old_time = times(&time_start)) == -1)
         {
           printf("time error\n");
           exit(1);
         }
         
       }
       else
       {
         cout << "Client Seq: " << seq_client << ", recv_buf.seq: " << recv_buf.seq << ", current_seq: " << current_seq << endl;
         if(current_seq > recv_buf.seq)
         {
           num_out_of_order++;
         }
       }
       current_seq = recv_buf.seq;
    
       total_recv_packets++;
       total_recv_size += (rsize - sizeof(recv_buf.seq));
     }

     //finish time
     if((new_time = times(&time_end)) == -1)
     {
       printf("time error\n");
       exit(1);
     }

   }
   
   if(continuous_timeo != 3)
   {
     close_connection();
   }
   return 1;
}
void close_connection()
{
   printf("\n[Close Connection]\n");
   printf("Client Seq: %d\n", seq_client);
   printf("TTL(ms): %d\n", ttl_ms); 
   printf("Num of Out-of-Order Packet(Cumulative): %d\n", num_out_of_order);
   execute_time = (new_time - old_time)/ticks; 
   if(execute_time != 0) 
   {
      throughput_bits = (total_recv_packets * PACKET_SIZE * UNITS_BYTE_TO_BITS)/execute_time;
   }
   else
   {
      throughput_bits = 0;
   }
   printf("Total Execute Time (sec): %2.2f\n", execute_time);
   printf("num_packets(from server): %d, total_recv_packets: %d\n", num_packets, total_recv_packets);
   printf("total_recv_size: %d\n", total_recv_size); 
   double tmp = (num_packets - total_recv_packets)/num_packets;
   if(mode == 2)
   {
     total_recv_packets = tmp_total_recv_packets;
   }
   cout << "[Data Loss]: " << (double)(num_packets * PACKET_SIZE - total_recv_size) * 100.0 / (num_packets * PACKET_SIZE) << " %" << endl;
   print_throughput(throughput_bits);  
 
   cout << "send END_TRANS" << endl;
   char control_data3[sizeof(END_TRANS)];
   int ss_control_data3 = 0;
   strcpy(control_data3,END_TRANS);
   if(UDT::ERROR == (ss_control_data3 = UDT::send(client_control, control_data3, sizeof(control_data3), 0))) 
   {
     cout << "send:" << UDT::getlasterror().getErrorMessage() << endl;
     exit(1);
   }
  
   if(ss_control_data3 > 0)
   {
     cout << "END connection" << endl;
     UDT::close(client_control);
     UDT::close(client_data);
  
     // use this function to release the UDT library
     UDT::cleanup();
   }
  
}

void* start_timer(void* sec)
{
  cout << "Enter timer thread!!" << endl;
 
  int limit_time = *(int*)sec;

  while(true)
  {
     if(timer_counter % limit_time == 0)
     {
       // timeout = true;
       cout << "\nTimer timeout!!" << endl;
       continuous_timeo++;
       total_timer_count = limit_time;
       // if timeout is continuous for 3 times
       if(continuous_timeo == 3)
       { 
         close_connection();
         break;
       }
     }
         
     timer_counter++; 
     sleep(1);
  }

  return NULL;
}

void reset_timer()
{

  timer_counter = 1;
  continuous_timeo = 0;
}

void* recv_control_data(void* usocket)
{
  UDTSOCKET client = *(UDTSOCKET*)usocket;
  delete (UDTSOCKET*)usocket;

  char control_data[sizeof(END_TRANS)];

  int rs_control = 0;

  if (UDT::ERROR == (rs_control = UDT::recv(client, control_data, sizeof(control_data), 0)))
  {
    cout << "recv1:" << UDT::getlasterror().getErrorMessage() << endl;
    pthread_exit(0);

  }
 
  // if control_data is END_TRANS
  if(strcmp(control_data, END_TRANS) == 0)
  {
    cout << "received control data: END_TRANS" << endl;

    char control_data3[sizeof(END_TRANS)];
    int ss = 0;
    if(UDT::ERROR == (ss = UDT::send(client, control_data3, sizeof(control_data3), 0))) 
    {
      cout << "send:" << UDT::getlasterror().getErrorMessage() << endl;
      exit(1);
    }

    if(ss > 0)
    {
      UDT::close(client);
    }
   }
   return NULL;
}

void print_throughput(double throughput)
{
  if(throughput_bits >= UNITS_G)
  {
    throughput_bits /= UNITS_G;
    printf("Throughput: %2.2f (Gbits/s)\n", throughput_bits);
    printf("Throughput: %2.2f (GBytes/s)\n", throughput_bits / UNITS_BYTE_TO_BITS);
  }
  else if(throughput_bits >= UNITS_M)
  {
    throughput_bits /= UNITS_M;
    printf("Throughput: %2.2f (Mbits/s)\n", throughput_bits);
    printf("Throughput: %2.2f (MBytes/s)\n", throughput_bits / UNITS_BYTE_TO_BITS);
  }
  else if(throughput_bits >= UNITS_K)
  {
    throughput_bits /= UNITS_K;
    printf("Throughput: %2.2f (Kbits/s)\n", throughput_bits);
    printf("Throughput: %2.2f (KBytes/s)\n", throughput_bits / UNITS_BYTE_TO_BITS);
  }
  else
  {
    printf("Throughput: %2.2f (bits/s)\n", throughput_bits);
    printf("Throughput: %2.2f (Bytes/s)\n", throughput_bits / UNITS_BYTE_TO_BITS);
  }
}

#ifndef WIN32
void* monitor(void* s)
#else
DWORD WINAPI monitor(LPVOID s)
#endif
{
   UDTSOCKET u = *(UDTSOCKET*)s;

   UDT::TRACEINFO perf;

   int interval = 0;
 
   while (true)
   {
      #ifndef WIN32
         usleep(output_interval);
      #else
         Sleep(1000);
      #endif

      printf("\n\n[Result]:\n");
      printf("Client Seq: %d\n", seq_client);
      printf("TTL(ms): %d\n", ttl_ms);
      interval++;
      printf("Interval: %d\n", interval);
      //executing time
      ticks=sysconf(_SC_CLK_TCK);
      printf("Packet Size (Bytes): %d\n", PACKET_SIZE);
      printf("Num of Out-of-Order Packet: %d\n", num_out_of_order);
      // interval mode
      if(mode == 2)
      {
         execute_time = (new_time - old_time)/ticks;
         final_execute_time = execute_time - tmp_execute_time;
         tmp_execute_time = execute_time;
         printf("Interval Execute Time (sec): %2.2f\n", final_execute_time);

         final_total_recv_packets = total_recv_packets;
         final_total_recv_size = total_recv_size;
         final_total_recv_packets -= tmp_total_recv_packets;
         final_total_recv_size -= tmp_total_recv_size;
         printf("Total Recv Packets(Interval): %d\n", final_total_recv_packets);
         printf("Total Recv Size(Interval): %d\n", final_total_recv_size);
         tmp_total_recv_packets = total_recv_packets;
         tmp_total_recv_size = total_recv_size;

         if(final_execute_time != 0)
         {
            throughput_bits = (final_total_recv_packets * PACKET_SIZE * UNITS_BYTE_TO_BITS)/final_execute_time;
         }
         else
         {
            throughput_bits = 0;
         }
      }
      // cumulative mode
      else if(mode == 1)
      {
        execute_time = (new_time - old_time)/ticks;
        printf("Cumulative Execute Time (sec): %2.2f\n", execute_time);

        printf("Total Recv Packets(Cumulative): %d\n", total_recv_packets);
        printf("Total Recv Size(Cumulative): %d\n", total_recv_size);
        
        if(execute_time != 0) 
        {
           throughput_bits = (total_recv_packets * PACKET_SIZE * UNITS_BYTE_TO_BITS)/execute_time;
        }
        else
        {
           throughput_bits = 0;
        }
      }
       
      print_throughput(throughput_bits);
   }

   #ifndef WIN32
      return NULL;
   #else
      return 0;
   #endif
}


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

#define DIE(x) perror(x),exit(1)

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
// argvs
char server_ip[100];
char server_port[100];
char tcp_server_port[100];
int mode;
int output_interval = 5000000; // 5sec

// UDT data port 
string service_data(DATA_DEFAULT_PORT); 

bool is_udt_ready = false;
bool is_tcp_ready = false;

// compute execution time
clock_t old_time_udt, new_time_udt, old_time_tcp, new_time_tcp;
// struct tms time_start,time_end;//use for count executing time
double ticks;
// packets(UDT)
int total_recv_packets = 0;
int tmp_total_recv_packets = 0;// use for mode 2 to compute interval num of packets
int final_total_recv_packets = 0;
int total_recv_size = 0;
int tmp_total_recv_size = 0;
int final_total_recv_size = 0;
int num_packets = 0;
// int seq_client = 0;
int ttl_ms = 0;
double tcp_execute_time;
double udt_execute_time;
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

// char recv_data[PACKET_SIZE];
//socket ID
UDTSOCKET client_control;
UDTSOCKET client_data;
// struct buffer recv_buf;

// use to check if pkt is in-order or out-of-order when arrive receiver side
int current_seq;
int num_out_of_order = 0;

// use to check tcp pkt seq & udt pkt seq
int udt_max_seq = 0;
int pkt_counter = 0;

//packets(TCP)
int tcp_total_recv_size = 0;
int tcp_sd;

bool is_udt_finish = false;
bool is_tcp_finish = false;

void* monitor(void*);
void* start_timer(void*);
void reset_timer();
void print_throughput(double);
void close_udt_connection();
void close_tcp_connection();
void* udt_thread(void*);
void* tcp_thread(void*);

int main(int argc, char* argv[])
{
   if ((6 != argc) || (0 == atoi(argv[2])) || (atoi(argv[4]) != 1 && atoi(argv[4]) != 2))
   {
      cout << "usage: ./udtclient [server_ip] [server_port] [output_interval(sec)] [mode(1 or 2)] [tcp_port]" << endl;
      return 0;
   }

   strcpy(server_ip, argv[1]);
   strcpy(server_port, argv[2]);
   strcpy(tcp_server_port, argv[5]);
   output_interval = atoi(argv[3]) * UNITS_M; // 1us * 1Ms => 1s (because usleep use u as unit)
   cout << "output_interval: " << output_interval << endl;
   mode = atoi(argv[4]);
   cout << "Choose mode: " << mode << endl;
   
   // create UDT thread
   pthread_t udtthread;
   pthread_create(&udtthread, NULL, udt_thread, NULL);

   // create TCP thread
   pthread_t tcpthread;
   pthread_create(&tcpthread, NULL, tcp_thread, NULL);

   //create monitor thread
   pthread_create(new pthread_t, NULL, monitor, NULL); 

   while(true)
   {
     if(is_udt_finish && is_tcp_finish)
     {

       cout << "Finish Loop" << endl;
       break;
     }
   }

   return 1;
}

void close_udt_connection()
{
   printf("\n[Close UDT Connection]\n");
//   printf("Client Seq: %d\n", seq_client);
   printf("TTL(ms): %d\n", ttl_ms); 
   // printf("Num of Out-of-Order Packet(Cumulative): %d\n", num_out_of_order);
   udt_execute_time = (new_time_udt - old_time_udt)/ticks; 
   if(udt_execute_time != 0) 
   {
      throughput_bits = (total_recv_packets * PACKET_SIZE * UNITS_BYTE_TO_BITS)/udt_execute_time;
   }
   else
   {
      throughput_bits = 0;
   }
   printf("Total Execute Time (sec): %2.2f\n", udt_execute_time);
   printf("num_packets(from server): %d, total_recv_packets: %d\n", num_packets, total_recv_packets);
   printf("total_recv_size: %d\n", total_recv_size); 
   double tmp = (num_packets - total_recv_packets)/num_packets;
   if(mode == 2)
   {
     total_recv_packets = tmp_total_recv_packets;
   }
   cout << "[Data Loss]: " << (double)(num_packets * PACKET_SIZE - total_recv_size) * 100.0 / (num_packets * PACKET_SIZE) << " %" << endl;
   // printf("[Data Loss]: %5.1f ", tmp * 100);
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
     
     is_udt_finish = true;
     if(is_tcp_finish)
       exit(0);

   }
  
}

void close_tcp_connection()
{
   printf("\n\n\n[Close TCP connection]\n");
   /*executing time*/
   ticks=sysconf(_SC_CLK_TCK);
   tcp_execute_time = (new_time_tcp-old_time_tcp)/ticks;
   printf("TCP Execute Time: %2.2f\n", tcp_execute_time);
   printf("TCP Total Recv Size: %d\n", tcp_total_recv_size);
   printf("Num of Delay TCP Pkt: %d\n\n", pkt_counter); 
   
   close(tcp_sd);

   is_tcp_finish = true;
   if(is_udt_finish)
     exit(0);
}

void* start_timer(void* sec)
{
  cout << "111Enter timer thread!!" << endl;
 
  int limit_time = *(int*)sec;
  // delete (int*)sec;

  cout << "Enter timer thread!! limit_time: " << limit_time << endl;
 
  while(true)
  {
     if(timer_counter % limit_time == 0)
     {
       // timeout = true;
       cout << "\nTimer timeout!!" << endl;
       continuous_timeo++;
       total_timer_count = limit_time;
       // close_connection();
       // if timeout is continuous for 3 times
       // cout << "continuous_timeo: " << continuous_timeo << endl;
       if(continuous_timeo == 3)
       { 
         close_udt_connection();
         break;
       }
    }
              // break;  
    
     timer_counter++;
     // cout << "\ntimer_counter: " << timer_counter << endl;
     // cout << "continuous_timeo: " << continuous_timeo << endl;
     // use when compute execution time (we have to minus total_timer_count)
     // cout << "timer_counter: " << timer_counter << endl;
  
     sleep(1);
  }

  return NULL;
}

void reset_timer()
{

  timer_counter = 1;
  continuous_timeo = 0;
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

void* monitor(void* a)
{
  printf("Enter monitor thread!!\n\n");
/*
   UDTSOCKET u = *(UDTSOCKET*)s;

   UDT::TRACEINFO perf;
*/
   int udt_interval = 0;
   int tcp_interval = 0;

   // cout << "SendRate(Mb/s)\tRTT(ms)\tCWnd\tPktSndPeriod(us)\tRecvACK\tRecvNAK\tRecvTotal" << endl;
//   cout << "SendingRate(Mb/s)\tCWnd\tPktSndPeriod(us)\tpktSentTotal\tpktRecvTotal\tpktRetransTotal\tpktSndLossTotal\tpktRcvLossTotal\tbyteAvailSndBuf\tbyteAvailRcvBuf" << endl;
   while (true)
   {
      #ifndef WIN32
         // sleep 60sec(1min)
         usleep(output_interval);
      #else
         Sleep(1000);
      #endif

      printf("\n\n[UDT Result]:\n");
//      printf("Client Seq: %d\n", seq_client);
      printf("UDT Data Port: %s\n", service_data.c_str());
      printf("UDT TTL(ms): %d\n", ttl_ms);
      // interval += (output_interval/UNITS_M);
      udt_interval++;
      printf("UDT Interval: %d\n", udt_interval);
      //executing time
      ticks=sysconf(_SC_CLK_TCK);
      // cout << "Total Timer Count: " << total_timer_count << endl;
      // double execute_time = (new_time - old_time)/ticks - total_timer_count;
      printf("UDT_Packet Size (Bytes): %d\n", PACKET_SIZE);
//      printf("Num of Out-of-Order Packet: %d\n", num_out_of_order);
      // interval mode
      if(mode == 2)
      {
         udt_execute_time = (new_time_udt - old_time_udt)/ticks;
         final_execute_time = udt_execute_time - tmp_execute_time;
         tmp_execute_time = udt_execute_time;
         // final_execute_time -= (num_timeo * recv_timeo);
         printf("UDT Interval Execute Time (sec): %2.2f\n", final_execute_time);

         // cout << "total_recv_packet: " << total_recv_packets << endl; 
         final_total_recv_packets = total_recv_packets;
         final_total_recv_size = total_recv_size;
         final_total_recv_packets -= tmp_total_recv_packets;
         final_total_recv_size -= tmp_total_recv_size;
         printf("UDT Total Recv Packets(Interval): %d\n", final_total_recv_packets);
         printf("UDT Total Recv Size(Interval): %d\n", final_total_recv_size);
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
        udt_execute_time = (new_time_udt - old_time_udt)/ticks;
        // cout << "num_timeo: " << num_timeo << ", recv_timeo: " << recv_timeo << endl;
        // execute_time -= (num_timeo * recv_timeo);
        printf("UDT Cumulative Execute Time (sec): %2.2f\n", udt_execute_time);

        printf("UDT Total Recv Packets(Cumulative): %d\n", total_recv_packets);
        printf("UDT Total Recv Size(Cumulative): %d\n", total_recv_size);
        
        if(udt_execute_time != 0) 
        {
           throughput_bits = (total_recv_packets * PACKET_SIZE * UNITS_BYTE_TO_BITS)/udt_execute_time;
        }
        else
        {
           throughput_bits = 0;
        }
      }
      
     /*
      double tmp = (num_packets - total_recv_packets)/num_packets;
      printf("Data Loss: %2.3f %%\n", tmp * 100);
*/   
      print_throughput(throughput_bits);

      // print TCP
      printf("\n\n\n[TCP Result]:\n");
      // tcp_interval++;
      // printf("TCP Interval: %d\n", tcp_interval);
      printf("TCP port: %s\n", tcp_server_port);
      tcp_execute_time = (new_time_tcp - old_time_tcp)/ticks;
      printf("TCP Execute Time (sec): %2.2f\n", tcp_execute_time);
      printf("TCP Total Recv Size(Cumulative): %d\n", tcp_total_recv_size);
      printf("Num of Delay TCP Pkt: %d\n", pkt_counter); 
/*
      if (UDT::ERROR == UDT::perfmon(u, &perf))
      {
         cout << "perfmon: " << UDT::getlasterror().getErrorMessage() << endl;
         break;
      }
*/
/*
      cout << perf.mbpsSendRate << "\t\t" 
           << perf.msRTT << "\t" 
           << perf.pktCongestionWindow << "\t" 
           << perf.usPktSndPeriod << "\t\t\t" 
           << perf.pktRecvACK << "\t" 
           << perf.pktRecvNAK << "\t"
	   << perf.pktRecvTotal << endl;
*/
  
/*    cout << perf.mbpsSendRate << "\t\t"
           << perf.pktCongestionWindow << "\t"
           << perf.usPktSndPeriod << "\t\t\t"
           << perf.pktSentTotal << "\t\t"
	   << perf.pktRecvTotal << "\t\t"
	   << perf.pktRetransTotal << "\t\t"
 	   << perf.pktSndLossTotal << "\t\t"
	   << perf.pktRcvLossTotal << "\t\t"
	   << perf.byteAvailSndBuf << "\t\t"
           << perf.byteAvailRcvBuf << endl;
*/
   }

   #ifndef WIN32
      return NULL;
   #else
      return 0;
   #endif
}

void* udt_thread(void* a)
{

   // use this function to initialize the UDT library
   UDT::startup();

   struct addrinfo hints, *local, *peer;
   struct buffer recv_buf;
//   UDTSOCKET client_control;
//   UDTSOCKET client_data;
   struct tms time_start,time_end;//use for count executing time


   memset(&hints, 0, sizeof(struct addrinfo));

   hints.ai_flags = AI_PASSIVE;
   hints.ai_family = AF_INET;
   hints.ai_socktype = SOCK_STREAM;

   if (0 != getaddrinfo(NULL, server_port, &hints, &local))
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

   cout << "connect to Server: " << server_ip << ", port: " << server_port << endl;

   if (0 != getaddrinfo(server_ip, server_port, &hints, &peer))
   {
      cout << "incorrect server/peer address. " << server_ip << ":" << server_port << endl;
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
     // cout << "rs(num_packets_recv): " << rs << endl;
     // cout << "num_packets: " << num_packets << endl; 
   }
/*
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
     // cout << "rs(seq_client_recv): " << rs << endl;
     // cout << "seq_client: " << seq_client << endl;
   }
*/

/*
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
     // cout << "rs(ttl_ms_recv): " << rs << endl;
     // cout << "ttl_ms: " << ttl_ms << endl;
   }
*/
   
   // receive port_data_socket
   if (UDT::ERROR == (rs = UDT::recv(client_control, (char *)port_data_socket.c_str(), sizeof(port_data_socket.c_str()), 0)))
   {
     cout << "recv:" << UDT::getlasterror().getErrorMessage() << endl;
     exit(1);
   }
   
   if(rs > 0)
   {
     // cout << "rs(port_data_socket): " << rs << endl;
     // cout << "port_data_socket: " << port_data_socket.c_str() << endl;
     service_data = port_data_socket;
     // cout << "service_data: " << service_data.c_str() << endl;
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

   // cout << "local->ai_family: " << local->ai_family << ", local->ai_socktype: " << local->ai_socktype << ", local->ai_protocol: " << local->ai_protocol << endl; 

   // UDT Options
   //UDT::setsockopt(client_data, 0, UDT_RCVTIMEO, &recv_timeo, sizeof(int));
   // UDT::setsockopt(client_data, 0, UDT_CC, new CCCFactory<CUDPBlast>, sizeof(CCCFactory<CUDPBlast>));
   //UDT::setsockopt(client_data, 0, UDT_MSS, new int(8999), sizeof(int));
   //UDT::setsockopt(client_data, 0, UDT_SNDBUF, new int(10000000), sizeof(int));
   //UDT::setsockopt(client_data, 0, UDP_SNDBUF, new int(10000000), sizeof(int));
   
   freeaddrinfo(local);
   
   if (0 != getaddrinfo(server_ip, service_data.c_str(), &hints, &peer))
   {
      cout << "incorrect server/peer address. " << server_ip << ":" << server_port << endl;
      return 0;
   }

   // connect to the server, implict bind
   if (UDT::ERROR == UDT::connect(client_data, peer->ai_addr, peer->ai_addrlen))
   {
      cout << "connect(client_data): " << UDT::getlasterror().getErrorMessage() << endl;
      return 0;
   }
   freeaddrinfo(peer);

   cout << "connect to Server: " << server_ip << ", port: " << service_data.c_str() << endl;

   // create thread to wait control packet(END_TRANS)
   // pthread_t rcvthread;
   // pthread_create(&rcvthread, NULL, recv_control_data, new UDTSOCKET(client_data));

   // create thread to monitor socket(client_data)
//   pthread_create(new pthread_t, NULL, monitor, &client_data);

   int rsize = 0;
   int i = 0;
 /* 
   is_udt_ready = true;
   while(!is_tcp_ready);
*/
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
         // pthread_create(new pthread_t, NULL, monitor, &client_data);
         // pthread_create(new pthread_t, NULL, monitor, NULL); 


         //start time
         if((old_time_udt = times(&time_start)) == -1)
         {
           printf("time error\n");
           exit(1);
         }

         udt_max_seq = recv_buf.seq;
         
       }
       else
       {
         if(recv_buf.seq > udt_max_seq)
         {
           udt_max_seq = recv_buf.seq; 
         }
         
         /*
         cout << "Client Seq: " << seq_client << ", UDT recv_buf.seq: " << recv_buf.seq << ", UDT current_seq: " << current_seq << endl;
         if(current_seq > recv_buf.seq)
         {
           num_out_of_order++;
         }
         */
       }
       // current_seq = recv_buf.seq;
       // cout << "udt_max_seq: " << udt_max_seq << endl;
       cout << "UDT recv_buf.seq: " << recv_buf.seq << endl; 
       total_recv_packets++;
       total_recv_size += (rsize - sizeof(recv_buf.seq));
     }
     //finish time
     if((new_time_udt = times(&time_end)) == -1)
     {
       printf("time error\n");
       exit(1);
     }

   }

   cout << "continuous_timeo: " << continuous_timeo << endl;
   
   if(continuous_timeo != 3)
   {
     close_udt_connection();
   }

}

void* tcp_thread(void* b)
{
   static struct sockaddr_in server;
   // clock_t old_time,new_time;//use for count executing time
   struct tms time_start,time_end;//use for count executing time
   // double ticks;
   // int sd;
   struct hostent *host;
   // char server_name[SNAME_SIZE];
   // char service_port[10];
   // char tcp_buffer[PACKET_SIZE];
   struct buffer tcp_buf;
   int recv_size = 0;
   int recv_packet = 0;
   int num_packets = 0;

   printf("new connection to: %s, port: %s\n", server_ip, tcp_server_port);
   
   /* Set up destination address. */
   server.sin_family = AF_INET;
   host = gethostbyname(server_ip);
   server.sin_port = htons(atoi(tcp_server_port));    
   memcpy((char*)&server.sin_addr,host->h_addr_list[0],host->h_length);

   tcp_sd = socket(AF_INET,SOCK_STREAM,0);
   if(tcp_sd < 0)
   {
     DIE("socket");
   }

   /* Connect to the server. */
   if(connect(tcp_sd,(struct sockaddr*)&server,sizeof(server)) == -1)
   {
     DIE("connect");
   }
   
   printf("Start Receiving!\n");
      
   // receive number of packets from server
   int rs = 0;
   char num_packets_recv[NUM_PACKET_LENGTH];
   if((rs = recv(tcp_sd,num_packets_recv,sizeof(num_packets_recv),0)) < 0)
   {
     DIE("recv");
   }
   if(rs > 0)
   {
     num_packets = atoi(num_packets_recv);
     printf("num_packets: %d\n",num_packets);  
   }

   /*receive packet*/
   // int total_recv_size = 0;
/* 
   is_tcp_ready = true;
   while(!is_udt_ready);
*/
   while(1)
   {
     if((recv_size = recv(tcp_sd,(char *)&tcp_buf,sizeof(tcp_buf),0)) < 0)
     {
       DIE("recv");
     }
     
     if(recv_size > 0)
     {
       recv_packet++;
       if(recv_packet == 1)
       {
 	  //start time
          if((old_time_tcp = times(&time_start)) == -1)
   	  {
             DIE("times");
   	  }
       }
       tcp_total_recv_size += (recv_size - sizeof(int));
       
       printf("udt_max_seq: %d, tcp_seq: %d\n\n\n", udt_max_seq, tcp_buf.seq);
       if(tcp_buf.seq < udt_max_seq)
       {
         pkt_counter++; 
       }
     } 
     else if(recv_size == 0)//last packet, break loop
     {
       break;
     }
     //finish time
     if((new_time_tcp = times(&time_end)) == -1)
     {
       DIE("times");
     }
     /********************************/
   }
  
   close_tcp_connection();
   

}

// pingwait.cpp : 

// To find power up or connect response delay:
//   sends out multiple ping packets & reports time of first response
//   3 parameters: IP addr, overall timeout_ms, ping_interval_ms
//     bad IP address return current microseconds for other timing use

#define _WINSOCK_DEPRECATED_NO_WARNINGS

#include <winsock2.h>
#include <iphlpapi.h>
#include <icmpapi.h>
#include <stdio.h>

#pragma comment(lib, "iphlpapi.lib")
#pragma comment(lib, "ws2_32.lib")

HANDLE replied;
IPAddr ipaddr;

const int SEND_SIZE = 8;
char SendData[SEND_SIZE] = "pong";
char ReplyBuffer[sizeof(ICMP_ECHO_REPLY) + sizeof(SendData)];
PICMP_ECHO_REPLY pEchoReply = (PICMP_ECHO_REPLY)ReplyBuffer;

int ping_interval_ms = 10;  // time report resolution
int timeout_ms = 2000;

DWORD WINAPI sendPings(LPVOID param) {
  while (timeout_ms > 0) {
    IcmpSendEcho2(IcmpCreateFile(), replied, NULL, NULL, ipaddr, SendData, sizeof(SendData), NULL, ReplyBuffer, sizeof(ReplyBuffer), timeout_ms);
    Sleep(ping_interval_ms);
    timeout_ms -= ping_interval_ms;  // + overhead; earlier timeout_ms will terminate
  }
  return -1;
}

int __cdecl main(int argc, char **argv) {
  const char* ipStr = "8.8.8.8";
  if (argc > 1) ipStr = argv[1];
  if (argc > 2) timeout_ms = atoi(argv[2]);
  if (argc > 3) ping_interval_ms = atoi(argv[3]);

  ipaddr = inet_addr(ipStr);
  replied = CreateEvent(NULL, FALSE, FALSE, NULL);
  
  LARGE_INTEGER freq, start, end;
  int elapsed_ms;
  QueryPerformanceFrequency(&freq);

  CreateThread(NULL, 0, sendPings, NULL, 0, NULL);
  QueryPerformanceCounter(&start);

  if (ipaddr == INADDR_NONE)
    return (int)(start.QuadPart * 1000000 / freq.QuadPart); // current microseconds

  while (1) {
    WaitForSingleObject(replied, timeout_ms);
    switch (pEchoReply->Status) {
       case IP_SUCCESS : 
         QueryPerformanceCounter(&end);
         elapsed_ms = (int)((end.QuadPart - start.QuadPart) * 1000 / freq.QuadPart - pEchoReply->RoundTripTime);
         printf("Up in %d ms  roundtrip %ld ms\n", elapsed_ms, pEchoReply->RoundTripTime);
         return elapsed_ms;
       case IP_DEST_NET_UNREACHABLE :
       case IP_DEST_HOST_UNREACHABLE :
         break; // keep trying
       case IP_REQ_TIMED_OUT: printf("Timed out\n"); return -1;
       default: printf("Echo status %ld\n", pEchoReply->Status);  // TODO: handle any other rare status returned
    }
  }
}
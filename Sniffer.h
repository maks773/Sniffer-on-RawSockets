#include <conio.h>
#include <iostream>
#include <iomanip>
#include <Winsock2.h>
#include <WS2tcpip.h>
#include <string>
#include <vector>
#include <windows.h>
#include <time.h>
#include <bitset>
#include <Mstcpip.h>
#include <fstream>
#include <iphlpapi.h>
#include <psapi.h>
#include <algorithm>
#include <sstream>
#include <chrono>
using namespace std;

#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "iphlpapi.lib")


                                   // �������� ���������� ����������


extern WSADATA wsData;             // ���������� � ������ ������� � ��
extern SOCKET s;                   // �������� ������ 
extern wstring previous_name;      // ��� �������� � ����������� ������������ ������



                                   // �������� ��������


#pragma pack(push, 1)
typedef struct // ��������� ip ������
{
	BYTE   ip_ver_hlen;            // ������ ��������� � ����� ��������� (4 + 4 ����)
	BYTE   ip_tos;                 // ��� �������
	UINT16 ip_length;              // ����� ����� ������ � ������
	UINT16 ip_id;                  // ������������� ������
	UINT16 ip_flag_offset;         // ����� � �������� ���������(3 + 13 ���)
	BYTE   ip_ttl;                 // TTL
	BYTE   ip_protocol;            // �������� �������� ������
	UINT16 ip_crc;                 // ����������� �����
	UINT32 ip_src_addr;            // ip-����� ���������
	UINT32 ip_dst_addr;            // ip-����� ����������
} IPHeader;

typedef struct // ��������� TCP ���������
{
	UINT16 tcp_srcport;            // ���� ���������
	UINT16 tcp_dstport;            // ���� �����������
	UINT32 tcp_seq;                // ���������� �����
	UINT32 tcp_ack;                // ����� �������������
	UINT16 tcp_hlen_flags;         // ����� ���������, ������ � ����� (4 + 6 + 6 ���)
	UINT16 tcp_window;             // ������ ����
	UINT16 tcp_crc;                // ����������� �����
	UINT16 tcp_urg_pointer;        // ��������� ���������
} TCPHeader;

typedef struct // ��������� UDP ���������
{
	UINT16 udp_srcport;            // ���� ���������
	UINT16 udp_dstport;            // ���� �����������
	UINT16 udp_length;             // ����� ����� ������ � ������
	UINT16 udp_xsum;               // ����������� �����
} UDPHeader;

typedef struct // ��������� ����������� ��������� pcap-�����  
{
	UINT32 magic_number;           // ���������� �����
	UINT16 version_major;          // ����� �������� ������ ������� �����
	UINT16 version_minor;          // ����� ������������� ������ ������� �����
	int    thiszone;               // ������� ����
	UINT32 sigfigs;                // �������� ��������� �����
	UINT32 snaplen;                // ������������ ����� ������
	UINT32 network;                // �������� ���������� ������ (������ �������� ������)
} pcap_hdr;

typedef struct // ��������� ��������� ������� ������ ��� pcap-�����
{
	UINT32 ts_sec;                 // ��������� ������� (�������)
	UINT32 ts_usec;                // ��������� ������� (������������)
	UINT32 incl_len;               // ���������� ����� ������
	UINT32 orig_len;               // ����������� ����� ������
} pcappack_hdr;

typedef struct // ��������� ��� ���������� ���������� �������
{	
	IPHeader  ipheader;            // ��������� IP
	TCPHeader tcpheader;           // ��������� TCP
	UDPHeader udpheader;	       // ��������� UDP
} temp_buf;
#pragma pack(pop)



                                   // �������� �������


void error_exit(int);                           // ������ � ������� ��������� �� ������ � ����������� �� � ����

void ShowIPHeaderInfo(IPHeader*);               // ����� � ������� ���������� �� ��������� IP

void ShowTCPHeaderInfo(TCPHeader*);             // ����� � ������� ���������� �� ��������� TCP

void ShowUDPHeaderInfo(UDPHeader*);             // ����� � ������� ���������� �� ��������� UDP

void ShowPacketData(IPHeader*, vector<BYTE>&);  // ������ � ������� IP ������ � ������� hex � ASCII

// ������ � ������� ���������� � ������ � ���� ������ (������� �����)
void print_info(int, IPHeader*, TCPHeader*, UDPHeader*, wstring&);

// ��������� ������ � ������� ���������� � ������ (��������� IP + TCP/UDP + ����� � ������� hex � ASCII)
void print_packet(int, IPHeader*, TCPHeader*, UDPHeader*, wstring&, vector<BYTE>&);

wstring GetProcessNameByPID(DWORD);             // ��������� ����� �������� �� PID

// ����� ������ IP+���� ������������ TCP ������ � ������� TCP-����������
wstring GetTcpProcessName(IPHeader*, TCPHeader*, wstring&);

// ����� ������ IP+���� ������������ UDP ������ � ������� UDP-����������
wstring GetUdpProcessName(IPHeader*, UDPHeader*, wstring&);

void init_gen_pcap_header(pcap_hdr*);           // ������������� ��������� ����������� ��������� pcap

void writehead_to_pcap(HANDLE&);                // ������ ����������� ��������� � pcap-����

// ������ ������������ ������ � ��� pcap-��������� � pcap-����
void writepack_to_pcap(HANDLE&, vector<BYTE>, UINT16, wstring&);

int isDNS(TCPHeader*, UDPHeader*);              // ��������, �������� dns-����� ��� ���
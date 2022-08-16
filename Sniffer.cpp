#include "sniffer.h"
using namespace std;


int main(int argc, char *argv[])                  // �������� ��� ���������
{
	int err;                                      // ������ ��������, ������������ ��������� (��� ������) 
	
	err = WSAStartup(MAKEWORD(2,2), &wsData);     // ������������� ���������� ������� � �������� �������
	if (err != 0) 
		error_exit(1);		

	s = socket(AF_INET, SOCK_RAW, 0);             // ������������� ������
	if (s == INVALID_SOCKET) 
		error_exit(2);

	char host_buf[256];                           // ��� �������� ����� �����
	addrinfo hints = {}, *addrs, *addr;
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_RAW;
	hints.ai_protocol = IPPROTO_IP;

	err = gethostname(host_buf, sizeof(host_buf));    // ��������� ����� �����
	if (err == -1) 
		error_exit(3);

	err = getaddrinfo(host_buf, 0, &hints, &addrs);   // ��������� IP-������� ������� ����������� ��
	if (err != 0)  		
	
	cout << "Select the interface to capture:" << endl << endl;
	
	char count = '1';                                 // ����� ������� IP-������� � ������� 
	vector<sockaddr *> ip(100);	
	for (addr = addrs; addr != NULL; addr = addr->ai_next) {
		ip[count - '0'] = addr->ai_addr; char buf_ip[20];
		cout << count << ". " << inet_ntop(AF_INET, &((sockaddr_in*)ip[count - '0'])->sin_addr,
																						buf_ip, 16) << endl;
		count++;
	}
	
	char num;
    cout << endl << "Please, enter the number of interface: ";
	do                                               // ����� ������ ����������
	{   
		num = _getche();
	}   while (num >= count || num < '1');

	err = bind(s, ip[num - '0'], sizeof(sockaddr));  // �������� ������ � ���������� ����������
	if (err != 0)  
		error_exit(5);		
	
	freeaddrinfo(addrs);

	ULONG flag = RCVALL_ON; ULONG z = 0;             // ����������� ������� ��������� � ������������� �����
	err = WSAIoctl(s, SIO_RCVALL, &flag, sizeof(flag), NULL, 0, &z, NULL, NULL);
	if (err == SOCKET_ERROR) 
		error_exit(6);		

	wstring enter_procname = L"NULL";
	cout << "\n\n\n\n" << "Use process filtering? (y/n): ";
	do
	{
		num = _getche();
	} while (num != 'y' && num != 'n');             // ������ ������� �� ������������ �������� (y) ��� ��� (n)

	if (num == 'y') {
		cout << endl << "Please, enter name of the process (for example, chrome.exe): ";
		getline(wcin, enter_procname);		        // ���� (y), ��������� � ������� ��� ��������
	}                                               // ��� ������ ���� ����-�-����, ��� � ������ netstat

	HANDLE hInput = GetStdHandle(STD_INPUT_HANDLE);    // ��������� ����� �������� �������������� � �������
	DWORD prevConsoleMode;
	GetConsoleMode(hInput, &prevConsoleMode);
	SetConsoleMode(hInput, prevConsoleMode & ENABLE_EXTENDED_FLAGS);

	cout << "\n\n\n\n" << "1. Full print (IP, TCP/UDP headers + Packet Data)" << endl;
	cout << "2. Short print in single-line format" << endl;
	cout << "3. Quiet mode. Only write to pcap file" << endl << endl;
	cout << "Please, select the mode: ";
	do
	{
		num = _getche();
	} while (num != '1' && num != '2' && num != '3');   // ����� ������ ������ ���������
	
	int p_count = 0;                                    // ������� ���������� ���������� �������
	HANDLE hFile = NULL;                                // ����� ��� pcap-�����
	vector<temp_buf> Temp(65535);                       // ��� �������� ������� ����� DNS-�������� � �������
	vector<vector<BYTE>> Buf(65535);
	int t = 0;                                          // ������� ������� ����� DNS-�������� � �������
	flag = 100;                                         // 100 - ��������� ��������� �����

	writehead_to_pcap(hFile);                           // ������ ����������� ��������� � pcap-����

	cout << "\n\n" << "Start packet capture...  [ TO STOP capture PRESS ANY KEY ]" << "\n\n";	

	while( !_kbhit() ) // �������� ������ �������
	{
		vector<BYTE> Buffer(65535);                     // ����� ��� �������� ������������ ������
		IPHeader* iph = NULL;  TCPHeader* tcph = NULL;  UDPHeader* udph = NULL;
		wstring process_name = L"Unknown process";      // ��� �������� ����� ��������

 		int byte_rcv = recvfrom(s, (char*)&Buffer[0], (int)Buffer.size(), 0, NULL, 0); // �������� ����� �� ����
		if (byte_rcv >= sizeof(IPHeader))
		{
			iph = (IPHeader *)&Buffer[0];                        // �������� ��������� IP
			UINT ip_hlen = (UINT)((iph->ip_ver_hlen & 15)*4);    // ����� IP-���������
			
			if (iph->ip_protocol == IPPROTO_TCP) {
				tcph = (TCPHeader*)(&Buffer[0] + ip_hlen);       // �������� ��������� TCP
				process_name = GetTcpProcessName(iph, tcph, enter_procname); // ���� ����� ������ � ��������� � ��
			}
			else if (iph->ip_protocol == IPPROTO_UDP) {
				udph = (UDPHeader*)(&Buffer[0] + ip_hlen);       // �������� ��������� UDP
				process_name = GetUdpProcessName(iph, udph, enter_procname); // ���� ����� ������ � ��������� � �� 
			}
			else
				continue;

			int is_dns = isDNS(tcph, udph);       // ��������, ��������� ����� � DNS (0,1) ��� ��� (2)               
			
			if (is_dns == 2 && flag == 1) {       // ��������� ������ ����� dns-������
				string buf_ip(16, '\0');  vector<int> temp_ip(4, 0);  int nn = 0;  string temp;
				inet_ntop(AF_INET, &iph->ip_dst_addr, (char*)buf_ip.c_str(), 16);				
				stringstream stream(buf_ip);

				while (getline(stream, temp, '.')) {
					temp_ip[nn] = stoi(temp);    // ������� IP-������ ��������� � ������ ������� �� ������ �����
					nn++;
				}

				vector<int> int_pack(65535);	 // ������� ������ � ������ ����� �������� (int)		
				for (nn = 0; nn < 65535; nn++) int_pack[nn] = Buf[t - 1][nn];

				// ����� IP ������ ��������� TCP ������ � DNS-������ (��� ��� ����� DNS-������
				// ��� ����� TCP c ���������� ���������� �� ������ �� ���������� �������)
				auto it = search(int_pack.begin(), int_pack.end(), temp_ip.begin(), temp_ip.end());
				if (it != int_pack.end() && process_name != L"NULL") {
					for (int i = 0; i < t; i++) { // ���� ����� ������ � TCP-����� ������ � ���������, �� �������
						p_count++;                // ����������� DNS-�����, ������ � ������ ����� ����,
						                          // ��� ��� DNS-������ ���� ������� � ���� ���������
						if (num == '2')
							print_info(p_count, &Temp[i].ipheader, &Temp[i].tcpheader,
								&Temp[i].udpheader, process_name);
						else
							if (num == '1')
								print_packet(p_count, &Temp[i].ipheader,
									&Temp[i].tcpheader, &Temp[i].udpheader, process_name, Buf[i]);

						writepack_to_pcap(hFile, Buf[i], ntohs(Temp[i].ipheader.ip_length), process_name);						
					}
					t = 0; flag = 100;    // �������� ������� � ������ ���� � �������� ���������
				}
				else {
					t = 0; flag = 100;
					continue;             // ����� ��������� � ������� ������� ��� ������ ����������
				}
			}
			
			if (is_dns != 2 || flag == 0) {	 // ��������� ������ ����� DNS-�������� � DNS-�������			
				Temp[t].ipheader = *iph;
				if (udph != NULL)
					Temp[t].udpheader = *udph;
				else
					Temp[t].tcpheader = *tcph;
				Buf[t] = Buffer;  t++;
				if (is_dns == 0)             // DNS-������
					flag = 0;
				else if (is_dns == 1)        // DNS-�����
						flag = 1;				
				continue;
			}


			if (process_name != L"NULL") {  // ���� ����������� ����� ������ � ���������, ������� ���
				p_count++;
				if (num == '2')
					print_info(p_count, iph, tcph, udph, process_name);
				else
					if (num == '1')
							print_packet(p_count, iph, tcph, udph, process_name, Buffer);

				writepack_to_pcap(hFile, Buffer, ntohs(iph->ip_length), process_name);
			}			
		}
	}

	cout << "\n\nPacket capture completed!\n\n";	
	SetConsoleMode(hInput, prevConsoleMode); // ������� ������� �������� �������
	CloseHandle(hInput);
	CloseHandle(hFile);
	closesocket(s);
	WSACleanup();	
	system("pause");
	return 0;
}
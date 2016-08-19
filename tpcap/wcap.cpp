
#pragma comment(lib, "Ws2_32.lib")

#include <winsock2.h>
#include <mstcpip.h>
//#include <ws2tcpip.h>
#include <stdio.h>
#include <windows.h>

#include "IPHlpApi.h"

#define ERROR_PRO(X) { \
        char *s; \
        char error_string[1024]; \
        DWORD dw = GetLastError(); \
        FormatMessage( FORMAT_MESSAGE_FROM_SYSTEM, NULL, dw, \
                MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPTSTR) error_string, 1024, NULL );\
        s= strstr(error_string, "\r\n") ; \
        if (s )  *s = '\0';  \
        printf("%s errno %d, %s", X,dw, error_string);\
        }

int main()
{
		int WSAStartup_result = 0;
		SOCKET fd;
		int i ;
		unsigned char buf[8192];

		 	DWORD dwBufferLen[10] ;
		DWORD dwBufferInLen= 1 ;
		DWORD dwBytesReturned = 0 ;
		SOCKADDR_IN sa;
//		struct in_addr me;

		WSADATA wsaData;
		WSAStartup_result = WSAStartup(MAKEWORD(2,2), &wsaData);

		if ( WSAStartup_result != 0 ) 
	{
		/* Tell the user that we could not find a usable */
		/* WinSock DLL.                                  */
		ERROR_PRO("Load WinSock DLL failed!");
       
	}

	
		fd = socket( AF_INET , SOCK_RAW , IPPROTO_IP ) ;
		if ( fd == INVALID_SOCKET ) 
		{
			ERROR_PRO("socket ")
		}

		/*
 if( SOCKET_ERROR != WSAIoctl(fd, SIO_RCVALL , &dwBufferInLen, sizeof(dwBufferInLen),             
                                      &dwBufferLen, sizeof(dwBufferLen),
									  &dwBytesReturned , NULL , NULL ) )
		{
			ERROR_PRO("WSAIoctl ")
		}
*/

		sa.sin_family = AF_INET;
 		sa.sin_port = htons(7000);
		//sa.sin_addr.s_addr= htonl( INADDR_ANY );

       {
                struct in_addr me;

				#define HAS_ADDR (me.s_addr = inet_addr("192.168.3.162")) == INADDR_NONE
                if ( HAS_ADDR )
                {
                        struct hostent* he;
                        he = gethostbyname("192.168.3.162");
                        if ( he == (struct hostent*) 0 )
                        {
                                ERROR_PRO("Invalid address")
                                
                        } else
                        {
                                (void) memcpy(&sa.sin_addr, he->h_addr, he->h_length );
                         }
                   } else
                   {
                        sa.sin_addr.s_addr = me. s_addr;
                   }
        }

        if (bind(fd,(struct sockaddr *)&sa, sizeof(sa)) == SOCKET_ERROR)
		{
			ERROR_PRO("bind")		
		} 
 
		if( SOCKET_ERROR == WSAIoctl(fd, SIO_RCVALL , &dwBufferInLen, sizeof(dwBufferInLen),             
                                      &dwBufferLen, sizeof(dwBufferLen),
									  &dwBytesReturned , NULL , NULL ) )
		{
			ERROR_PRO("WSAIoctl ")
		}

		for ( i = 0; i < 1000; i++)
		{
			int iRet,j;
			FILE *fp;
					iRet = 	recv( fd , (char*)buf , sizeof( buf ) , 0 ) ;
				if( iRet == SOCKET_ERROR )
				{
					ERROR_PRO("recv ")
				} else 
				{
					unsigned char http[64];
					http[0] = 0x00;
					http[1] = 0x50;
					//printf("recv %d\n", iRet);
					if ( memcmp(&buf[22], http, 2) == 0 ||
						memcmp(&buf[20], http, 2) == 0 )
					{
						fopen_s(&fp,"c:\\tcp_hsm.txt", "a+");
						if (!fp )
						{
							ERROR_PRO("fopen ")
							continue; 
						}
						fprintf_s(fp, "recv %d\n", iRet);
						for ( j =0 ; j < 8; j++ )
						{
							fprintf_s(fp, "%02X ", buf[20+j]);
						}
						fprintf_s(fp, "\n");
						fclose(fp);
					}
	
					
						

				}
	

		}


	return 0;
}


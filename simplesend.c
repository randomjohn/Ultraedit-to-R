// compile me with mingw
// gcc simplesend.c -o send.exe -lws2_32

// the following is suggested here:
// http://readlist.com/lists/lists.sourceforge.net/mingw-users/0/1543.html
// without it, mingw can't find getaddrinfo and freeaddrinfo
#define _WIN32_WINNT 0x501

// code is copied from msdn tutorial
// http://msdn.microsoft.com/en-us/library/ms738545(v=VS.85).aspx
#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdlib.h>
#include <stdio.h>


// Need to link with Ws2_32.lib, Mswsock.lib, and Advapi32.lib
#pragma comment (lib, "Ws2_32.lib")
#pragma comment (lib, "Mswsock.lib")
#pragma comment (lib, "AdvApi32.lib")


#define DEFAULT_SENDLEN 1024
#define DEFAULT_BUFLEN 512
#define DEFAULT_PORT "8888"
// the following is a change from the tutorial
#define DEFAULT_ADDY "127.0.0.1"

void usage(char *me,int status)
{
	// call with usage(argv[0],1) to exit with a warning or usage(argv[0],0) to exit normally
	printf("Usage: %s [-f file] [-p port] [-c command string] [-e]\n",me);
	printf("       -f specifies a file path with R commands (incompatible with -c)\n");
	printf("       -p specifies a port number on which R should be listening (default 8888)\n");
	printf("       -c specifies a command to send to R, note there is a problem with double\n");
	printf("          quotes getting stripped, so this really has limited utility\n");
	printf("       -e echos input (contents of clipboard or file if applicable)\n");
	printf("       -h prints this help and ignores other switches\n");
	printf("       DOS style switches with a slash (/) can also be used.\n");
	printf("       If neither -f or -c is specified, the content of the clipboard is sent.\n");
	printf("       Essentially, this command sends text to port 8888 (or specified port)\n");
	printf("       and outputs the response of the socket server to STDIN.\n");
	printf("       Use  R, with the svSocket package and startSocket() command, to use this\n");
	printf("       tool as a way to send commands and capture output from R. This was\n");
	printf("       developed to give Ultraedit a way to talk to R, but it can be used with\n");
	printf("       any tool that can call a command line program and capture its output.\n");
	exit(status);
}
	
// clearly, this is a change 
void get_args(int argc, char** argv, char* port, char* cmd,int* echo,int* customcmd, char *file)
{
  int i,j;
  strcpy(file,"");
  /* Start at i = 1 to skip the command name. */
  for (i = 1; i < argc; i++) {
		/* Check for a switch (leading "-" as in unix or "/" as in MSDOS). */
		if (argv[i][0] == '-' || argv[i][0]=='/') {
		    /* Use the next character to decide what to do. */
		  switch (argv[i][1]) {
	
			case 'p':	strcpy(port,argv[++i]);
					break;
	
			case 'c':	
				if (strlen(file)==0) {
					strcpy(cmd,argv[++i]);
					*customcmd=1;
				} else {
					fprintf(stderr,"-f and -c switches are incompatible\n");
					usage(argv[0],1);
				}
				break;
					
			case 'e': *echo=1;
				break;
				
			case 'f': 
				if (!*customcmd) {
					strcpy(file, argv[++i]);
					// convert backslashes to forward slashes for R command
					for (j=0;j<strlen(file);j++) if (file[j]=='\\') file[j]='/';
					sprintf(cmd,"source(\"%s\");",file);
					// and get the original path back so we can print it
					strcpy(file,argv[i]);
				} else {
					fprintf(stderr,"-f and -c switches are incompatible\n");
					usage(argv[0],1);
				}
				break;
	
			case 'h': usage(argv[0],0);
				break;
				
			default:	fprintf(stderr,
					"Unknown switch %s\n", argv[i]);
					usage(argv[0],1);
		  }
		} else {
			fprintf(stderr,"Unknown command %s\n",argv[i]);
			exit(1);
		}
  }
}


int __cdecl main(int argc, char **argv) 
{
    WSADATA wsaData;
    SOCKET ConnectSocket = INVALID_SOCKET;
    struct addrinfo *result = NULL,
                    *ptr = NULL,
                    hints;
    //char *sendbuf = "this is a test";
    char recvbuf[DEFAULT_BUFLEN];
    char sendbuf[DEFAULT_SENDLEN]="source(\"clipboard\");";
    char sendport[DEFAULT_BUFLEN]=DEFAULT_PORT;
    char file[DEFAULT_BUFLEN];
    FILE *fhandle;
    char *clipbuf;
    int iResult,i,j,mylen,cmdarg=1,fchar;
    int recvbuflen = DEFAULT_BUFLEN;
    int echo=0,customcmd=0;
    char quote='"';
    
    echo=0;
 		if (argc>5) {
 			fprintf(stderr,"Too many arguments.\n");
 			usage(argv[0],1);
 		} else {
 			get_args(argc,argv,sendport,sendbuf,&echo,&customcmd,file);
 		}

    // Initialize Winsock
    iResult = WSAStartup(MAKEWORD(2,2), &wsaData);
    if (iResult != 0) {
        printf("WSAStartup failed with error: %d\n", iResult);
        return 1;
    }

    ZeroMemory( &hints, sizeof(hints) );
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;

    // Resolve the server address and port
    // I changed the following line from the tutorial - we know the addy
    // (127.0.0.1) and so argv contains what is sent
    iResult = getaddrinfo(DEFAULT_ADDY, DEFAULT_PORT, &hints, &result);
    if ( iResult != 0 ) {
        printf("getaddrinfo failed with error: %d\n", iResult);
        WSACleanup();
        return 1;
    }

    // Attempt to connect to an address until one succeeds
    for(ptr=result; ptr != NULL ;ptr=ptr->ai_next) {

        // Create a SOCKET for connecting to server
        ConnectSocket = socket(ptr->ai_family, ptr->ai_socktype, 
            ptr->ai_protocol);
        if (ConnectSocket == INVALID_SOCKET) {
            printf("socket failed with error: %ld\n", WSAGetLastError());
            WSACleanup();
            return 1;
        }

        // Connect to server.
        iResult = connect( ConnectSocket, ptr->ai_addr, (int)ptr->ai_addrlen);
        if (iResult == SOCKET_ERROR) {
            closesocket(ConnectSocket);
            ConnectSocket = INVALID_SOCKET;
            continue;
        }
        break;
    }

    freeaddrinfo(result);

    if (ConnectSocket == INVALID_SOCKET) {
        printf("Unable to connect to server!\n");
        WSACleanup();
        return 1;
    }

		// echo the command we are sending
		if (echo) {
			if (customcmd) {
				// we specified the command on the command line, so echo it
				printf("%s\n",sendbuf);
			} else if (strlen(file)>0) {
				// we specified the command in a file, so type out the file
				fhandle=fopen(file,"r");
				while((fchar=fgetc(fhandle)) != EOF) putchar(fchar);
				fclose(fhandle);
			} else {
				// command is on the clipboard, so echo clipboard
				if(OpenClipboard(NULL))
				{
					clipbuf = (char*)GetClipboardData(CF_TEXT);
					printf("%s\n",clipbuf);		
				}
				CloseClipboard(); 
			}
		}
    // Send an initial buffer
    iResult = send( ConnectSocket, sendbuf, (int)strlen(sendbuf), 0 );
    if (iResult == SOCKET_ERROR) {
        printf("send failed with error: %d\n", WSAGetLastError());
        closesocket(ConnectSocket);
        WSACleanup();
        return 1;
    }

    // printf("Bytes Sent: %ld\n", iResult);

    // shutdown the connection since no more data will be sent
    iResult = shutdown(ConnectSocket, SD_SEND);
    if (iResult == SOCKET_ERROR) {
        printf("shutdown failed with error: %d\n", WSAGetLastError());
        closesocket(ConnectSocket);
        WSACleanup();
        return 1;
    }

    // Receive until the peer closes the connection
    do {

        iResult = recv(ConnectSocket, recvbuf, recvbuflen, 0);
        if ( iResult > 0 ) {
            // printf("Bytes received: %d\n", iResult);
            recvbuf[iResult]=0;
            printf("%s",recvbuf);
        }
        else if ( iResult == 0 ) {
            // printf("Connection closed\n");
        }
        else
            printf("recv failed with error: %d\n", WSAGetLastError());

    } while( iResult > 0 );

    // cleanup
    closesocket(ConnectSocket);
    WSACleanup();

    return 0;
}

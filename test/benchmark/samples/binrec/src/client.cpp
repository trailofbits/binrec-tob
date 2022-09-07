//
// A simple Internet client application.
// It connects to a remote server,
// receives a "Hello" message from a server,
// sends the line "Thanks! Bye-bye..." to a server,
// and terminates.
//
// Usage:
//          client [IP_address_of_server [port_of_server]]
//      where IP_address_of_server is either IP number of server
//      or a symbolic Internet name, default is "localhost";
//      port_of_server is a port number, default is 1234.
//

#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

static void usage();

int main(int argc, char *argv[]) {
    if (argc > 1 && *(argv[1]) == '-') {
        usage();
        exit(1);
    }

    // Create a socket
    int s0 = socket(AF_INET, SOCK_STREAM, 0);
    if (s0 < 0) {
        perror("Cannot create a socket");
        exit(1);
    }

    // Fill in the address of server
    struct sockaddr_in peeraddr{};
    const char *peerHost = "localhost";
    if (argc > 1)
        peerHost = argv[1];

    // Resolve the server address (convert from symbolic name to IP number)
    struct hostent *host = gethostbyname(peerHost);
    if (host == NULL) {
        perror("Cannot define host address");
        exit(1);
    }
    peeraddr.sin_family = AF_INET;
    short peerPort = 1234;
    if (argc >= 3)
        peerPort = (short)atoi(argv[2]);
    peeraddr.sin_port = htons(peerPort);

    // Print a resolved address of server (the first IP of the host)
    printf("peer addr = %d.%d.%d.%d, port %d\n", host->h_addr_list[0][0] & 0xff, host->h_addr_list[0][1] & 0xff,
           host->h_addr_list[0][2] & 0xff, host->h_addr_list[0][3] & 0xff, (int)peerPort);

    // Write resolved IP address of a server to the address structure
    memmove(&(peeraddr.sin_addr.s_addr), host->h_addr_list[0], 4);

    // Connect to a remote server
    int res = connect(s0, (struct sockaddr *)&peeraddr, sizeof(peeraddr));
    if (res < 0) {
        perror("Cannot connect");
        exit(1);
    }
    printf("Connected. Reading a server message.\n");

    char buffer[1024];
    res = read(s0, buffer, 1024);
    if (res < 0) {
        perror("Read error");
        exit(1);
    }
    buffer[res] = 0;
    printf("Received:\n%s", buffer);

    write(s0, "Thanks! Bye-bye...\r\n", 20);

    close(s0);
    return 0;
}

static void usage() {
    printf("A simple Internet client application.\n"
           "Usage:\n"
           "         client [IP_address_of_server [port_of_server]]\n"
           "     where IP_address_of_server is either IP number of server\n"
           "     or a symbolic Internet name, default is \"localhost\";\n"
           "     port_of_server is a port number, default is 1234.\n"
           "The client connects to a server which address is given in a\n"
           "command line, receives a message from a server, sends the message\n"
           "\"Thanks! Bye-bye...\", and terminates.\n");
}

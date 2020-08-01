//
// A simple Internet server application.
// It listens to the port written in command line (default 1234),
// accepts a connection, and sends the "Hello!\r\n" message
// to a client. Then it receives the answer from a client and terminates.
//
// Usage:
//      server [port_to_listen]
// Default is the port 1234.
//
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>

static void usage();

int main(int argc, char *argv[]) {
    if (argc > 1 && *(argv[1]) == '-') {
        usage(); exit(1);
    }

    int listenPort = 1234;
    if (argc > 1)
        listenPort = atoi(argv[1]);

    // Create a socket
    int s0 = socket(AF_INET, SOCK_STREAM, 0);
    if (s0 < 0) {
        perror("Cannot create a socket"); exit(1);
    }

    // Fill in the address structure containing self address
    struct sockaddr_in myaddr;
    memset(&myaddr, 0, sizeof(struct sockaddr_in));
    myaddr.sin_family = AF_INET;
    myaddr.sin_port = htons(listenPort);        // Port to listen
    myaddr.sin_addr.s_addr = htonl(INADDR_ANY);

    // Bind a socket to the address
    int res = bind(s0, (struct sockaddr*) &myaddr, sizeof(myaddr));
    if (res < 0) {
        perror("Cannot bind a socket"); exit(1);
    }

    // Set the "LINGER" timeout to zero, to close the listen socket
    // immediately at program termination.
    struct linger linger_opt = { 1, 0 }; // Linger active, timeout 0
    setsockopt(s0, SOL_SOCKET, SO_LINGER, &linger_opt, sizeof(linger_opt));

    // Now, listen for a connection
    res = listen(s0, 1);    // "1" is the maximal length of the queue
    if (res < 0) {
        perror("Cannot listen"); exit(1);
    }

    // Accept a connection (the "accept" command waits for a connection with
    // no timeout limit...)
    struct sockaddr_in peeraddr;
    socklen_t peeraddr_len;
    int s1 = accept(s0, (struct sockaddr*) &peeraddr, &peeraddr_len);
    if (s1 < 0) {
        perror("Cannot accept"); exit(1);
    }

    // A connection is accepted. The new socket "s1" is created
    // for data input/output. The peeraddr structure is filled in with
    // the address of connected entity, print it.
    printf(
        "Connection from IP %d.%d.%d.%d, port %d\n",
        (ntohl(peeraddr.sin_addr.s_addr) >> 24) & 0xff, // High byte of address
        (ntohl(peeraddr.sin_addr.s_addr) >> 16) & 0xff, // . . .
        (ntohl(peeraddr.sin_addr.s_addr) >> 8) & 0xff,  // . . .
        ntohl(peeraddr.sin_addr.s_addr) & 0xff,         // Low byte of addr
        ntohs(peeraddr.sin_port)
    );

    res = close(s0);    // Close the listen socket

    write(s1, "Hello!\r\n", 8);

    char buffer[1024];
    res = read(s1, buffer, 1023);
    if (res < 0) {
        perror("Read error"); exit(1);
    }
    buffer[res] = 0;
    printf("Received %d bytes:\n%s", res, buffer);

    close(s1);          // Close the data socket
    return 0;
}

static void usage() {
    printf(
        "A simple Internet server application.\n"
        "It listens to the port written in command line (default 1234),\n"
        "accepts a connection, and sends the \"Hello!\" message to a client.\n"
        "Then it receives the answer from a client and terminates.\n\n"
        "Usage:\n"
        "     server [port_to_listen]\n"
        "Default is the port 1234.\n"
    );
}

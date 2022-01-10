/**
 * Example code for getting the IP address from hostname.
 * tidy up includes
 */
#include <stdio.h>
#include <stdlib.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>

int main(int argc, char *argv[])
{
    int h;

    if (argc != 2)
    {
        fprintf(stderr, "Usage: %s <address to get IP address>\n", argv[0]);
        exit(-1);
    }

    /**
 * The struct hostent (host entry) with its terms documented

    struct hostent {
        char *h_name;    // Official name of the host.
        char **h_aliases;    // A NULL-terminated array of alternate names for the host.
        int h_addrtype;    // The type of address being returned; usually AF_INET.
        int h_length;    // The length of the address in bytes.
        char **h_addr_list;    // A zero-terminated array of network addresses for the host.
        // Host addresses are in Network Byte Order.
    };

    #define h_addr h_addr_list[0]	The first address in h_addr_list.
*/
    struct addrinfo hints;

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    struct addrinfo* results, *it;

    if ((h = getaddrinfo(argv[1], "ftp", &hints, &results)) != 0)
    {
        herror("getaddrinfo()");
        exit(-1);
    }

    for(it = results; it != NULL; it = it->ai_next) {
        struct sockaddr_in *ipv4 = (struct sockaddr_in *)it->ai_addr;
        char ip[INET_ADDRSTRLEN];

        inet_ntop(AF_INET, &(ipv4->sin_addr), ip, INET_ADDRSTRLEN);

        printf("Host name  : %s\n", argv[1]);
        printf("IP Address : %s\n", ip);
        printf("PORT: %d\n", ntohs(ipv4->sin_port));
    }

    freeaddrinfo(results);

    return 0;
}

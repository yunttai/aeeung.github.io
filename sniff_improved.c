#include <stdlib.h>
#include <stdio.h>
#include <pcap.h>
#include <arpa/inet.h>
#include <string.h>

/* Ethernet header */
struct ethheader {
    u_char  ether_dhost[6]; /* destination host address */
    u_char  ether_shost[6]; /* source host address */
    u_short ether_type;     /* protocol type (IP, ARP, RARP, etc) */
};

/* IP Header */
struct ipheader {
    unsigned char      iph_ihl:4, /* IP header length */
                       iph_ver:4; /* IP version */
    unsigned char      iph_tos; /* Type of service */
    unsigned short int iph_len; /* IP Packet length (data + header) */
    unsigned short int iph_ident; /* Identification */
    unsigned short int iph_flag:3, /* Fragmentation flags */
                       iph_offset:13; /* Flags offset */
    unsigned char      iph_ttl; /* Time to Live */
    unsigned char      iph_protocol; /* Protocol type */
    unsigned short int iph_chksum; /* IP datagram checksum */
    struct  in_addr    iph_sourceip; /* Source IP address */
    struct  in_addr    iph_destip;   /* Destination IP address */
};

/* TCP Header */
struct tcpheader {
    u_short tcph_srcport; /* Source port */
    u_short tcph_destport; /* Destination port */
    u_int   tcph_seqnum; /* Sequence number */
    u_int   tcph_acknum; /* Acknowledgement number */
    u_char  tcph_offset:4, /* Data offset */
            tcph_reserved:4; /* Reserved */
    u_char  tcph_flags; /* Flags */
    u_short tcph_win; /* Window */
    u_short tcph_chksum; /* Checksum */
    u_short tcph_urgptr; /* Urgent pointer */
};

void got_packet(u_char *args, const struct pcap_pkthdr *header,
                const u_char *packet)
{
    struct ethheader *eth = (struct ethheader *)packet;

    if (ntohs(eth->ether_type) == 0x0800) { // 0x0800 is IP type
        struct ipheader *ip = (struct ipheader *)(packet + sizeof(struct ethheader));

        if (ip->iph_protocol == IPPROTO_TCP) { // Check if the protocol is TCP
            struct tcpheader *tcp = (struct tcpheader *)(packet + sizeof(struct ethheader) + ip->iph_ihl * 4);

            printf("Ethernet Header:\n");
            printf("   Src MAC: %02x:%02x:%02x:%02x:%02x:%02x\n",
                   eth->ether_shost[0], eth->ether_shost[1], eth->ether_shost[2],
                   eth->ether_shost[3], eth->ether_shost[4], eth->ether_shost[5]);
            printf("   Dst MAC: %02x:%02x:%02x:%02x:%02x:%02x\n",
                   eth->ether_dhost[0], eth->ether_dhost[1], eth->ether_dhost[2],
                   eth->ether_dhost[3], eth->ether_dhost[4], eth->ether_dhost[5]);

            printf("IP Header:\n");
            printf("   Src IP: %s\n", inet_ntoa(ip->iph_sourceip));
            printf("   Dst IP: %s\n", inet_ntoa(ip->iph_destip));

            printf("TCP Header:\n");
            printf("   Src Port: %d\n", ntohs(tcp->tcph_srcport));
            printf("   Dst Port: %d\n", ntohs(tcp->tcph_destport));

            int ip_header_len = ip->iph_ihl * 4;
            int tcp_header_len = tcp->tcph_offset * 4;
            int payload_size = ntohs(ip->iph_len) - (ip_header_len + tcp_header_len);

            printf("Message Size: %d bytes\n", payload_size);
            printf("\n");
        }
    }
}

int main()
{
    pcap_t *handle;
    char errbuf[PCAP_ERRBUF_SIZE];
    struct bpf_program fp;
    char filter_exp[] = "tcp";
    bpf_u_int32 net;

    // Step 1: Open live pcap session on NIC with name enp0s3
    handle = pcap_open_live("enp0s3", BUFSIZ, 1, 1000, errbuf);
    if (handle == NULL) {
        fprintf(stderr, "Couldn't open device %s: %s\n", "enp0s3", errbuf);
        return 2;
    }

    // Step 2: Compile filter_exp into BPF pseudo-code
    if (pcap_compile(handle, &fp, filter_exp, 0, net) == -1) {
        fprintf(stderr, "Couldn't parse filter %s: %s\n", filter_exp, pcap_geterr(handle));
        return 2;
    }
    if (pcap_setfilter(handle, &fp) == -1) {
        fprintf(stderr, "Couldn't install filter %s: %s\n", filter_exp, pcap_geterr(handle));
        return 2;
    }

    // Step 3: Capture packets
    pcap_loop(handle, -1, got_packet, NULL);

    pcap_close(handle);   // Close the handle
    return 0;
}

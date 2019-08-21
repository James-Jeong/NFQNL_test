#include "stdafx.h"

#define HW_MAX_LEN 1

bool is_harmful = false;

vector<char*> harmful_website;

void dump(unsigned char* buf, int size) {
    int i;
    for (i = 0; i < size; i++) {
        if (i % 16 == 0)
            printf("\n");
        printf("%02x ", buf[i]);
    }
}

uint8_t check_Port(u_char* Packet_DATA){
    struct libnet_tcp_hdr* TH = (struct libnet_tcp_hdr*)(Packet_DATA);
    // TCP data offset check
    if(TH->th_off < 4) return false;

    char dp[6];
    sprintf(dp, "%d", ntohs(TH->th_dport));

    if(!strncmp(dp, "80", 2)){
        return (TH->th_off * 4);
    }
    return 0;
}

bool is_Harmful_Web(char* dns){
    for(int i = 0; i < HW_MAX_LEN; i++){
        if(!strncmp(dns, harmful_website.at(i), strlen(dns))){
            printf("[ Success to Drop packets of %s ]\n", dns);
            return true;
        }
    }
    return false;
}

/* returns packet id */
static u_int32_t print_pkt (struct nfq_data *tb)
{
    int id = 0; int ret;
    struct nfqnl_msg_packet_hdr *ph;
    struct nfqnl_msg_packet_hw *hwph;
    u_int32_t mark,ifi;
    unsigned char *data;

    ph = nfq_get_msg_packet_hdr(tb);
    if (ph) {
        id = ntohl(ph->packet_id);
        printf("[ hw_protocol=0x%04x hook=%u id=%u ",
            ntohs(ph->hw_protocol), ph->hook, id);
    }

    hwph = nfq_get_packet_hw(tb);
    if (hwph) {
        int i, hlen = ntohs(hwph->hw_addrlen);

        printf("hw_src_addr=");
        for (i = 0; i < hlen-1; i++)
            printf("%02x:", hwph->hw_addr[i]);
        printf("%02x ", hwph->hw_addr[hlen-1]);
    }

    mark = nfq_get_nfmark(tb);
    if (mark)
        printf("mark=%u ", mark);
    ifi = nfq_get_indev(tb);
    if (ifi)
        printf("indev=%u ", ifi);
    ifi = nfq_get_outdev(tb);
    if (ifi)
        printf("outdev=%u ", ifi);
    ifi = nfq_get_physindev(tb);
    if (ifi)
        printf("physindev=%u ", ifi);
    ifi = nfq_get_physoutdev(tb);
    if (ifi)
        printf("physoutdev=%u ", ifi);
    printf("]\n");
    // ********* Print packets *********
    ret = nfq_get_payload(tb, &data);
    //dump(data, ret);

    // ********* Parsing packets *********
    int is_80 = 0; is_harmful = false;
    printf("[ Start parsing packet ]\n");
    printf("[ Total packet size : %d ]\n", ret);
    for (int i = 0; i < ret; i++) {
        if(i == 20){
            data += 20;
            is_80 = check_Port(data);
            if(is_80 == 0){ printf("{ Destination port is not 80 }\n"); break; }
            printf("[ TCP header size : %d ]\n", is_80);
            data -= 20;
        }
        if(i == (is_80 + 20)){ //printf("[ HTTP ]\n");
            char first = (char)(data[i]);
            //printf("first : %c\n", first);
            if(first == 'G' || first == 'P'){
                if(first == 'G'){ i += 4; } // 3 + 1(space)
                else if(first == 'P'){ i += 5; } // 4 + 1(space)
                int j = 0;
                while(1){
                    if((char)(data[j + i]) == '\r'){ i += (j + 2); break; }
                    j++;
                }

                char temp;
                char* buff = (char*)malloc(100); // DNS is not over 100 bytes.
                int a = 0;
                memset(buff, 0, 100);
                //printf("Second : %c\n", (char)(data[i]));
                if((char)(data[i]) == 'H'){ i += 6; // 5 + 1(space)
                    for(; a < 100; a++){
                        temp = (char)(data[a + i]);
                        //printf("temp : %c\n", temp);
                        if(temp == '\r'){ break; }
                        else{
                            buff[a] = temp;
                            //printf("buff[%d] : %c\n", a, buff[a]);
                        }
                    }
                }
                buff = (char*)realloc(buff, a);
                printf("[ HOST : %s ]\n", buff);
                is_harmful = is_Harmful_Web(buff);
                free(buff);
                break;
            }
        }
        else if(i > is_80 + 20) { break; }
    }

    //if (ret >= 0)
    //    printf("payload_len=%d ", ret);

    fputc('\n', stdout);

    return id;
}


static int cb(struct nfq_q_handle *qh, struct nfgenmsg *nfmsg,
          struct nfq_data *nfa, void *data)
{
    u_int32_t id = print_pkt(nfa);
    //printf("entering callback\n");
    // accept rules
    if(is_harmful == false){ return nfq_set_verdict(qh, id, NF_ACCEPT, 0, NULL); }
    // drop rules
    else if(is_harmful == true){ return nfq_set_verdict(qh, id, NF_DROP, 0, NULL); }
}

int main(int argc, char **argv)
{
    struct nfq_handle *h;
    struct nfq_q_handle *qh;
    struct nfnl_handle *nh;
    int fd;
    int rv;
    char buf[4096] __attribute__ ((aligned));

    // Add harmful websites
    // You have to set the variable, HW_MAX_LEN
    // The value of Current HW_MAX_LEN is 1
    harmful_website.push_back("test.gilgil.net");

    printf("[ opening library handle ]\n");
    h = nfq_open();
    if (!h) {
        fprintf(stderr, "{ error during nfq_open() }\n");
        exit(1);
    }

    printf("[ unbinding existing nf_queue handler for AF_INET (if any) ]\n");
    if (nfq_unbind_pf(h, AF_INET) < 0) {
        fprintf(stderr, "{ error during nfq_unbind_pf() }\n");
        exit(1);
    }

    printf("[ binding nfnetlink_queue as nf_queue handler for AF_INET ]\n");
    if (nfq_bind_pf(h, AF_INET) < 0) {
        fprintf(stderr, "{ error during nfq_bind_pf() }\n");
        exit(1);
    }

    printf("[ binding this socket to queue '0' ]\n");
    qh = nfq_create_queue(h,  0, &cb, NULL);
    if (!qh) {
        fprintf(stderr, "{ error during nfq_create_queue() }\n");
        exit(1);
    }

    printf("[ setting copy_packet mode ]\n");
    if (nfq_set_mode(qh, NFQNL_COPY_PACKET, 0xffff) < 0) {
        fprintf(stderr, "{ can't set packet_copy mode }\n");
        exit(1);
    }

    fd = nfq_fd(h);

    for (;;) {
        if ((rv = recv(fd, buf, sizeof(buf), 0)) >= 0) {
            printf("[ Packet received ]\n");
            nfq_handle_packet(h, buf, rv);
            continue;
        }
        /* if your application is too slow to digest the packets that
         * are sent from kernel-space, the socket buffer that we use
         * to enqueue packets may fill up returning ENOBUFS. Depending
         * on your application, this error may be ignored. nfq_nlmsg_verdict_putPlease, see
         * the doxygen documentation of this library on how to improve
         * this situation.
         */
        if (rv < 0 && errno == ENOBUFS) {
            printf("{ losing packets! }\n");
            continue;
        }
        perror("{ recv failed }");
        break;
    }

    printf("[ unbinding from queue 0 ]\n");
    nfq_destroy_queue(qh);

#ifdef INSANE
    /* normally, applications SHOULD NOT issue this command, since
     * it detaches other programs/sockets from AF_INET, too ! */
    printf("unbinding from AF_INET\n");
    nfq_unbind_pf(h, AF_INET);
#endif

    printf("[ closing library handle ]\n");
    nfq_close(h);

    exit(0);
}
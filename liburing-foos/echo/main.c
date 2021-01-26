// https://github.com/frevib/io_uring-echo-server/blob/master/io_uring_echo_server.c

#include <errno.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/socket.h>

#include <liburing.h>
#include "defs.h"

// buffer used in io_uring buffer selection
char buf[BUFFERS_COUNT][MAX_MESSAGE_LEN] = {0};
// buffer group ID used in io_uring buffer selection
int buffer_group_id = 1337;
// queue depth using in io_uring
unsigned int QD = 128;

int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("Please provide a port number: ./echo [port]\n");
        exit(1);
    }

    int port_num;
    struct sockaddr_in serv_addr, client_addr;
    socklen_t client_len;
    int sock_listen_fd;
    const int opt_val = 0x1;
    struct io_uring_params uring_params;
    struct io_uring ring;
    struct io_uring_probe *probe;
    struct io_uring_sqe *sqe;
    struct io_uring_cqe *cqe;

    port_num = strtol(argv[1], NULL, 10);
    if (!port_num) {
        printf("%s\n", "Please provide a valid port (0-65535)");
        exit(1);
    }

    client_len = sizeof(client_addr);
    // setup listening socket
    sock_listen_fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    setsockopt(sock_listen_fd, SOL_SOCKET, SO_REUSEADDR, &opt_val, sizeof(opt_val));
    setsockopt(sock_listen_fd, SOL_SOCKET, SO_REUSEPORT, &opt_val, sizeof(opt_val));

    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port_num);
    serv_addr.sin_addr.s_addr = INADDR_ANY;

    // bind and listen
    if (bind(sock_listen_fd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("Error binding socket...\n");
        exit(1);
    }
    if (listen(sock_listen_fd, BACKLOG) < 0) {
        perror("Error listening on socket...\n");
        exit(1);
    }
    printf("io_uring echo server listening for connections on port: %d\n", port_num);

    // initialize io_uring
    memset(&uring_params, 0, sizeof(uring_params));

    if (io_uring_queue_init_params(QD, &ring, &uring_params) < 0) {
        perror("io_uring_queue_init_params failed: \n");
        exit(1);
    }

    // check if IORING_FEAT_FAST_POLL is supported
    if (!(uring_params.features & IORING_FEAT_FAST_POLL)) {
        printf("IORING_FEAT_FAST_POLL is not available in the kernel, quiting...\n");
        exit(0);
    }

    // check if buffer selection is supported
    probe = io_uring_get_probe_ring(&ring);
    if (!probe ||
        !io_uring_opcode_supported(probe, IORING_OP_PROVIDE_BUFFERS) ||
        !io_uring_opcode_supported(probe, IORING_OP_REMOVE_BUFFERS)) {

        printf("Buffer selection is not supported, exiting...\n");
        exit(0);
    }
    free(probe);

    // register buffers for buffer selection
    sqe = io_uring_get_sqe(&ring);
    // initialise buffer selection
    io_uring_prep_provide_buffers(sqe, buf, MAX_MESSAGE_LEN, BUFFERS_COUNT, buffer_group_id, 0);

    io_uring_submit(&ring);
    io_uring_wait_cqe(&ring, &cqe);
    if (cqe->res < 0) {
        printf("cqe->res = %d\n", cqe->res);
        exit(1);
    }
    io_uring_cqe_seen(&ring, cqe);

    // add first accept SQE to monitor for new incoming connections
    add_socket_accept(&ring, sock_listen_fd, (struct sockaddr *)&client_addr, &client_len, 0);

    // start event loop
    while (1) {
        io_uring_submit_and_wait(&ring, 1);
        struct io_uring_cqe *ev_cqe;
        unsigned int head;
        unsigned int count = 0;

        // go through all CQEs
        io_uring_for_each_cqe(&ring, head, ev_cqe) {
            ++count;
            connection conn_i;
            memcpy(&conn_i, &ev_cqe->user_data, sizeof(conn_i));

            int type = conn_i.type;
            if (ev_cqe->res == -ENOBUFS) {
                fprintf(stdout, "buffer in automatic buffer selection empty, this should not happen...\n");
                fflush(stdout);
                exit(1);
            } else if (type == PROV_BUF) {
                if (ev_cqe->res < 0) {
                    printf("cqe->res = %d\n", ev_cqe->res);
                    exit(1);
                }
            } else if (type == ACCEPT) {
                int sock_conn_fd = ev_cqe->res;
                // only read when there is no error, >= 0
                if (sock_conn_fd >= 0) {
                    add_socket_read(&ring, sock_conn_fd, buffer_group_id, MAX_MESSAGE_LEN, IOSQE_BUFFER_SELECT);
                }
                // new connected client; read data from socket and re-add accept to monitor for new connections
                add_socket_accept(&ring, sock_listen_fd, (struct sockaddr *)&client_addr, &client_len, 0);
            } else if (type == READ) {
                int bytes_read = ev_cqe->res;
                if (ev_cqe->res <= 0) {
                    // de-initialise buffer selection
                    io_uring_prep_remove_buffers(sqe, 1, buffer_group_id);
                    // connection closed or error
                    if (shutdown(conn_i.fd, SHUT_RDWR) < 0) {
                        printf("shutdown failed");
                    }
                } else {
                    // bytes have been read into buf, now add write to socket sqe
                    int bid = ev_cqe->flags >> 16;
                    add_socket_write(&ring, conn_i.fd, bid, bytes_read, 0);
                }
            } else if (type == WRITE) {
                // write has been completed, first re-add the buffer
                add_provide_buffers(&ring, conn_i.bid, buffer_group_id);
                // add a new read for the existing connection
                add_socket_read(&ring, conn_i.fd, buffer_group_id, MAX_MESSAGE_LEN, IOSQE_BUFFER_SELECT);
            }
        }

        io_uring_cq_advance(&ring, count);
    }
}

static inline void add_socket_accept(struct io_uring *ring, int fd, struct sockaddr *client_addr, socklen_t *client_len, unsigned flags) {
    struct io_uring_sqe *sqe;

    sqe = io_uring_get_sqe(ring);
    io_uring_prep_accept(sqe, fd, client_addr, client_len, 0);
    io_uring_sqe_set_flags(sqe, flags);

    connection conn_i = {
        .fd = fd,
        .type = ACCEPT,
    };
    memcpy(&sqe->user_data, &conn_i, sizeof(conn_i));
}

static inline void add_socket_read(struct io_uring *ring, int fd, unsigned gid, size_t message_size, unsigned flags) {
    struct io_uring_sqe *sqe;

    sqe = io_uring_get_sqe(ring);
    io_uring_prep_recv(sqe, fd, NULL, message_size, 0);
    io_uring_sqe_set_flags(sqe, flags);
    sqe->buf_group = gid;

    connection conn_i = {
        .fd = fd,
        .type = READ,
    };
    memcpy(&sqe->user_data, &conn_i, sizeof(conn_i));
}

static inline void add_socket_write(struct io_uring *ring, int fd, unsigned short bid, size_t message_size, unsigned flags) {
    struct io_uring_sqe *sqe;

    sqe = io_uring_get_sqe(ring);
    io_uring_prep_send(sqe, fd, &buf[bid], message_size, 0);
    io_uring_sqe_set_flags(sqe, flags);

    connection conn_i = {
        .fd = fd,
        .type = WRITE,
        .bid = bid,
    };
    memcpy(&sqe->user_data, &conn_i, sizeof(conn_i));
}

static inline void add_provide_buffers(struct io_uring *ring, unsigned short bid, unsigned gid) {
    struct io_uring_sqe *sqe;

    sqe = io_uring_get_sqe(ring);
    io_uring_prep_provide_buffers(sqe, buf[bid], MAX_MESSAGE_LEN, 1, gid, bid);

    connection conn_i = {
        .fd = 0,
        .type = PROV_BUF,
    };
    memcpy(&sqe->user_data, &conn_i, sizeof(conn_i));
}

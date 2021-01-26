#define MAX_CONNECTIONS     4096
#define BACKLOG             20480
#define MAX_MESSAGE_LEN     2048
#define BUFFERS_COUNT       MAX_CONNECTIONS

static inline void add_socket_accept(struct io_uring *ring, int fd, struct sockaddr *client_addr, socklen_t *client_len, unsigned flags);
static inline void add_socket_read(struct io_uring *ring, int fd, unsigned gid, size_t size, unsigned flags);
static inline void add_socket_write(struct io_uring *ring, int fd, unsigned short bid, size_t size, unsigned flags);
static inline void add_provide_buffers(struct io_uring *ring, unsigned short bid, unsigned gid);

// io_uring states
enum {
    ACCEPT,
    READ,
    WRITE,
    PROV_BUF,
};

// connection context
typedef struct connection {
    unsigned int fd;
    unsigned short type;
    unsigned short bid;
} connection;

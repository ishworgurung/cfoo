struct request {
    int event_type;
    int iovec_count;
    int client_socket;
    struct iovec iov[];
};
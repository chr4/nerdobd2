int tcp_listen(int port);
void tcp_loop_accept(int s, void (*callback)(int));

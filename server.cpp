#include <iostream>
#include <cstring>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <unistd.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include <cerrno>
#include "epoll_reactor_event.h"

// global epoll fd
int global_efd;
// global MyEvent table
MyEvent global_events[MAX_EVENTS + 1];

void initlistensocket(int efd, unsigned short port);

void acceptconn(int lfd, int events, void *arg);

void recvdata(int fd, int events, void *arg);

void senddata(int fd, int events, void *arg);


int main(int argc, char *argv[]) {
    unsigned short port = SERV_PORT;

    if (argc == 2) {
        port = std::stoi(argv[1]);
    }

    global_efd = epoll_create(MAX_EVENTS + 1);
    if (global_efd <= 0) {
        printf("create efd in %s err %s\n", __func__, strerror(errno));
    }

    initlistensocket(global_efd, port);

    epoll_event ep_events[MAX_EVENTS + 1];
    printf("server running:port[%d]\n", port);

    int checkpos = 0, i;
    bool stop = false;
    while (!stop) {

        // time out check
        long now = time(nullptr); // current time
        for (i = 0; i < 100; ++i, checkpos++) { // check 100 connections every while loop
            if (checkpos == MAX_EVENTS) checkpos = 0;
            // if not in epoll rb tree
            if (global_events[checkpos].is_added_to_epoll != 1) continue;

            // non-active time for this client
            long duration = now - global_events[checkpos].last_active;

            if (duration >= 60) {
                close(global_events[checkpos].fd); // include <unistd.h>
                printf("[fd=%d] timeout\n", global_events[checkpos].fd);
                global_events[checkpos].delete_event(global_efd);
            }
        }

        // listen to epoll (rb tree) synchronously
        int ready_num = epoll_wait(global_efd, ep_events, MAX_EVENTS + 1, 1000);
        if (ready_num < 0) {
            printf("epoll_wait error, exit\n");
            stop = true;
            continue;
        }

        for (i = 0; i < ready_num; ++i) {
            auto *my_evt = reinterpret_cast<MyEvent *>(ep_events[i].data.ptr);

            // reactor
            if ((ep_events[i].events & EPOLLIN) && (my_evt->epoll_events & EPOLLIN)) {
                my_evt->call_back(my_evt->fd, (int) ep_events[i].events, my_evt->arg);
            }
            if ((ep_events[i].events & EPOLLOUT) && (my_evt->epoll_events & EPOLLOUT)) {
                my_evt->call_back(my_evt->fd, (int) ep_events[i].events, my_evt->arg);
            }
        }
    }

    return 0;
}


void initlistensocket(int efd, unsigned short port) {
    int listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    // set Non-block socket
    fcntl(listen_fd, F_SETFL, O_NONBLOCK);

    // register listen fd & event to the end position of global MyEvent table
    global_events[MAX_EVENTS].set_event(listen_fd, &acceptconn, &global_events[MAX_EVENTS]);
    // register listen fd & event to epoll
    global_events[MAX_EVENTS].add_event(efd, EPOLLIN);

    struct sockaddr_in sin{};
    bzero(&sin, sizeof(sin)); // std::memset(&sin, 0, sizeof(sin));
    sin.sin_family = AF_INET;
    sin.sin_addr.s_addr = INADDR_ANY;
    sin.sin_port = htons(port);

    listen(listen_fd, 20);
}

void acceptconn(int lfd, int events, void *arg) {
    struct sockaddr_in cin{};
    socklen_t len = sizeof(cin);
    int cfd, i; // client fd, index of MyEvents

    if ((cfd = accept(lfd, (struct sockaddr *) &cin, &len)) == -1) {
        if (errno != EAGAIN && errno != EINTR) {

        }
        printf("%s: accept, %s\n", __func__, strerror(errno));
        return;
    }

    do {
        // find the first available MyEvent
        for (i = 0; i < MAX_EVENTS; ++i) {
            if (!global_events[i].is_added_to_epoll) break;
        }

        // the last MyEvent is used for listen fd to accept new connections
        if (i == MAX_EVENTS) {
            printf("%s: max connection limit[%d]\n", __func__, MAX_EVENTS);
            break;
        }

        int flag = fcntl(cfd, F_SETFL, O_NONBLOCK);
        if (flag < 0) {
            printf("%s: fcntl nonblocking failed, %s\n", __func__, strerror(errno));
            break;
        }

        // register this new client fd to global MyEvent table
        global_events[i].set_event(cfd, &recvdata, &global_events[i]);
        // register event to epoll
        global_events[i].add_event(global_efd, EPOLLIN);

    } while (false);

    printf("new connection [%s:%d][time:%ld], pos[%d]\n",
           inet_ntoa(cin.sin_addr), ntohs(cin.sin_port), global_events[i].last_active, i);

}

void recvdata(int fd, int events, void *arg) {
    auto *my_evt = reinterpret_cast<MyEvent *>(arg);
    int len;

    len = (int) recv(fd, my_evt->buf, sizeof(my_evt->buf), 0);

    my_evt->delete_event(my_evt->fd);

    if (len > 0) {
        my_evt->len = len;
        my_evt->buf[len] = '\0';
        printf("C[%d]:%s\n", fd, my_evt->buf);

        // reactor: set fd to EPOLLOUT for sending out data later
        my_evt->set_event(fd, &senddata, my_evt);
        my_evt->add_event(global_efd, EPOLLOUT);
    } else if (len == 0) {
        close(my_evt->fd);
        // my_evt - global_events: address sub operation
        printf("[fd=%d] pos[%ld], closed\n", fd, my_evt - global_events);
    } else {
        close(my_evt->fd);
        // my_evt - global_events: address sub operation
        printf("recv[fd=%d] error[%d]:[%s], closed\n", fd, errno, strerror(errno));
    }
}

void senddata(int fd, int events, void *arg) {
    auto *my_evt = reinterpret_cast<MyEvent *>(arg);
    int len;

    len = (int) send(fd, my_evt->buf, my_evt->len, 0);

    if (len > 0) {
        printf("send[fd=%d], length[%d]: %s\n", fd, len, my_evt->buf);
        my_evt->delete_event(global_efd);

        // reactor: set fd to EPOLLIN for receiving data from client in the future
        my_evt->set_event(fd, &recvdata, my_evt);
        my_evt->add_event(global_efd, EPOLLIN);
    } else {
        close(my_evt->fd);
        my_evt->delete_event(global_efd);
        printf("send[fd=%d] error:[%s], closed\n", fd, strerror(errno));
    }
}
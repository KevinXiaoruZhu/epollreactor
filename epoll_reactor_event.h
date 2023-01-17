//
// Created by Kevin Xiaoru Zhu.
//

#ifndef EPOLLREACTOR_EPOLL_REACTOR_EVENT_H
#define EPOLLREACTOR_EPOLL_REACTOR_EVENT_H

const int MAX_EVENTS = 1024;
const int BUFLEN = 4096;
const int SERV_PORT = 8080;

class MyEvent {
public:
    MyEvent() = default;
    MyEvent(int fd, void (*call_back)(int, int, void*), void* arg);

    void set_event(int fd, void (*call_back)(int, int, void*), void* arg);

    void add_event(int efd, int e_evts);

    void delete_event(int efd);

public:
    int fd; // linux file descriptor
    int epoll_events; // enum EPOLL_EVENTS in epoll.h (EPOLLIN, EPOLLOUT ...)
    void *arg; // callback function arguments
    void (*call_back)(int fd, int events, void *arg); // callback function
    bool is_added_to_epoll; // 1: event is on the epoll rb-tree, 0: not on the rb-tree
    char buf[BUFLEN];
    int len; // buf length
    long last_active; // timestamp for the last active time
};





#endif //EPOLLREACTOR_EPOLL_REACTOR_EVENT_H

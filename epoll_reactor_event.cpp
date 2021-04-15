//
// Created by Kevin Xiaoru Zhu.
//

#include <iostream>
#include "epoll_reactor_event.h"
#include <sys/epoll.h>
#include <ctime>
#include <cstring>

MyEvent::MyEvent(int fd, void (*call_back)(int, int, void *), void *arg)
        : fd(fd), call_back(call_back), arg(arg) {
    this->events = 0;
    this->status = 0;
    this->len = 0;
    this->last_active = time(nullptr);
    std::memset(this->buf, 0, sizeof(this->buf));
}

void MyEvent::set_event(int fd, void (*call_back)(int, int, void *), void *arg) {
    this->fd = fd;
    this->call_back = call_back;
    this->arg = arg;
    this->events = 0;
    this->status = 0;
    this->len = 0;
    this->last_active = time(nullptr);
    std::memset(this->buf, 0, sizeof(this->buf));
}

void MyEvent::add_event(int efd, int evts) {
    struct epoll_event ep_evt = {0, {nullptr}};
    int op;
    ep_evt.data.ptr = this; // ptr -> MyEvent
    ep_evt.events = this->events = evts; // EPOLLIN or EPOLLOUT

    if (this->status == 1) {
        // my event was on rb tree
        op = EPOLL_CTL_MOD; // Modify its attributes
    } else {
        op = EPOLL_CTL_ADD;
        this->status = 1;
    }

    // epoll operation
    if (epoll_ctl(efd, op, this->fd, &ep_evt) < 0) {
        printf("event add failed [fd=%d], events[%d]\n", this->fd, evts);
    } else {
        printf("event added [fd=%d], op=%d, events[%0X]\n", this->fd, op, evts);
    }
}

void MyEvent::delete_event(int efd) {
    struct epoll_event ep_evt = {0, {nullptr}};

    if (this->status != 1) return;

    ep_evt.data.ptr = this;
    this->status = 0;
    epoll_ctl(efd, EPOLL_CTL_DEL, this->fd, &ep_evt);
}






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
    this->epoll_events = 0;
    this->is_added_to_epoll = 0;
    this->len = 0;
    this->last_active = time(nullptr);
    std::memset(this->buf, 0, sizeof(this->buf));
}

void MyEvent::set_event(int fd, void (*call_back)(int, int, void *), void *arg) {
    this->fd = fd;
    this->call_back = call_back;
    this->arg = arg;
    this->epoll_events = 0;
    this->is_added_to_epoll = false;
    this->len = 0;
    this->last_active = time(nullptr);
    std::memset(this->buf, 0, sizeof(this->buf));
}

void MyEvent::add_event(int efd, int e_evts) {
    struct epoll_event ep_evt = {0, {nullptr}};
    int op;
    ep_evt.data.ptr = this; // ptr -> MyEvent
    ep_evt.events = this->epoll_events = e_evts; // epoll events: EPOLLIN or EPOLLOUT

    if (this->is_added_to_epoll) {
        // MyEvent was on rb tree
        op = EPOLL_CTL_MOD; // modify its attributes
    } else {
        op = EPOLL_CTL_ADD; // will add new epoll event
        this->is_added_to_epoll = true;
    }

    // epoll operation
    if (epoll_ctl(efd, op, this->fd, &ep_evt) < 0) {
        printf("event add failed [fd=%d], epoll_events[%d]\n", this->fd, e_evts);
    } else {
        printf("event added [fd=%d], op=%d, epoll_events[%0X]\n", this->fd, op, e_evts);
    }
}

void MyEvent::delete_event(int efd) {
    struct epoll_event ep_evt = {0, {nullptr}};

    if (this->is_added_to_epoll != 1) return;

    ep_evt.data.ptr = this;
    this->is_added_to_epoll = false;
    epoll_ctl(efd, EPOLL_CTL_DEL, this->fd, &ep_evt);
}






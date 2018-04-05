#include "epoller.hh"

#include <unistd.h>

#include <iostream>
#include "exception.hh"

using namespace std;

Epoller::Epoller()
  : epoller_fd_(CheckSystemCall("epoll_create1", epoll_create1(EPOLL_CLOEXEC))),
    callback_table_()
{}

Epoller::~Epoller()
{
  if (close(epoller_fd_) < 0) {
    cerr << "Epoller: failed to close epoll instance " << epoller_fd_ << endl;
  }
}

inline void Epoller::epoll_control(const int op,
                                   FileDescriptor & fd, const uint32_t events)
{
  struct epoll_event ev;
  ev.data.fd = fd.fd_num();
  ev.events = events;
  CheckSystemCall("epoll_ctl", epoll_ctl(epoller_fd_, op, fd.fd_num(), &ev));
}

void Epoller::add_events(FileDescriptor & fd, const uint32_t events)
{
  if (events == 0) {
    throw runtime_error("Epoller::add_events: empty events");
  }

  epoll_control(EPOLL_CTL_ADD, fd, events);

  /* attach this to fd */
  fd.attach_epoller(shared_from_this());
}

void Epoller::modify_events(FileDescriptor & fd, const uint32_t events)
{
  if (events == 0) {
    throw runtime_error("Epoller::modify_events: empty events; "
                        "use Epoller::deregister instead");
  }

  epoll_control(EPOLL_CTL_MOD, fd, events);
}

void Epoller::set_callback(FileDescriptor & fd, const uint32_t event,
                           const callback_t & callback)
{
  callback_table_[fd.fd_num()][event] = callback;
}

void Epoller::deregister(FileDescriptor & fd)
{
  /* deregister fd from epoll instance */
  CheckSystemCall("epoll_ctl",
                  epoll_ctl(epoller_fd_, EPOLL_CTL_DEL, fd.fd_num(), nullptr));

  /* clear associated callback functions */
  callback_table_.erase(fd.fd_num());

  fd.detach_epoller(shared_from_this());
}

int Epoller::poll(const int timeout_ms)
{
  int nfds = CheckSystemCall("epoll_wait",
      epoll_wait(epoller_fd_, event_list_, sizeof(event_list_), timeout_ms));

  for (int i = 0; i < nfds; i++) {
    int fd = event_list_[i].data.fd;
    uint32_t revents = event_list_[i].events;

    auto it = callback_table_.find(fd);
    if (it == callback_table_.end()) {
      throw runtime_error("Epoller::poll: fd " + to_string(fd) + " does not "
                          "have any callbacks");
    }

    for (const auto & [event, callback] : it->second) {
      if (revents & event) {
        /* callback returns error */
        if (callback() < 0) {
          throw runtime_error("Epoller::poll: callback of fd " + to_string(fd)
                              + " returns error");
        }
      }
    }
  }

  return nfds;
}

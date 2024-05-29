
// read_thread.h

#ifndef BLUETOOTH_READ_THREAD_H
#define BLUETOOTH_READ_THREAD_H

#include <boost/asio.hpp>
#include <memory>
#include "../bluetooth/bluetooth_adapter.h"
#include "read_thread.h"

class BluetoothReadThread: public ReadThread {
public:

    virtual void init() override ;

    virtual void startComplexRequests(const std::vector<read_thread_ns::ReplayRequest>& requests, int repeatCount) override ;

    virtual int outputWrite(int fd, const void *buf, size_t count) override ;

    virtual int outputOpen(const char* path, int flags) override ;

    virtual int outputClose(int fd) override;
private:
    std::shared_ptr<BluetoothAdapter> adapter;
};

#endif // READ_THREAD_H


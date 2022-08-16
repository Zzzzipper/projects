#include <boost/asio/signal_set.hpp>
#include <boost/asio/io_service.hpp>
#include <boost/thread.hpp>
#include <iostream>

#include "system.h"

using boost::thread;
using namespace tester;

void handler( const boost::system::error_code& error , int signal_number )
{
    std::cout << "handling signal " << signal_number << std::endl;
    exit(1);
}

int main()
{
    System s;
    boost::asio::io_service io_service;

    thread t([&s] {
        for(;;) {
            std::cout << "Tot.Virt.:" << s.totalVirtualMemory() << ", "
                      << "Virt.Used:" << s.currentlyUsedVirtualMemory() << ", "
                      << "Virt.Used.Cur.Proc:" << s.currentProcessVirtualMemoryUsed() << ", "
                      << "Tot.Ph.:" << s.totalPhisicalMemory() << ", "
                      << "Ph.Used:" << s.currentlyUsedPhisicalMemory() << ", "
                      << "Ph.Used.Cur.Proc:" << s.currentProcessPhisicalMemoryUsed() << ", "
                      << "Mem.Data.Used.Cur.Proc:" << s.currentProcessMemoryDataUsed() << ", "
                      << "CPU%.:" << s.cpuCurrentlyUsed() << ", "
                      << "CPU cur.proc.%.:" << s.cpuCurrentlyUsedByCurrentProcess() << std::endl;
            std::cout << ".." << std::endl;
//            auto b = new int[1024];
            boost::this_thread::sleep(boost::posix_time::milliseconds(10));
        }
    });


    std::cout << "service run" << std::endl;

    // Construct a signal set registered for process termination.
    boost::asio::signal_set signals(io_service, SIGINT );
    // Start an asynchronous wait for one of the signals to occur.
    signals.async_wait( handler );

    io_service.run();

    t.join();

    return 0;
}

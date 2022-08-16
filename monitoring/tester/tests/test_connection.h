#include <stdio.h>
#include <stdlib.h>

#include <string>
#include <set>
#include <vector>
#include <exception>
#include <iostream>

#include <boost/foreach.hpp>
#include <boost/bind.hpp>
#include <boost/function.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/xml_parser.hpp>

#include <boost/shared_ptr.hpp>
#include <boost/weak_ptr.hpp>

#include <boost/thread/thread.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/asio.hpp>

class TestObj: boost::noncopyable
{
private:

public:
	boost::thread thread1_;
	boost::weak_ptr<int> user_;

    //void runThread1(TestObj1 *in);
    void runThread1();
};

typedef const char *(FuncT)(const char *);

class demo_class
{
private:
    typedef boost::function<FuncT> func_t;
    func_t func;
    
public:
    void accept(func_t f) {
        func = f;
    }
    
    void run() {
        func("james");
    }
};

class call_back_factory
{
public:
    const char *call_back_func1(const char *in) {
        std::cout << "call_back_func1:" << in << std::endl;
        
        return in;
    }
    
    const char *call_back_func2(const char *in, int in2) {
        std::cout << "call_back_func2:" << in << std::endl;
        
        return in;
    }
};

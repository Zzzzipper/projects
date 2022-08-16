#include <iostream>


#include "test_tuple.h"
#include "boost/typeof/typeof.hpp"
#include "boost/assign.hpp"
#include "boost/tuple/tuple.hpp"
#include "boost/tuple/tuple_io.hpp"

using namespace std;

boost::tuple<int, std::string> tup_func(){
    return boost::make_tuple(1, "hello world");
}

template <typename T>
void print_tuple(const T& t){
//    cout << t.get_head() << ",";
//    print_tuple(t.get_tail());
}
void print_tuple(const boost::tuples::null_type& t){}

void test_tuple_example()
{
    boost::tuple<int, double> tp;
    tp = boost::make_tuple(1, 100.00);
    assert(tp.get<0>() == 1);
    cout << tp.get<0>() << endl; // 1
    cout << tp.get<1>() << endl; // 100
    cout << boost::get<0>(tp) << endl; // 1

    typedef boost::tuple<int, double, std::string> idsType;
    idsType tp2 = boost::make_tuple(1, 2.0, "hello world");
    cout << tp2 << endl; // (1 2 hello world)
    cout << boost::tuples::set_open('[') << boost::tuples::set_close(']') << boost::tuples::set_delimiter(',');
    cout << tp2 << endl; // [1,2,hello world]

    int i; std::string str;
    boost::tie(i, str) = tup_func();
    cout << i << ", " << str << endl; // 1, hello world
    int j;
    boost::tie(j, boost::tuples::ignore) = tup_func();
    print_tuple(tp2); // 1,2,hello world,
    cout << endl;

    typedef boost::tuple<int, std::string> tisType;
    assert(typeid(int) == typeid(boost::tuples::element<0, tisType>::type));
}

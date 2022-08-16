#pragma once

#include <map>
#include <string>
#include <iostream>
#include <boost/bind.hpp>
#include <boost/function.hpp>
#include <boost/container/vector.hpp>

#include "record.h"

namespace bip=boost::interprocess;

namespace tester {

    static const char* operation_mutex = "7FD6D7E8-320B-11AC-82CF-F0C533D71593";

    struct request;
    struct response;

    enum Result { Success = 0, Failed = 4 };
    enum Operation { INSERT = 0, UPDATE, DELETE, GET};

    class Processor {
    public:
        Processor(bip::managed_mapped_file& seg);
        response exec(request& r);
        std::ostringstream echoStatus();

    private:
        void incSuccess(Operation op);
        void incFailed(Operation op);
        bip::managed_mapped_file& seg_;

        record_container* records_;

        typedef std::map<std::string,
            boost::function<response(std::string& key, std::string& value)>> command_map;
        command_map procMap_;

        bip::named_mutex mutex_;

        typedef boost::interprocess::allocator<unsigned long,
        bip::managed_mapped_file::segment_manager> ShmemAllocator;

        typedef boost::container::vector<unsigned long, ShmemAllocator> StatArray;
        StatArray* s_;
    };

}

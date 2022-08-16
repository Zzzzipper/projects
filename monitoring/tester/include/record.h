#pragma once

#if !defined(NDEBUG)
#define BOOST_MULTI_INDEX_ENABLE_INVARIANT_CHECKING
#define BOOST_MULTI_INDEX_ENABLE_SAFE_MODE
#endif

#include <iostream>
#include <iterator>
#include <sstream>
#include <string>

#include <boost/config.hpp>
#include <algorithm>
#include <boost/multi_index_container.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index/member.hpp>
#include <boost/interprocess/allocators/allocator.hpp>
#include <boost/interprocess/containers/string.hpp>
#include <boost/interprocess/managed_mapped_file.hpp>
#include <boost/interprocess/sync/named_mutex.hpp>
#include <boost/interprocess/sync/scoped_lock.hpp>

using boost::multi_index_container;
using namespace boost::multi_index;
namespace bip=boost::interprocess;

/*
 * shared_string is a string type placeable in shared memory,
 * courtesy of Boost.Interprocess.
 */

typedef bip::basic_string<char,std::char_traits<char>,
bip::allocator<char,bip::managed_mapped_file::segment_manager>> shared_string;

/*
 * Record. All its members can be placed in shared memory,
 * hence the structure itself can too.
 */

struct record
{
    shared_string key;
    shared_string value;

    record(const shared_string::allocator_type& al):
        key(al), value(al)
    {}

    friend std::ostream& operator<<(std::ostream& os,const record& b) {
        os << "\""  << b.key << "\": \"" << b.value <<"\"" << std::endl;
        return os;
    }
};


/*
 * Define a multi_index_container of records with indices on
 * key and value. This container can be placed in shared memory because:
 *   * record can be placed in shared memory.
 *   * We are using a Boost.Interprocess specific allocator.
 */

/*
 * see Compiler specifics: Use of member_offset for info on
 * BOOST_MULTI_INDEX_MEMBER
 */

typedef multi_index_container<record,
    indexed_by<
    ordered_unique<BOOST_MULTI_INDEX_MEMBER(record, shared_string, key)>,
    ordered_non_unique<BOOST_MULTI_INDEX_MEMBER(record, shared_string, value)>>,
    bip::allocator<record, bip::managed_mapped_file::segment_manager>> record_container;



#include <iostream>
#include <string>

#include "test_string_algo.h"
#include "boost/format.hpp"
#include "boost/typeof/typeof.hpp"
#include "boost/algorithm/string.hpp"
#include "boost/assign.hpp"

using namespace std;

void test_string_algo()
{
    string str("readme.txt");
    if (boost::ends_with(str, "txt"))
    {
        cout << boost::to_upper_copy(str) << endl;
        assert(boost::ends_with(str, "txt"));
    }

    boost::replace_first(str, "readme", "followme");
    cout << str << endl;

    std::vector<char> v(str.begin(), str.end());
    std::vector<char> v2(boost::to_upper_copy(boost::erase_first_copy(v, "txt")));
    for (size_t i = 0; i < v2.size(); ++i)
    {
        cout << v2[i];
    }
    cout << endl;


    // begin with i , not check case sentive
    // copy end, return copy
    // boost::to_upper  boost::to_upper_copy
    std::string strTest("I Known");
    cout << boost::to_upper_copy(strTest) << endl; // I KNOWN
    cout << strTest << endl; // I Known
    boost::to_lower(strTest);
    cout << strTest << endl; // i known
    cout << boost::istarts_with(strTest, "I") << endl; // 1
    cout << boost::starts_with(strTest, "I") << endl; // 0
    // starts_with ends_with contains equals lexicographical_compare(check if str1 < str2 by sequence of zidian)
    // all
    cout << boost::all(strTest.substr(2,5), boost::is_lower()) << endl; // 1

    std::string str1("Hello");
    std::string	str2("hello");
    assert(!boost::is_equal()(str1, str2));

    /*
    ** trim
    */
    std::string strFmt(" hello world! ");
    cout << boost::trim_copy(strFmt) << endl; // hello world!
    cout << boost::trim_left_copy(strFmt) << endl; // hello world!
    cout << boost::trim_right_copy(strFmt) << endl; // hello world!

    std::string strFmt2("2017  hello world  123!!!");
    cout << boost::trim_copy_if(strFmt2, boost::is_digit()) << endl; // hello world  123!!!
    cout << boost::trim_left_copy_if(strFmt2, boost::is_digit()) << endl; // hello world  123!!!
    cout << boost::trim_right_copy_if(strFmt2, boost::is_punct()) << endl; // 2017  hello world  123
    cout << boost::trim_copy_if(strFmt2, boost::is_punct() || boost::is_digit()) << endl; // hello world

    /*
    ** find
    */
    boost::format fmt("|%s|.pos = %d\n");
    std::string strrge("Long long ago, there was a king.");
    boost::iterator_range<std::string::iterator> rge;
    rge = boost::find_first(strrge, "long");
    cout << fmt % rge % (rge.begin() - strrge.begin()); // |long|.pos = 5
    rge = boost::ifind_first(strrge, "long");
    cout << fmt % rge % (rge.begin() - strrge.begin()); // |Long|.pos = 0
    rge = boost::find_nth(strrge, "ng", 2); // 3th
    cout << fmt % rge % (rge.begin() - strrge.begin()); // |ng|.pos = 29
    rge = boost::find_head(strrge, 4);
    cout << fmt % rge % (rge.begin() - strrge.begin()); // |Long|.pos = 0
    rge = boost::find_tail(strrge, 5);
    cout << fmt % rge % (rge.begin() - strrge.begin()); // |king.|.pos = 27
    rge = boost::find_tail(strrge, 5);
    cout << fmt % rge % (rge.begin() - strrge.begin()); // |king.|.pos = 27
    rge = boost::find_first(strrge, "tantaijn");
    assert(rge.empty() && !rge);

    /*
    ** replace and erase, like find
    */
    //boost::replace_first boost::replace_first_copy
    //boost::erase_head boost::erase_head_copy

    /*
    ** ifind_all  split
    */
    std::string strFindAll("Samus,Link.Zelda::Mario-Luii+zelda");
    std::deque<std::string> d;
    boost::ifind_all(d, strFindAll, "ZELda");
    for (BOOST_AUTO(pos, d.begin()); pos != d.end(); ++pos)
    {
        cout << "[" << *pos << "]" << endl;
    }

    std::list<boost::iterator_range<std::string::iterator>> l;
    boost::split(l, strFindAll, boost::is_any_of(",.:-+"));
    for (BOOST_AUTO(pos, l.begin()); pos != l.end(); ++pos)
    {
        cout << "[" << *pos << "]";
    }
    cout << endl;

    l.clear();
    boost::split(l, strFindAll, boost::is_any_of(",.:-+"), boost::token_compress_on); // token_compress_on, ingore two char same together
    for (BOOST_AUTO(pos, l.begin()); pos != l.end(); ++pos)
    {
        cout << "[" << *pos << "]";
    }
    cout << endl;

    /*
    ** join join_if
    */
    std::vector<std::string> v3 = boost::assign::list_of("Samus")("Link")("Zelda")("Mario")("Luii")("zelda");
    cout << boost::join(v3, "+") << endl;
    struct is_contains_a{
        bool operator()(const std::string& x){
            return boost::contains(x, "a");
        }
    };
    cout << boost::join_if(v3, "+", is_contains_a()) << endl;

    /*
    ** boost::find_iterator boost::split_iterator
    */
    std::string str3("Samus,Link,Zelda,Mario-Luii+zelda");
    typedef boost::find_iterator<std::string::iterator> string_find_iter;
    string_find_iter spos, send;
    for (spos = boost::make_find_iterator(str3, boost::first_finder("Samus", boost::is_equal())); spos != send; ++spos )
    {
        cout << "[" << *spos << "]";
    }
    cout << endl;
    typedef boost::split_iterator<std::string::iterator> string_split_iter;
    string_split_iter sppos, spend;
    for (sppos = boost::make_split_iterator(str3, boost::first_finder(",", boost::is_equal())); sppos != spend; ++sppos)
    {
        cout << "[" << *sppos << "]";
    }
    cout << endl;
}

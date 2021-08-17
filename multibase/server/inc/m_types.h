/**
 * Message type entities
 */

#pragma once

#include <iostream>
#include <boost/fusion/adapted/struct/adapt_struct.hpp>
#include <boost/fusion/include/adapt_struct.hpp>

// parser
#include <boost/config/warning_disable.hpp>
#include <boost/spirit/include/qi.hpp>
#include <boost/spirit/include/phoenix.hpp>
#include <boost/spirit/include/phoenix_core.hpp>
#include <boost/spirit/include/phoenix_operator.hpp>
#include <boost/spirit/include/phoenix_object.hpp>
#include <boost/fusion/include/adapt_struct.hpp>
#include <boost/fusion/include/io.hpp>

namespace multibase {

    enum ExecResponseCode {
        ExecSuccess = 0x0,
        ArgsError,
        AppError,
        InsertFailedByKeyPresent,
        UpdateFailedByKeyNotPresent,
        UpdateFailedByKeyValueIsPresent,
        DeleteFailedByKeyNotPresent,
        GetFailedByKeyNotPresent,
        InvalidRequestCommand
    };

    struct request {
        std::string command;
        std::string key;
        std::string value;
    };

    struct response {
        ExecResponseCode code;
        std::string what;
    };

} // ! multibase

BOOST_FUSION_ADAPT_STRUCT(
        multibase::request,
        (std::string, command)
        (std::string, key)
        (std::string, value)
        );

namespace qi = boost::spirit::qi;
namespace ascii = boost::spirit::ascii;


template <typename Iterator>
struct request_parser : qi::grammar<Iterator, multibase::request(), ascii::space_type>
{
    request_parser() : request_parser::base_type(start)
    {
        using namespace qi;

        quoted_string %= lexeme['"' >> +(char_ - '"') >> '"'];
        prop_key %= lexeme ['"' >> lazy(_r1) >> '"'] >> ':';

        start %= lit("{") >> prop_key(+"command")  >> quoted_string >> ','
                          >> prop_key(+"key") >> quoted_string >> ','
                          >> prop_key(+"value") >> quoted_string
                          >> '}';

    }

    qi::rule<Iterator, void(const char*), ascii::space_type> prop_key;
    qi::rule<Iterator, std::string(), ascii::space_type> quoted_string;
    qi::rule<Iterator, multibase::request(), ascii::space_type> start;

};

BOOST_FUSION_ADAPT_STRUCT(
        multibase::response,
        (multibase::ExecResponseCode, code)
        (std::string, what)
        );

template <typename Iterator>
struct response_parser : qi::grammar<Iterator, multibase::response(), ascii::space_type>
{
    response_parser() : response_parser::base_type(start)
    {
        using namespace qi;

        quoted_string %= lexeme['"' >> +(char_ - '"') >> '"'];
        prop_key %= lexeme ['"' >> lazy(_r1) >> '"'] >> ':';

        start %= lit("{") >> prop_key(+"code")  >> int_ >> ','
                          >> prop_key(+"what") >> quoted_string
                          >> '}';

    }

    qi::rule<Iterator, void(const char*), ascii::space_type> prop_key;
    qi::rule<Iterator, std::string(), ascii::space_type> quoted_string;
    qi::rule<Iterator, multibase::response(), ascii::space_type> start;

};

inline const char* errorBody(multibase::ExecResponseCode code_) {
    switch(code_) {
    case multibase::ExecSuccess:
        return "Success..";
    case multibase::InsertFailedByKeyPresent:
        return "Insertion is denied because the key already exists..";
    case multibase::UpdateFailedByKeyNotPresent:
        return "The update is incomplete because the key was not found..";
    case multibase::UpdateFailedByKeyValueIsPresent:
        return "The update is not completed because the key value matches..";
    case multibase::DeleteFailedByKeyNotPresent:
        return "Deletion not completed because the key was not found..";
    case multibase::GetFailedByKeyNotPresent:
        return "Value by key not found..";
    case multibase::InvalidRequestCommand:
        return "Invalid command..";
    case multibase::ArgsError:
        return "Failed set arguments..";
    case multibase::AppError:
        return "Application error..";
    }
    return "Unknown what..";
}

#pragma once

#include <iostream>
#include <sstream>

#include <boost/type_traits.hpp>                // is_array, is_class, remove_bounds
#include <boost/convert/detail/is_string.hpp>   // is_class

#include <boost/mpl/eval_if.hpp>
#include <boost/mpl/identity.hpp>
#include <boost/mpl/next_prior.hpp>

#include <boost/fusion/mpl.hpp>
#include <boost/fusion/adapted.hpp>             // BOOST_FUSION_ADAPT_STRUCT

// boost::fusion::result_of::value_at
#include <boost/fusion/sequence/intrinsic/value_at.hpp>
#include <boost/fusion/include/value_at.hpp>

// boost::fusion::result_of::size
#include <boost/fusion/sequence/intrinsic/size.hpp>
#include <boost/fusion/include/size.hpp>

// boost::fusion::at
#include <boost/fusion/sequence/intrinsic/at.hpp>
#include <boost/fusion/include/at.hpp>

namespace json {

    // Forward
    template < typename T >
    struct serializer;

    namespace detail {

        namespace iterator {

            template < typename S, typename N >
            struct Comma
            {
                template < typename Ostream >
                static inline void comma(Ostream& os)
                {
                    os << ", ";
                }
            };

            template < typename S >
            struct Comma< S, typename boost::mpl::prior< typename boost::fusion::result_of::size< S >::type >::type >
            {
                template < typename Ostream >
                static inline void comma(Ostream& os)
                {
                    (void)os;
                }
            };

            // Iteration on a structure
            template < typename S, typename N >
            struct StructImpl
            {
                // Current field type
                typedef typename boost::fusion::result_of::value_at< S, N >::type current_t;
                typedef typename boost::mpl::next< N >::type next_t;
                typedef boost::fusion::extension::struct_member_name< S, N::value > name_t;

                template < typename Ostream >
                static inline void serialize(Ostream& os, const S& s)
                {
                    os << "\"" << name_t::call() << "\": ";
                    ::json::serializer< current_t >::serialize(os, boost::fusion::at< N >(s));

                    // Insert comma or not
                    Comma< S, N >::comma(os);

                    StructImpl< S, next_t >::serialize(os, s);
                }
            };

            // End of iteration on structures.
            template < typename S >
            struct StructImpl< S, typename boost::fusion::result_of::size< S >::type >
            {
                template < typename Ostream >
                static inline void serialize(Ostream& os, const S& s)
                {
                    (void)os;
                    (void)s;
                }
            };

            // Iterator on a structure. Facade template.
            template < typename S >
            struct Struct : StructImpl< S, boost::mpl::int_< 0 > > {};

        } // iterator

        template < typename T >
        struct string_serializer
        {
            typedef string_serializer< T > type;

            template < typename Ostream >
            static inline void serialize(Ostream& os, const T& t)
            {
                os << "\"" << t << "\"";
            }

        };

        template < typename T >
        struct array_serializer
        {
            typedef array_serializer< T > type;

            typedef typename boost::remove_bounds< T >::type slice_t;

            static const size_t size = sizeof(T) / sizeof(slice_t);

            template < typename Ostream >
            static inline void serialize(Ostream& os, const T& t)
            {
                os << "[";
                for(size_t idx=0; idx<size; idx++)
                {
                    ::json::serializer< slice_t >::serialize(os, t[idx]);
                    if (idx != size-1)
                        os << ", ";
                }
                os << "]";
            }

        };

        template < typename T >
        struct struct_serializer
        {
            typedef struct_serializer< T > type;

            template < typename Ostream >
            static inline void serialize(Ostream& os, const T& t)
            {
                os << "{";
                iterator::Struct< T >::serialize(os, t);
                os << "}";
            }
        };

        template < typename T >
        struct arithmetic_serializer
        {
            typedef arithmetic_serializer< T > type;

            template < typename Ostream >
            static inline void serialize(Ostream& os, const T& t)
            {
                os << t;
            }
        };

        template < typename T >
        struct calculate_serializer
        {
            typedef
            typename boost::mpl::eval_if<
            boost::cnv::is_string< T >, boost::mpl::identity< string_serializer < T > >,
            // else
            typename boost::mpl::eval_if< boost::is_array< T >, boost::mpl::identity< array_serializer < T > >,
            //else
            typename boost::mpl::eval_if< boost::is_class< T >,
            boost::mpl::identity< struct_serializer < T > >,
            //else
            boost::mpl::identity< arithmetic_serializer < T > >
            >>
            >::type type;

        };

    } // detail

    template < typename T >
    struct serializer : public detail::calculate_serializer < T >::type
    {
    };


} // !json


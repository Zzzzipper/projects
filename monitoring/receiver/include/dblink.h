#pragma once

#include <iostream>
#include <boost/move/unique_ptr.hpp>
#include <pqxx/pqxx>
#include <map>

namespace  receiver {

class DbLink{
public:
    DbLink() = default;
    DbLink(const char* connectionString);
    DbLink(const DbLink&);

    // Debug print
    template<class Elem, class Traits>
    void print(std::basic_ostream<Elem, Traits>& aStream) {

    }

    /// Connecting to database server
    bool connect();

    /// Get function execution result as vector strings
    template<typename ...Args>
    std::vector<std::string> execFunc(const std::string &query, Args &&...args) {
        pqxx::work transaction(*_connection);
        pqxx::result result(transaction.exec_params(query, args...));
        transaction.commit();
        std::vector<std::string> output(result.size());
        size_t i = 0;
        for(auto row: result)  {
            output[i] = row[0].as<std::string>();
            ++i;
        }
        return output;
    }

    std::string dbAlias();

private:
    std::vector<std::string> _explode(const std::string& str, const char& ch);

    std::string _connectionString;
    std::string _dbAlias;
    std::shared_ptr<pqxx::connection> _connection;
};


class DbLinkContainer {
public:
    DbLinkContainer(const DbLinkContainer&) = delete;
    DbLinkContainer& operator=(const DbLinkContainer &) = delete;
    DbLinkContainer(DbLinkContainer &&) = delete;
    DbLinkContainer & operator=(DbLinkContainer &&) = delete;

    static auto& instance(){
        static DbLinkContainer container;
        return container;
    }

    bool initialize(std::string configPath);

    const std::map<uint16_t, DbLink>& container();

private:
    DbLinkContainer() = default;
    std::map<uint16_t, DbLink> _container;
};

}

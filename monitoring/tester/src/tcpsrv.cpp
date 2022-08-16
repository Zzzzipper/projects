#include <sstream>
#include <iostream>

#include "tcpsrv.h"
#include "processor.h"
#include "json.h"
#include "m_types.h"
#include "handshake.h"
#include "hex_dump.h"
#include "utils.h"
#ifdef RECEIVER_SIDE
#include "dblink.h"
#include "tester.h"
#include "dataflow.h"
#endif

static const char* _tag = "[LISTENER] ";

namespace tester {

/**
 * @brief session::session
 * @param socket
 */
Session::Session(tcp::socket socket,
                 SessionManager& manager, Dispatcher& dispatcher)
    : _socket(std::move(socket)),
      _session_manager(manager),
      _dispatcher(dispatcher),
      _isAuthorized(false),
      _isStopped(false),
      _needToRead(1UL),
      _received(0UL),
      _responseTick(0)
{
}

Session::~Session() {
    LOG_TRACE << _tag << "Destruct session success..";
}

/**
 * @brief session::start
 */
void Session::start() {
    LOG_TRACE << _tag << "Start session..";
    _responseTimer = std::thread([&](){
        while(_responseTick < BREAK_CONN_INTERVAL) {
            std::this_thread::sleep_for(std::chrono::seconds(TICK_INTERVAL_SEC));
            LOG_TRACE << _tag << "...fire tick connection timer";
            _responseTick++;
        }
        if(!_isStopped) {
            _sendStatus(SOCK_RC_TIMEOUT);
            _session_manager.stop(shared_from_this());
            return;
        }
    });
    _doRead();
}

void Session::stop()
{
    LOG_TRACE << _tag << "Stop session..";
    _isStopped = true;
    if(_responseTick < BREAK_CONN_INTERVAL) {
        _responseTick = BREAK_CONN_INTERVAL;
        _responseTimer.join();
    } else {
        _responseTimer.detach();
    }
    _socket.close();
}

/**
 * @brief Session::_sendStatus - Perfom send status
 * @param status
 */
void Session::_sendStatus(uint32_t status) {
    std::vector<uint8_t> buffer(sizeof(uint32_t));
    memcpy(buffer.data(), &status, sizeof(uint32_t));
    _doWrite(buffer);
}

/**
 * @brief session::doRead
 */
void Session::_doRead() {
    auto self(shared_from_this());
    _socket.async_read_some(boost::asio::buffer(_buffer),
                            [&, self](boost::system::error_code ec, std::size_t bytes_transferred)
    {
        LOG_TRACE << _tag
                  << "Read buffer, bytes_transferred:" << bytes_transferred;
        if(bytes_transferred) {
            dash::hex_dump(std::string(reinterpret_cast<char*>(_buffer.data()), bytes_transferred),
                           LOG_TRACE << _tag);
        }
        if (!ec) {

            _packet.insert(_packet.end(), _buffer.begin(), _buffer.begin() + bytes_transferred);

            if(!_isAuthorized && bytes_transferred == DH_PUBLIC_KEY_LENGTH) {
                Handshake handshake;
                if(!handshake.fromClient(_socket, _packet)) {
                    _sendStatus(SOCK_RC_AUTH_FAILURE);
                    _session_manager.stop(shared_from_this());
                    return;
                }

                _responseTick = 0;

                _isAuthorized = true;
                _packet.clear();
            } else if(_isAuthorized && _received < _needToRead) {
                if(_received == 0 && bytes_transferred > 0 && bytes_transferred >= sizeof(ST_SIZES)) {
                    ST_SIZES sizes;
                    memcpy(&sizes, _buffer.data(), sizeof(ST_SIZES));
                    _needToRead = ((sizes.CmprSize != 0)? sizes.CmprSize: sizes.UncmprSize) +
                            sizeof(ST_SIZES);
                }
                _received += bytes_transferred;
            }

            if(_isAuthorized && _received == _needToRead) {
                ST_T_HDR hdr;

                _responseTick = 0;

                memcpy(&hdr, _packet.data() + sizeof(ST_SIZES), sizeof(ST_T_HDR));

#ifdef RECEIVER_SIDE

                auto testerIpAddr = _socket.remote_endpoint().address().to_string();
                receiver::TesterContainer::instance().
                        accept(hdr.TesterId, Config::instance()["reReadPeriod"].ToInt() * 2,
                        testerIpAddr);

                auto numericIpAdd = ipStringToNumeric(testerIpAddr);

                LOG_TRACE << _tag << "Numeric tester IP address is " << numericIpAdd;

                if(hdr.ReqType == MODULE_READ_OBJECTS_CFG) {

                    LOG_TRACE << _tag << "Request tasks for tester ID " << hdr.TesterId
                              << ", IP address " << testerIpAddr;
                    dash::hex_dump(std::string(reinterpret_cast<char*>(_packet.data()), bytes_transferred),
                                   LOG_TRACE << _tag);

                    auto dbLinkContainer = receiver::DbLinkContainer::instance().container();

                    if(dbLinkContainer.size() > 0) {

                        std::vector<uint8_t> outBuffer;
                        receiver::DataFlow flow;
                        auto status = flow.packConfigForTester(hdr.TesterId, numericIpAdd, outBuffer);
                        if(outBuffer.size()) {
                            _doWrite(outBuffer);
                        }
                    }

                    _session_manager.stop(shared_from_this());

                } else if(hdr.ReqType >= MODULE_PING && hdr.ReqType < MODULE_PORT_SCAN + 1) {

                    LOG_TRACE << _tag << "Upload reports from tester ID " << hdr.TesterId
                              << ", IP address " << testerIpAddr;

                    ST_SIZES sizes;
                    std::memcpy(&sizes, _packet.data(), sizeof(ST_SIZES));
                    uint32_t size = (sizes.UncmprSize - sizeof(ST_T_HDR))/sizeof(sChgRecord);

                    LOG_INFO << _tag << "Load reports from tester ID: " << hdr.TesterId;
                    receiver::DataFlow flow;
                    auto status = flow.uploadReportsFromTester(size, _packet);

                    std::vector<uint8_t> responseBuffer(4, '\0');
                    std::memcpy(responseBuffer.data(), &size, sizeof(uint32_t));
                    _doWrite(responseBuffer);

                    _isAuthorized = false;
                }
#endif
            }

            if(!_isAuthorized) {
                _session_manager.stop(shared_from_this());
                return;
            }

            _doRead();

        } else if (ec != boost::asio::error::operation_aborted) {
            LOG_TRACE << _tag << "Connection state: " << ec;
        }

#if 0
        if ((boost::asio::error::eof == ec) ||
                (boost::asio::error::connection_reset == ec))
        {
            LOG_INFO << _tag << "Close connection with ec " << ec;
        }
        else
        {
            try {
                if (!ec) {
                    using boost::spirit::ascii::space;
                    typedef std::string::const_iterator iterator_type;
                    typedef request_parser<iterator_type> request_parser;
                    request_parser g; // Our grammar

                    std::string buffer(_data);
                    size_t startpos = buffer.find_first_not_of("}");
                    if( std::string::npos != startpos ) {
                        buffer = buffer.substr( startpos );
                    }

                    request r;
                    std::string::const_iterator iter = buffer.begin();
                    std::string::const_iterator end = buffer.end();
                    bool result = phrase_parse(iter, end, g, space, r);
                    if(result) {
                        // response resp = p_.exec(r);
                        std::ostringstream streamBuf;
                        //json::serializer<response>::serialize(streamBuf, resp);
                        std::string copy = streamBuf.str();
                        length = (copy.length() > MAX_MESSAGE_LENGTH)?
                                    MAX_MESSAGE_LENGTH: copy.length();
                        strncpy(_data, copy.c_str(), length);
                        _data[length - 1] = '\0';
                    }

                    doWrite(length, processor);
                }
            } catch (std::exception e) {
                LOG_ERROR << _tag << "Exception (Session::doRead): " << e.what();
            }
        }
#endif

    });
}

/**
 * @brief session::doRead
 * @param length
 */
void Session::_doWrite(std::vector<uint8_t> buffer) {
    auto self(shared_from_this());
    boost::asio::async_write(_socket, boost::asio::buffer(buffer),
                             [&, self](boost::system::error_code ec, std::size_t /*length*/)
    {
        if (!ec) {
            _doRead();
        }
    });
}

SessionManager::SessionManager()
{
}

void SessionManager::start(session_ptr c)
{
    _sessions.insert(c);
    c->start();
}

void SessionManager::stop(session_ptr c)
{
    _sessions.erase(c);
    c->stop();
}

void SessionManager::stop_all()
{
    for (auto c: _sessions)
        c->stop();
    _sessions.clear();
}



/**
 * @brief server::server
 * @param io_context
 * @param port
 */
Server::Server(boost::asio::io_context& io_context, unsigned short port, Dispatcher& dispatcher)
    : _acceptor(io_context, tcp::endpoint(tcp::v4(), port)),
      _session_manager()
{
    _doAccept(dispatcher);
}


/**
 * @brief server::doAccept
 */
void Server::_doAccept(Dispatcher& dispatcher) {
    _acceptor.async_accept(
                [&](boost::system::error_code ec, tcp::socket socket)
    {
        if (!ec) {
            _session_manager.start(std::make_shared<Session>(
                                       std::move(socket), _session_manager, dispatcher));
        }

        _doAccept(dispatcher);
    });
}

} // namespace tester


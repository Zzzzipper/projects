#include <QTimer>
#include <QMessageBox>
#include <QByteArray>
#include <unistd.h>
#include <signal.h>
#include "udsclient.h"

namespace uds {
// Delay of query server, usec
const uint32_t POLL_DELAY = 5000;
// Delay of responce
const qint64   WAIT_OF_RESPONCE = 3000;
// Singleton instance only
UdsClient* UdsClient::_instance = nullptr;

/**
 * @brief UdsClient::UdsClient
 * @param parent_
 */
UdsClient::UdsClient(QObject* parent_)
    : QObject(parent_)
    , _socket(this)
    , _timer(this)
{
    _in.setDevice(&_socket);
    _in.setVersion(QDataStream::Qt_5_10);
    QObject::connect(&_socket, &QLocalSocket::readyRead
                     , this, &UdsClient::handleReadyRead);
    QObject::connect(&_timer, &QTimer::timeout
                     , this, &UdsClient::timerShot);
    _timer.start(POLL_DELAY);
}

/**
 * @brief UdsClient::instance
 * @return
 */
UdsClient* UdsClient::instance() {
    if(!_instance) {
        _instance = new UdsClient;
    }
    return _instance;
}

/**
 * @brief UdsClient::timerShot - timer handler
 */
void UdsClient::timerShot() {
    //Equire channels status
    foreach(_ChannelDesc desc, _channels) {
        getStatus(desc.num);
    }
}

/**
 * @brief UdsClient::_readChannelsInfo
 * @param data_
 */
void UdsClient::_readChannelsInfo(QString data_) {
    _channels.clear();
    QStringList parts = data_.split(',');
    if(parts.length() > 1) {
        foreach(QString str, parts) {
            if(str == "ok") {
                continue;
            }
            QString num = str.replace("channel", "").trimmed();
            _channels.push_back({num, str});
        }
    }
}

/**
 * @brief UdsClient::handleReadyRead
 */
void UdsClient::handleReadyRead() {
    UdsMessage message(QByteArray(_socket.readAll()).data());
    qDebug() << "Have received: " << message.data().c_str();

    if(_index.check(message.index(), message.command())) {
        _waitOfWrote.wakeOne();
    } else {
        qDebug() << "Unowned packet: " << message.index() << message.data().c_str();
    }

    if(!message.data().empty() && message.data().find("fail") != std::string::npos) {
        qDebug() << "Responce failed..";
        return;
    }
    switch (message.command()) {
    case CMD_CHANNELS_INFO:
        _readChannelsInfo(message.data().c_str());
        Q_EMIT channelInfo(message.data().c_str());
        break;
    default:
        break;
    }
}

/**
 * @brief UdsClient::_error - emit error string to qml advisors
 * @param error_ - error string
 */
void UdsClient::_error(QString message_) {
    qCritical() << message_;
    Q_EMIT haveError(message_);
}

/**
 * @brief UdsClient::_write
 * @param data
 * @param len
 */
void UdsClient::_write(UdsMessage* message_) {
    _writeLocker.lock();
    uint64_t index = _index.indexUp(message_->command());
    message_->setIndex(index);
    if(_socket.write(message_->body(), message_->length()) == -1) {
        _error("Failed write data to socket..");
    }
    if(!_waitOfWrote.wait(&_writeLocker, WAIT_OF_RESPONCE)) {
        qCritical() << "Timout of responce > " << WAIT_OF_RESPONCE << " usec..";
    }
    _writeLocker.unlock();
}

/**
 * @brief UdsClient::startMeasure
 */
void UdsClient::startMeasure(QString) {

}

/**
 * @brief UdsClient::setRange
 */
void UdsClient::setRange(QString, int) {

}

/**
 * @brief UdsClient::stopMeasure
 */
void UdsClient::stopMeasure(QString) {

}

/**
 * @brief UdsClient::getChannelInfo
 */
void UdsClient::getChannelInfo() {
    qDebug() << "Fire getChannelInfo";
    if(!_socket.isOpen() && connect()) {
        UdsMessage message(CMD_CHANNELS_INFO);
        _write(&message);
    }
}

/**
 * @brief UdsClient::getStatus
 * @param channel_
 */
void UdsClient::getStatus(QString channel_) {
    qDebug() << "Fire getStatus " << channel_;
    UdsMessage message(CMD_GET_STATUS, qPrintable(channel_));
    _write(&message);
}

/**
 * @brief UdsClient::getResult
 */
void UdsClient::getResult(QString) {

}

/**
 * @brief UdsClient::connect
 */
bool UdsClient::connect() {
    qDebug() << "Fire connection..";
    _socket.connectToServer(UDS_SOCK_PATH);
    return  _socket.waitForConnected(3000);
}

/**
 * @brief UdsClient::displayError
 * @param socketError
 */
void UdsClient::displayError(QLocalSocket::LocalSocketError socketError)
{
    switch (socketError) {
    case QLocalSocket::ServerNotFoundError:
        _error(tr("The host was not found. Please make sure "
                  "that the server is running and that the "
                  "server name is correct."));
        break;
    case QLocalSocket::ConnectionRefusedError:
        _error(tr("The connection was refused by the peer. "
                  "Make sure the fortune server is running, "
                  "and check that the server name "
                  "is correct."));
        break;
    case QLocalSocket::PeerClosedError:
        break;
    default:
        _error(tr("The following error occurred: %1.")
               .arg(_socket.errorString()));
    }
}

}

#ifndef _udsclient_h_
#define _udsclient_h_

#include <QDataStream>
#include <QLocalSocket>
#include <QMutex>
#include <QWaitCondition>
#include <QTimer>

#include "message.h"

QT_BEGIN_NAMESPACE
class QLabel;
class QLineEdit;
class QPushButton;
QT_END_NAMESPACE


namespace uds{

class UdsClient : public QObject
{
    Q_OBJECT

public:
    // Meausurement start: chennel_
    Q_INVOKABLE void startMeasure(QString);
    // Set range: channel_, bandnumber_
    Q_INVOKABLE void setRange(QString, int);
    // Stop measurement: channel_
    Q_INVOKABLE void stopMeasure(QString);
    // Equire status: channel_
    Q_INVOKABLE void getStatus(QString);
    // Equire result measurement: channel_
    Q_INVOKABLE void getResult(QString);
    // Try to connect
    Q_INVOKABLE bool connect();
    // Get channel info
    Q_INVOKABLE void getChannelInfo();

public:
    static UdsClient* instance();
    explicit UdsClient(QObject *parent = nullptr);
private:
    static UdsClient* _instance;

private slots:
    void displayError(QLocalSocket::LocalSocketError socketError);
    void handleReadyRead();
    void timerShot();

Q_SIGNALS:
    void haveError(const QString message);
    void channelInfo(const QString info);

private:
    void _write(UdsMessage* message_);
    void _error(QString message);
    void _readChannelsInfo(QString data_);

    typedef struct channelDesc {
        QString num;
        QString name;
    } _ChannelDesc;

    typedef struct Index {
        // Increment sinc index and Return current
        // index numeric value
        uint64_t indexUp(uds_command_type command_) {
            ++sentindex;
            command = command_;
            return sentindex;
        }
        // Check index pair
        bool check(uint64_t index_, uds_command_type command_) {
            return (sentindex == index_) && (command == command_);
        }
        uint64_t sentindex = 0L;
        uds_command_type command = CMD_UNKNOWN;
    } _Index;

    _Index                _index;

    QVector<_ChannelDesc> _channels;
    QLocalSocket          _socket;
    QDataStream           _in;
    QTimer                _timer;
    QMutex                _writeLocker;
    QWaitCondition        _waitOfWrote;
};

};

#endif // _udsclient_h_

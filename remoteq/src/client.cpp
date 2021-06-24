#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>

#include "udsclient.h"

/**
 * @brief main
 * @param argc
 * @param argv
 * @return
 */
int main(int argc, char *argv[])
{
    QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);

    QGuiApplication app(argc, argv);

    QQmlApplicationEngine engine;

    QSharedPointer<uds::UdsClient> client(uds::UdsClient::instance());

    engine.rootContext()->setContextProperty("udsClient", client.get());
    qmlRegisterType<uds::UdsClient>("UdsClient", 1, 0, "UdsClient");

    const QUrl url(QStringLiteral("qrc:/client.qml"));

    QObject::connect(&engine, &QQmlApplicationEngine::quit, &QGuiApplication::quit);
    QObject::connect(&engine, &QQmlApplicationEngine::objectCreated,
                     &app, [url](QObject *obj, const QUrl &objUrl) {
        if (!obj && url == objUrl)
            QCoreApplication::exit(-1);
    }, Qt::QueuedConnection);

    engine.load(url);

    return app.exec();
}

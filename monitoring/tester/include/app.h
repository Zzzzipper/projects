#pragma once

#include "dispatcher.h"

namespace tester {

class App {
public:
    App(int argc, char* argv[]);

    // Инициализация и создание модулей, запуск потоков
    bool init();

    // Основной поток приложения
    static int run();

    // Перезагрузка приложения
    void reboot(bool force = true);

private:
    Dispatcher dispatcher_;

};

}

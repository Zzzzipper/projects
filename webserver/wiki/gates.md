[...оглавление](./main.md)

Для примера, методика создания модуля сопряжения для получения данных от сервера приложения по протоколу MDX будет выглядеть следующим образом.
1. Подключение разделяемого кода библиотеки ScdMdx_*
   
- Создание проекта библиотеки обертки на C++ (можно использовать Qt Creator), с которой линкуется библиотека ScdMdx_*;
- Программирование внутри  нее класса, объекты которого реализуют необходимый функционал в соответствии с описаниями методов универсального интерфейса доступа к серверу приложений и данных (пример):
  ```
  class SCDWRAPPERSHARED_EXPORT ScdWrapper
  {
      public:
         ScdWrapper();
            ~ScdWrapper();

   bool connect(const char* addr_, const char* login_, const char* pass_, unsigned int port_);
   void disconnect();
   bool isActive();
   // Периодическое подтверждение активности (при отсутствии других действий)
   bool doTest();
   
   private:
      void* _agent = nullptr;
    };
    ```
- Сборка и тестирование библиотеки;
2. Создание проекта SWIG для сборки обертки между созданной разделяемой библиотекой и бинарником на Go:
- Модификация дефолтного сценария *.i;

    ```
    /* File : scd.i */
    %module(directors="1") scd
    %include "std_string.i"
    %{
        #include "include/scd_client.h"
    %}
    
    /* turn on director wrapping Callback */
    %feature("director") Callback;
    
    /* Let's just grab the original header file here */
    %include "include/scd_client.h"
    ```
- Правка make файла:
    ```
    OP                 = ..
    SWIGEXE             = swig

    # Путь к библиотекам SWIG
    SWIG_LIB_DIR        = ../../../../../../swig/Lib
    
    # Список C++ исходников обертки
    CXXSRCS             = scd_client.cxx
    
    # Наименование выходного результата - статической библиотеки
    TARGET              = scd
    
    # Файл проекта с описанием интерфейсов SWIG
    INTERFACE           = scd.i
    
    # Путь к библиотеке ScdMdx_x64
    SCDMDX_LIB_PATH     = $(PWD)/../../../../../x64/debug

    # Вот здесь указывается путь к библиотеке обертки, которая 
    # линкуется со статической библиотекой, результатом сборки
    SCDWRAPPER_LIB_PATH = $(PWD)/build-scdwrapper-debug

    # Путь к библиотекам Qt
    QTDIR               = $(PWD)/../../../../../../../Qt5.7

    # И здесь - линкуется библиотека ScdMdx_x64.so, Qt* и обертки
    LDFLAGS             = -L$(SCDMDX_LIB_PATH) \
        -L$(QTDIR)/lib -L$(SCDWRAPPER_LIB_PATH) -lQt5Core -lScdMdx_x64 -lscdwrapper

    # Путь к файлам заголовков
    INCLUDES        = -I$(PWD)/../../../../../../../Qt5.7/include

    check: build
        $(MAKE) -f $(TOP)/Makefile SRCDIR='$(SRCDIR)' CXXSRCS='$(CXXSRCS)' TARGET='$(TARGET)' INTERFACE='$(INTERFACE)' go_run

    build:
        @echo `pwd`
        $(MAKE) -f $(TOP)/Makefile SRCDIR='$(SRCDIR)' CXXSRCS='$(CXXSRCS)' \
        SWIG_LIB_DIR='$(SWIG_LIB_DIR)' SWIGEXE='$(SWIGEXE)' INCLUDES='$(INCLUDES)' LDFLAGS='$(LDFLAGS)' \
        TARGET='$(TARGET)' INTERFACE='$(INTERFACE)' go_cpp

    clean:
        $(MAKE) -f $(TOP)/Makefile SRCDIR='$(SRCDIR)' INTERFACE='$(INTERFACE)' go_clean
    ```
- Генерация оберток:

    ```
    #include "../scdwrapper/include/ScdWrapper.h"
    class ScdClient : public ScdWrapper
    {
        public:
           ScdClient() {}
           bool connect(const char* addr_, const char* login_, const char* pass_, unsigned int port_);
           void disconnect();
           bool isActive();
    };
    ```
- Сборка статической библиотек scd.a для инжекции в бинарный код приложения;

3. Вызов функций Go из вновь созданных интерфейсов в коде Go (пример):

    ```
    var Client ScdClient = nil
    
    func Create() {
        var failPingCouner int = 0;
        // Check periodically connection
        go func() {
            for {
                Client = NewScdClient()
                connectRet := Client.Connect("192.168.1.49", "admin", "", 1312)
                if connectRet == true {
                    fmt.Println("-------------------------------------------")
                    fmt.Println("Scd gate connected successfully")
                    for {
                        if failPingCouner < 5 {
                            failPingCouner++
                        } else {
                            failPingCouner = 0;
                            break
                        }
                        fmt.Println("..ping")
                        time.Sleep(time.Duration(3) * time.Second)
                    }
                } 
            
                // Start reconnect loop
                Client.Disconnect()
                time.Sleep(time.Duration(3) * time.Second)
                fmt.Println("Reconnect Scd gate fire..")
            }
        }()
    }
    ```
    


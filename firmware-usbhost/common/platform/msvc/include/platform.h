#ifndef COMMON_PLATFORM_MSVC_PLATFORM_H
#define COMMON_PLATFORM_MSVC_PLATFORM_H

#ifdef _MSC_VER

//На процессоре x64 невозможно запретить прерывания. Защищать нужно от работы в мульти-нитевом приложении.
// Это можно реализовать через мютексы. Но для этого в класс List требуется добавить мютекс, после чего класс перестанет собираться под
// другие платформы. Возможно класс List, следует специализировать по типу платформу.
#define ATOMIC_RESTORESTATE 1
#define ATOMIC_BLOCK(x)

#define strncasecmp _strnicmp
#define strcasecmp _stricmp

#endif

#endif

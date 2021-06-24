#if 0
#include "fiscal_register/atol/AtolProtocol.h"
#include "logger/include/Logger.h"
#include "test/include/Test.h"

void TestAtol::testAtol() {
/*
Пример: передать блок данных <1F 00 FF 10 02 03 1A>.
1. Маскируем байты, равные DLE и ETX (10h и 03h): <1F 00 FF 10 10 02 10 03 1A>.
2. Добавляем в конец ETX:                         <1F 00 FF 10 10 02 10 03 1A 03>.
3. Подсчитываем <CRC>: 1F XOR 00 XOR FF XOR 10 XOR 10 XOR 02 XOR 10 XOR 03 XOR 1A XOR 03 = E8.
4. Добавляем в начало STX:                     <02 1F 00 FF 10 10 02 10 03 1A 03>.
5. Добавляем в конец <CRC>:                    <02 1F 00 FF 10 10 02 10 03 1A 03 E8>.
Передавать следует последовательность байт, полученную после шага 5.
*/
//todo: тест данного случая или отправителя данных целиком
}
#endif

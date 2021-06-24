#include "RttLogTarget.h"
#include "uart/include/interface.h"
#include "common/logger/arm/rtt/SEGGER_RTT.h"

/* Режим логера RTT для SEGGER J-LINK: https://habrahabr.ru/post/259205/
 *
 * Для отладки через SEGGER J-LINK, удалить файл C:\Users\%username%\AppData\Roaming\CooCox\CoIDE\config\debugger\debugData.d
 * Источник - http://www.coocox.org/forum/viewtopic.php?f=2&t=5156&start=20
*/

RttLogTarget::RttLogTarget(AbstractUart *uart) : uart(uart) {}

void RttLogTarget::send(const uint8_t *data, const uint16_t len)
{
	SEGGER_RTT_Write(0, data, len);
}

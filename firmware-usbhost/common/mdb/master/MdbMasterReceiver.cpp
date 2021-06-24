#include "MdbMasterReceiver.h"
#include "include/MdbMasterEngine.h"

#include "mdb/MdbProtocol.h"
#include "logger/include/Logger.h"

MdbMasterReceiver::MdbMasterReceiver(Customer *customer, StatStorage *stat) :
	customer(customer),
	buf(MDB_PACKET_MAX_SIZE),
	uart(NULL)
{
	this->responseCount = stat->add(Mdb::DeviceContext::Info_MdbM_ResponseCount, 0);
	this->confirmCount = stat->add(Mdb::DeviceContext::Info_MdbM_ConfirmCount, 0);
	this->crcErrorCount = stat->add(Mdb::DeviceContext::Info_MdbM_CrcErrorCount, 0);
}

void MdbMasterReceiver::setUart(AbstractUart *uart, AbstractUart *sendUart) {
	this->setLen(1);
	this->uart = uart;
	this->uart->setReceiveHandler(this);
	this->sendUart = sendUart;
}

void MdbMasterReceiver::handle() {
	if(uart == NULL) {
		return;
	}

	while(uart->isEmptyReceiveBuffer() == false) {
		uint8_t b1 = uart->receive();
		uint8_t b2 = uart->receive();
		LOG_DEBUG(LOG_MDBM, "recv");
		if(sendUart != NULL) {
			sendUart->sendAsync(b1);
			sendUart->sendAsync(b2);
		}
		buf.addUint8(b2);
		if(b1 == 0x01) {
			uint16_t len = buf.getLen() - 1;
			if(len > 0) {
				LOG_TRACE_HEX(LOG_MDBM, buf.getData(), buf.getLen());
				uint8_t actCrc = Mdb::calcCrc(buf.getData(), len);
				uint8_t expCrc = buf[len];
				if(actCrc != expCrc) {
					LOG_ERROR(LOG_MDBM, "Wrong CRC " << actCrc << "<>" << expCrc);
					LOG_ERROR_HEX(LOG_MDBS, buf.getData(), buf.getLen());
					buf.clear();
					crcErrorCount->inc();
					return;
				}
				customer->procResponse(buf.getData(), len, true);
				buf.clear();
				responseCount->inc();
				return;
			} else {
				LOG_TRACE_HEX(LOG_MDBM, b2);
				customer->procResponse(buf.getData(), 1, false);
				buf.clear();
				confirmCount->inc();
				return;
			}
		}
	}
}

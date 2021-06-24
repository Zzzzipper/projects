#include "include/EcpClient.h"
#include "EcpProtocol.h"
#include "EcpClientPacketLayer.h"
#include "EcpClientCommandLayer.h"
#include "logger/include/Logger.h"

namespace Ecp {

Client::Client(TimerEngine *timers, AbstractUart *uart) {
	this->packetLayer = new ClientPacketLayer(timers, uart);
	this->commandLayer = new ClientCommandLayer(timers, packetLayer);
}

Client::~Client() {
	delete this->commandLayer;
	delete this->packetLayer;
}

void Client::setObserver(EventObserver *observer) {
	commandLayer->setObserver(observer);
}

bool Client::connect() {
	return commandLayer->connect();
}

void Client::disconnect() {
	commandLayer->disconnect();
}

bool Client::uploadData(Destination destination, Dex::DataGenerator *generator) {
	return commandLayer->uploadData(destination, generator);
}

bool Client::downloadData(Source source, Dex::DataParser *parser) {
	return commandLayer->downloadData(source, parser);
}

bool Client::getTableInfo(uint16_t tableId) {
	return commandLayer->getTableInfo(tableId);
}

bool Client::getTableEntry(uint16_t tableId, uint32_t entryIndex) {
	return commandLayer->getTableEntry(tableId, entryIndex);
}

bool Client::getDateTime() {
	return commandLayer->getDateTime();
}

bool Client::resetConfig() {
	return commandLayer->resetConfig();
}

void Client::cancel() {
	commandLayer->cancel();
}

}

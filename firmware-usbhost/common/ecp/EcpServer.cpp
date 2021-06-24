#include "include/EcpServer.h"
#include "EcpProtocol.h"
#include "EcpServerPacketLayer.h"
#include "EcpServerCommandLayer.h"
#include "logger/include/Logger.h"

namespace Ecp {

Server::Server(AbstractUart *uart, TimerEngine *timers) {
	this->packetLayer = new ServerPacketLayer(timers, uart);
	this->commandLayer = new ServerCommandLayer(timers, packetLayer);
}

Server::~Server() {
	delete this->commandLayer;
	delete this->packetLayer;
}

void Server::setObserver(EventObserver *observer) {
	commandLayer->setObserver(observer);
}

void Server::setFirmwareParser(Dex::DataParser *parser) {
	commandLayer->setFirmwareParser(parser);
}

void Server::setGsmParser(Dex::DataParser *parser) {
	commandLayer->setGsmParser(parser);
}

void Server::setScreenParser(Dex::DataParser *parser) {
	commandLayer->setScreenParser(parser);
}

void Server::setConfigParser(Dex::DataParser *parser) {
	commandLayer->setConfigParser(parser);
}

void Server::setConfigGenerator(Dex::DataGenerator *generator) {
	commandLayer->setConfigGenerator(generator);
}

void Server::setConfigEraser(Dex::DataParser *eraser) {
	commandLayer->setConfigEraser(eraser);
}

void Server::setTableProcessor(TableProcessor *processor) {
	commandLayer->setTableProcessor(processor);
}

void Server::reset() {
	commandLayer->reset();
}

void Server::shutdown() {
	commandLayer->shutdown();
}

void Server::disconnect() {
	commandLayer->disconnect();
}

}

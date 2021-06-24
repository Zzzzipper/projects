#include "GsmCommand.h"

#include <stddef.h>

namespace Gsm {

Command::Command(Observer *observer) :
	m_text(AT_COMMAND_TEXT_SIZE, AT_COMMAND_TEXT_SIZE),
	m_timeout(AT_COMMAND_TIMEOUT),
	m_data(NULL),
	m_dataLen(0),
	m_observer(observer)
{
}

void Command::set(Type type) {
	m_type = type;
	m_text.clear();
	m_data = NULL;
	m_dataLen = 0;
	m_timeout = AT_COMMAND_TIMEOUT;
}

void Command::set(Type type, const char *text) {
	m_type = type;
	m_text = text;
	m_data = NULL;
	m_dataLen = 0;
	m_timeout = AT_COMMAND_TIMEOUT;
}

void Command::set(Type type, const char *text, uint32_t timeout) {
	m_type = type;
	m_text = text;
	m_data = NULL;
	m_dataLen = 0;
	m_timeout = timeout;
}

StringBuilder &Command::setText() {
	return m_text;
}

void Command::setTimeout(uint32_t timeout) {
	m_timeout = timeout;
}

void Command::setData(uint8_t *data, uint16_t dataLen) {
	m_data = data;
	m_dataLen = dataLen;
}

Command::Type Command::getType() const {
	return m_type;
}

const char *Command::getText() const {
	return m_text.getString();
}

uint32_t Command::getTimeout() const {
	return m_timeout;
}

uint8_t *Command::getData() {
	return m_data;
}

uint16_t Command::getDataLen() const {
	return m_dataLen;
}

void Command::deliverResponse(Result result, const char *data) {
	if(m_observer == NULL) {
		return;
	}
	m_observer->procResponse(result, data);
}

}

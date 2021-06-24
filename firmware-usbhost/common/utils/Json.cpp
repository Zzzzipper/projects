#include "utils/include/Json.h"
#include "utils/include/StringParser.h"
#include "utils/include/CodePage.h"
#include "logger/include/Logger.h"

#include "platform/include/platform.h"

#include <string.h>
#include <strings.h>

void JsonNode::init(Type type, JsonNode *parent, JsonParser *parser) {
	this->type = type;
	this->parent = parent;
	this->parser = parser;
}

void JsonNode::setType(Type type) {
	this->type = type;
}

JsonNode::Type JsonNode::getType() const {
	return type;
}

void JsonNode::setValue(const char *value, uint16_t valueLen) {
	this->value = value;
	this->valueLen = valueLen;
}

const char *JsonNode::getValue() const {
	return value;
}

uint16_t JsonNode::getValueLen() const {
	return valueLen;
}

bool JsonNode::isValue(const char *value, int valueLen) const {
	if(valueLen != this->valueLen) {
		return false;
	}
	return (strncmp(value, this->value, this->valueLen) == 0);
}

bool JsonNode::isValue(const char *value) const {
	int valueLen = strlen(value);
	return isValue(value, valueLen);
}

bool JsonNode::isCaseValue(const char *value, int valueLen) const {
	if(valueLen != this->valueLen) {
		return false;
	}
	return (strncasecmp(value, this->value, this->valueLen) == 0);
}

bool JsonNode::isCaseValue(const char *value)  const {
	int valueLen = strlen(value);
	return isCaseValue(value, valueLen);
}

JsonNode *JsonNode::getParent() {
	return parent;
}

JsonNode *JsonNode::getChild() {
	if(type != Type_Member) {
		LOG_ERROR(LOG_JSON, "Wrong node type");
		return NULL;
	}
	int i = 0;
	JsonNode *node = parser->getNode(i);
	for(; node != NULL; i++, node = parser->getNode(i)) {
		if(node->parent == this) {
			return node;
		}
	}
	return NULL;
}

JsonNode *JsonNode::getChild(const char *name) {
	if(type != Type_Object) {
		LOG_ERROR(LOG_JSON, "Wrong node type " << type);
		return NULL;
	}
	int i = 0;
	JsonNode *node = parser->getNode(i);
	for(; node != NULL; i++, node = parser->getNode(i)) {
		if(node->parent == this && node->isValue(name) == true) {
			return node;
		}
	}
	return NULL;
}

JsonNode *JsonNode::getChildByIndex(int index) {
	if(type != Type_Array) {
		LOG_ERROR(LOG_JSON, "Wrong node type");
		return NULL;
	}
	int i = 0;
	int count = 0;
	JsonNode *node = parser->getNode(i);
	for(; node != NULL; i++, node = parser->getNode(i)) {
		if(node->parent == this) {
			if(count == index) {
				return node;
			} else {
				count += 1;
			}
		}
	}
	return NULL;
}

JsonNode *JsonNode::getField(const char *name, Type valueType) {
	JsonNode *field = getChild(name);
	if(field == NULL || field->getType() != JsonNode::Type_Member) {
		LOG_ERROR(LOG_JSON, "Wrong response format");
		return NULL;
	}

	JsonNode *fieldValue = field->getChild();
	if(fieldValue == NULL || fieldValue->getType() != valueType) {
		LOG_ERROR(LOG_JSON, "Wrong response format " << fieldValue->getType());
		return NULL;
	}

	return fieldValue;
}

JsonNode *JsonNode::getDateTimeField(const char *name, DateTime *datetime) {
	JsonNode *field = getField(name, JsonNode::Type_String);
	if(field == NULL) {
		LOG_ERROR(LOG_JSON, "Wrong response format");
		return NULL;
	}

	if(stringToDateTime(field->getValue(), field->getValueLen(), datetime) == false) {
		LOG_ERROR(LOG_JSON, "Wrong response format");
		return NULL;
	}

	return field;
}

JsonNode *JsonNode::getStringField(const char *name, StringBuilder *buf) {
	JsonNode *field = getField(name, JsonNode::Type_String);
	if(field == NULL) {
		LOG_ERROR(LOG_JSON, "Wrong response format");
		return NULL;
	}

	convertJsonUnicodeToWin1251(field->getValue(), field->getValueLen(), buf);
	return field;
}

JsonParser::JsonParser(uint16_t nodeMaxNumber) : nodeNum(nodeMaxNumber) {
	nodes = new JsonNode[nodeNum];
}

JsonParser::~JsonParser() {
	delete nodes;
}

JsonNode *JsonParser::getRoot() {
	if(0 >= nodeNext) {
		return NULL;
	}
	return &nodes[0];
}

JsonNode *JsonParser::getNode(int index) {
	if(index >= nodeNext) {
		return NULL;
	}
	return &nodes[index];
}

JsonNode *JsonParser::allocNode() {
	if(nodeNext >= nodeNum) {
		return NULL;
	}
	JsonNode *node = &nodes[nodeNext];
	nodeNext += 1;
	return node;
}

bool JsonParser::parse(const char *data, uint16_t dataLen) {
	nodeNext = 0;
	StringParser parser(data, dataLen);
	return parseValue(&parser, NULL);
}

JsonNode *JsonParser::parseValue(StringParser *parser, JsonNode *parent) {
	LOG_DEBUG(LOG_JSON, "parseValue");
	parser->skipEqual("\t\n\r ");
	if(parser->compareAndSkip('{') == true) {
		LOG_DEBUG(LOG_JSON, "Found {: " << parser->unparsed());
		return parseObject(parser, parent);
	} else if(parser->compareAndSkip('[') == true) {
		LOG_DEBUG(LOG_JSON, "Found [: " << parser->unparsed());
		return parseArray(parser, parent);
	} else if(parser->compareAndSkip('"') == true) {
		LOG_DEBUG(LOG_JSON, "Found String: " << parser->unparsed());
		return parseString(parser, parent);
	} else {
		LOG_DEBUG(LOG_JSON, "Found other: " << parser->unparsed());
		return parsePrimitive(parser, parent);
	}

	return NULL;
}

JsonNode *JsonParser::parseObject(StringParser *parser, JsonNode *parent) {
	LOG_DEBUG(LOG_JSON, "parseObject");
	JsonNode *node = allocNode();
	if(node == NULL) {
		LOG_ERROR(LOG_JSON, "Free nodes is out");
		return NULL;
	}
	node->init(JsonNode::Type_Object, parent, this);
	const char *start = parser->unparsed();

	while(parser->hasUnparsed() == true) {
		parser->skipEqual("\t\n\r ");
		if(parser->compareAndSkip('"') == true) {
			LOG_DEBUG(LOG_JSON, "Found member: " << parser->unparsed());
			if(parseMember(parser, node) == NULL) {
				return NULL;
			}
		} else if(parser->compareAndSkip(',') == true) {
			LOG_DEBUG(LOG_JSON, "Found ,: " << parser->unparsed());
		} else if(parser->compareAndSkip('}') == true) {
			LOG_DEBUG(LOG_JSON, "Found }: " << parser->unparsed());
			const char *end = parser->unparsed();
			int size = end - start - 1;
			node->setValue(start, size);
			return node;
		} else {
			LOG_ERROR(LOG_JSON, "Invalid json format");
			return NULL;
		}
	}

	return NULL;
}

JsonNode *JsonParser::parseMember(StringParser *parser, JsonNode *parent) {
	LOG_DEBUG(LOG_JSON, "parseMember");
	JsonNode *node = parseString(parser, parent);
	if(node == NULL) {
		LOG_ERROR(LOG_JSON, "parseString failed");
		return NULL;
	}

	parser->skipEqual("\t\n\r ");
	if(parser->compareAndSkip(':') == false) {
		LOG_ERROR(LOG_JSON, "Invalid json format " << parser->getIndex());
		return NULL;
	}

	if(parseValue(parser, node) == NULL) {
		LOG_ERROR(LOG_JSON, "parseValue failed");
		return NULL;
	}

	node->setType(JsonNode::Type_Member);
	return node;
}

JsonNode *JsonParser::parseArray(StringParser *parser, JsonNode *parent) {
	LOG_DEBUG(LOG_JSON, "parseArray");
	JsonNode *node = allocNode();
	if(node == NULL) {
		LOG_ERROR(LOG_JSON, "Free nodes is out");
		return NULL;
	}
	node->init(JsonNode::Type_Array, parent, this);
	const char *start = parser->unparsed();

	while(parser->hasUnparsed() == true) {
		parser->skipEqual("\t\n\r ");
		if(parser->compareAndSkip(',') == true) {
			LOG_DEBUG(LOG_JSON, "Found ,: " << parser->unparsed());
		} else if(parser->compareAndSkip(']') == true) {
			LOG_DEBUG(LOG_JSON, "Found ]: " << parser->unparsed());
			const char *end = parser->unparsed();
			int size = end - start - 1;
			node->setValue(start, size);
			return node;
		} else {
			if(parseValue(parser, node) == NULL) {
				LOG_ERROR(LOG_JSON, "parseValue failed");
				return NULL;
			}
		}
	}

	return NULL;
}

JsonNode *JsonParser::parseString(StringParser *parser, JsonNode *parent) {
	LOG_DEBUG(LOG_JSON, "parseString");
	JsonNode *node = allocNode();
	if(node == NULL) {
		LOG_ERROR(LOG_JSON, "Free nodes is out");
		return NULL;
	}
	node->init(JsonNode::Type_String, parent, this);
	const char *start = parser->unparsed();

	parser->skipNotEqual("\"", '\\');
	const char *end = parser->unparsed();
	if(parser->compareAndSkip('"') == false) {
		LOG_DEBUG(LOG_JSON, "Not found \": " << parser->unparsed());
		return NULL;
	}

	int size = end - start;
	node->setValue(start, size);
	return node;
}

JsonNode *JsonParser::parsePrimitive(StringParser *parser, JsonNode *parent) {
	LOG_DEBUG(LOG_JSON, "parsePrimitive");
	JsonNode *node = allocNode();
	if(node == NULL) {
		LOG_ERROR(LOG_JSON, "Free nodes is out");
		return NULL;
	}
	node->init(JsonNode::Type_None, parent, this);

	const char *start = parser->unparsed();
	parser->skipNotEqual(",}]\t\n\r ");
	const char *end = parser->unparsed();

	int size = end - start;
	node->setValue(start, size);
	const char stringTrue[] = "true";
	const char stringFalse[] = "false";
	const char stringNull[] = "null";
	if(node->isCaseValue(stringTrue, sizeof(stringTrue) - 1) == true) {
		node->setType(JsonNode::Type_TRUE);
	} else if(node->isCaseValue(stringFalse, sizeof(stringFalse) - 1) == true) {
		node->setType(JsonNode::Type_FALSE);
	} else if(node->isCaseValue(stringNull, sizeof(stringNull) - 1) == true) {
		node->setType(JsonNode::Type_NULL);
	} else {
		node->setType(JsonNode::Type_Number);
	}

	return node;
}

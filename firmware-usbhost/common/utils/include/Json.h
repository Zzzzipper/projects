#ifndef COMMON_UTILS_JSON_H_
#define COMMON_UTILS_JSON_H_

#include "utils/include/Number.h"
#include "utils/include/StringBuilder.h"
#include "timer/include/DateTime.h"

#include <stdint.h>

class StringParser;
class JsonParser;

class JsonNode {
public:
	enum Type {
		Type_None = 0,
		Type_Object,
		Type_Member,
		Type_String,
		Type_Number,
		Type_TRUE,
		Type_FALSE,
		Type_NULL,
		Type_Array,
	};

	void init(Type type, JsonNode *parent, JsonParser *parser);

	void setType(Type type);
	Type getType() const;

	void setValue(const char *value, uint16_t valueLen);
	const char *getValue() const;
	uint16_t getValueLen() const;
	bool isValue(const char *value, int valueLen) const;
	bool isValue(const char *value) const;
	bool isCaseValue(const char *value, int valueLen) const;
	bool isCaseValue(const char *value) const;

	JsonNode *getParent();
	JsonNode *getChild();
	JsonNode *getChild(const char *name);
	JsonNode *getChildByIndex(int index);
	JsonNode *getField(const char *name, Type valueType);
	JsonNode *getDateTimeField(const char *name, DateTime *datetime);
	JsonNode *getStringField(const char *name, StringBuilder *buf);
	template <typename T>
	JsonNode *getNumberField(const char *name, T *value) {
		JsonNode *field = getChild(name);
		if(field == NULL || field->getType() != JsonNode::Type_Member) {
			return NULL;
		}

		JsonNode *fieldValue = field->getChild();
		if(fieldValue == NULL || (fieldValue->getType() != Type_Number && fieldValue->getType() != Type_String)) {
			return NULL;
		}

		uint16_t l = Sambery::stringToNumber<T>(fieldValue->getValue(), fieldValue->getValueLen(), value);
		if(l != fieldValue->getValueLen()) {
			return NULL;
		}
		return fieldValue;
	}

private:
	Type type;
	const char *value;
	uint16_t valueLen;
	JsonNode *parent;
	JsonParser *parser;
};

class JsonParser {
public:
	JsonParser(uint16_t nodeMaxNumber);
	~JsonParser();
	bool parse(const char *data, uint16_t dataLen);
	JsonNode *getRoot();
	JsonNode *getNode(int index);

private:
	JsonNode *nodes;
	int nodeNum;
	int nodeNext;

	JsonNode *allocNode();
	JsonNode *parseValue(StringParser *parser, JsonNode *parent);
	JsonNode *parseObject(StringParser *parser, JsonNode *parent);
	JsonNode *parseMember(StringParser *parser, JsonNode *parent);
	JsonNode *parseArray(StringParser *parser, JsonNode *parent);
	JsonNode *parseString(StringParser *parser, JsonNode *parent);
	JsonNode *parsePrimitive(StringParser *parser, JsonNode *parent);
};

#endif

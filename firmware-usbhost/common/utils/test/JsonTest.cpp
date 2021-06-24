#include "utils/include/Json.h"
#include "logger/include/Logger.h"
#include "test/include/Test.h"

class JsonTest : public TestSet {
public:
	JsonTest();
	bool test();
	bool testStringQuotedSymbols();
	bool testNullValue();
};

TEST_SET_REGISTER(JsonTest);

JsonTest::JsonTest() {
	TEST_CASE_REGISTER(JsonTest, test);
	TEST_CASE_REGISTER(JsonTest, testStringQuotedSymbols);
	TEST_CASE_REGISTER(JsonTest, testNullValue);
}

bool JsonTest::test() {
	const char data[] = "{\"success\":true,\"data\":[{\"sv\":\"1.1.1\",\"config_id\":14}]}";

	JsonParser parser(20);
	TEST_NUMBER_EQUAL(true, parser.parse(data, sizeof(data)));

	JsonNode *object1 = parser.getRoot();
	TEST_POINTER_NOT_NULL(object1);
	TEST_NUMBER_EQUAL(JsonNode::Type_Object, object1->getType());
	TEST_SUBSTR_EQUAL("\"success\":true,\"data\":[{\"sv\":\"1.1.1\",\"config_id\":14}]", object1->getValue(), object1->getValueLen());

	// --------------------
	// success:true
	//  --------------------
	JsonNode *member1 = object1->getChild("success");
	TEST_POINTER_NOT_NULL(member1);
	TEST_NUMBER_EQUAL(JsonNode::Type_Member, member1->getType());
	TEST_SUBSTR_EQUAL("success", member1->getValue(), member1->getValueLen());

	JsonNode *true1 = member1->getChild();
	TEST_POINTER_NOT_NULL(true1);
	TEST_NUMBER_EQUAL(JsonNode::Type_TRUE, true1->getType());
	TEST_SUBSTR_EQUAL("true", true1->getValue(), true1->getValueLen());

	// --------------------
	// data:array()
	//  --------------------
	JsonNode *member2 = object1->getChild("data");
	TEST_POINTER_NOT_NULL(member2);
	TEST_NUMBER_EQUAL(JsonNode::Type_Member, member2->getType());
	TEST_SUBSTR_EQUAL("data", member2->getValue(), member2->getValueLen());

	JsonNode *array1 = member2->getChild();
	TEST_POINTER_NOT_NULL(array1);
	TEST_NUMBER_EQUAL(JsonNode::Type_Array, array1->getType());
	TEST_SUBSTR_EQUAL("{\"sv\":\"1.1.1\",\"config_id\":14}", array1->getValue(), array1->getValueLen());

	JsonNode *object2 = array1->getChildByIndex(0);
	TEST_POINTER_NOT_NULL(object2);
	TEST_NUMBER_EQUAL(JsonNode::Type_Object, object2->getType());
	TEST_SUBSTR_EQUAL("\"sv\":\"1.1.1\",\"config_id\":14", object2->getValue(), object2->getValueLen());

	// --------------------
	// sv:"1.1.1"
	//  --------------------
	JsonNode *member3 = object2->getChild("sv");
	TEST_POINTER_NOT_NULL(member3);
	TEST_NUMBER_EQUAL(JsonNode::Type_Member, member3->getType());
	TEST_SUBSTR_EQUAL("sv", member3->getValue(), member3->getValueLen());

	JsonNode *string1 = member3->getChild();
	TEST_POINTER_NOT_NULL(string1);
	TEST_NUMBER_EQUAL(JsonNode::Type_String, string1->getType());
	TEST_SUBSTR_EQUAL("1.1.1", string1->getValue(), string1->getValueLen());

	// --------------------
	// config_id:14
	//  --------------------
	JsonNode *member4 = object2->getChild("config_id");
	TEST_POINTER_NOT_NULL(member4);
	TEST_NUMBER_EQUAL(JsonNode::Type_Member, member4->getType());
	TEST_SUBSTR_EQUAL("config_id", member4->getValue(), member4->getValueLen());

	JsonNode *number1 = member4->getChild();
	TEST_POINTER_NOT_NULL(number1);
	TEST_NUMBER_EQUAL(JsonNode::Type_Number, number1->getType());
	TEST_SUBSTR_EQUAL("14", number1->getValue(), number1->getValueLen());

	return true;
}

bool JsonTest::testStringQuotedSymbols() {
	const char data[] = "{\"text\":\"test=\\\"value\\\"\"}";

	JsonParser parser(20);
	TEST_NUMBER_EQUAL(true, parser.parse(data, sizeof(data)));

	JsonNode *object1 = parser.getRoot();
	TEST_POINTER_NOT_NULL(object1);
	TEST_NUMBER_EQUAL(JsonNode::Type_Object, object1->getType());

	StringBuilder str1(32, 32);
	JsonNode *text1 = object1->getStringField("text", &str1);
	TEST_POINTER_NOT_NULL(text1);
	TEST_SUBSTR_EQUAL("test=\\\"value\\\"", text1->getValue(), text1->getValueLen());
	TEST_STRING_EQUAL("test=\"value\"", str1.getString());
	return true;
}

bool JsonTest::testNullValue() {
	const char data[] = "{\"qr\": null}";

	JsonParser parser(20);
	TEST_NUMBER_EQUAL(true, parser.parse(data, sizeof(data)));

	JsonNode *object1 = parser.getRoot();
	TEST_POINTER_NOT_NULL(object1);
	TEST_NUMBER_EQUAL(JsonNode::Type_Object, object1->getType());

	JsonNode *field1 = object1->getChild("qr");
	TEST_POINTER_NOT_NULL(field1);
	TEST_NUMBER_EQUAL(JsonNode::Type_Member, field1->getType());

	JsonNode *field1Value = field1->getChild();
	TEST_POINTER_NOT_NULL(field1Value);
	TEST_NUMBER_EQUAL(JsonNode::Type_NULL, field1Value->getType());
	return true;
}

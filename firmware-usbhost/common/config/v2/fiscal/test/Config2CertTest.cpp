#include "memory/include/RamMemory.h"
#include "config/v2/fiscal/Config2Cert.h"
#include "logger/include/Logger.h"
#include "test/include/Test.h"

class Config2CertTest : public TestSet {
public:
	Config2CertTest();
	bool test();
};

TEST_SET_REGISTER(Config2CertTest);

Config2CertTest::Config2CertTest() {
	TEST_CASE_REGISTER(Config2CertTest, test);
}

bool Config2CertTest::test() {
	RamMemory memory(32000);
	memory.clear();

	const char certData[] =
"-----BEGIN CERTIFICATE-----\n"
"MIIDYjCCAkoCAQAwDQYJKoZIhvcNAQELBQAwcTELMAkGA1UEBhMCUlUxDzANBgNV\n"
"BAgMBk1vc2NvdzEPMA0GA1UEBwwGTW9zY293MRMwEQYDVQQKDApPcmFuZ2VkYXRh\n"
"MQ8wDQYDVQQLDAZOZWJ1bGExGjAYBgNVBAMMEXd3dy5vcmFuZ2VkYXRhLnJ1MB4X\n"
"DTE4MDMxNTE2NDYwMVoXDTI4MDMxMjE2NDYwMVowfTELMAkGA1UEBhMCUlUxDzAN\n"
"BgNVBAgMBk1vc2NvdzEPMA0GA1UEBwwGTW9zY293MR8wHQYDVQQKDBZPcmFuZ2Vk\n"
"YXRhIHRlc3QgY2xpZW50MRMwEQYDVQQLDApFLWNvbW1lcmNlMRYwFAYDVQQDDA1v\n"
"cmFuZ2VkYXRhLnJ1MIIBIjANBgkqhkiG9w0BAQEFAAOCAQ8AMIIBCgKCAQEAo7XZ\n"
"+VUUo9p+Q0zPmlt1eThA8NmVVAgNXkVDZoz3umyEnnm2d4R5Voxf4y6fuesW3Za8\n"
"/ImKWLbQ3/S/pHZKWiz75ElSfpnYJfMRuLAaqqs0eFfxmHbHi8Mgg9zjAMdILpR6\n"
"eEaP7qeCNRom3Zb6ziYoWEmDC2ZFFu9995rjkn7CtV3noWZveOCGExjM7WTkql8L\n"
"v1PX3ee3fXaEC7Kefxl4O/4w7agEceKRHlc0l3iwVJaKittQwAQd3ieUwoqsxzPH\n"
"dRwB4IU9aI6IjfqteyD51s7xd+ayM/O4j+aJ/HBhJajDHBcGWKytxv0f6YpqPUAc\n"
"25fRAXVa0Gsei6eY/QIDAQABMA0GCSqGSIb3DQEBCwUAA4IBAQCv/Vcxh2lMt8RV\n"
"Al0V9xIst0ZdjH22yTOUCOiH9PZgeagqrjTLT3ycWAdbZZUpzcFSdOmPUsgQ7Eqz\n"
"+TpcY5lmYFInLwJK/Afjqsb5LK2irGKT254p5qzD9rSRlM42wxRzQTA0BWX3mmhi\n"
"zwdrfLAvyCw1gHBbUZNf3eemBCY+8RRGPRAqD2XbyIya1bX0AHLXbx5dBe9EIOG/\n"
"F46WbTlrkR7kc06eiacTiGYwNdcywJ2KOcvmnXPup8Os6KOWe197CIathDHeiG2C\n"
"mQlsQDF/d7W4G/+l6Q66BhfRtuhp99gkT8P8j82X6ChrwbgQ5+vya3SytJ0wmIg2\n"
"67jOKmGK\n"
"-----END CERTIFICATE-----\n";

	StringBuilder str(2048, 2048);
	str.set(certData);
	Config2Cert cert1;
	TEST_NUMBER_EQUAL(MemoryResult_Ok, cert1.init(&memory));
	TEST_NUMBER_EQUAL(MemoryResult_Ok, cert1.save(&str));

	// check auth cert
	StringBuilder buf(2048, 2048);
	Config2Cert cert2;
	memory.setAddress(0);
	TEST_NUMBER_EQUAL(MemoryResult_Ok, cert2.load(&memory));
	TEST_NUMBER_EQUAL(MemoryResult_Ok, cert2.load(&buf));
	TEST_STRING_EQUAL(certData, buf.getString());

	return true;
}

#include "config/v3/Config3ConfigGenerator.h"
#include "config/v3/Config3ConfigIniter.h"
#include "config/v3/Config3ConfigParser.h"
#include "test/include/Test.h"
#include "memory/include/RamMemory.h"
#include "timer/include/TestRealTime.h"
#include "logger/include/Logger.h"

class Config3GeneratorTest : public TestSet {
public:
	Config3GeneratorTest();
	bool init();
	void cleanup();
	bool testSimple();
	bool testWithCerts();

private:
	RamMemory *memory;
	TestRealTime *realtime;
	StatStorage *stat;
	Config3Modem *config;
};

TEST_SET_REGISTER(Config3GeneratorTest);

Config3GeneratorTest::Config3GeneratorTest() {
	TEST_CASE_REGISTER(Config3GeneratorTest, testSimple);
	TEST_CASE_REGISTER(Config3GeneratorTest, testWithCerts);
}

bool Config3GeneratorTest::init() {
	memory = new RamMemory(32000);
	memory->clear();
	realtime = new TestRealTime;
	stat = new StatStorage;
	config = new Config3Modem(memory, realtime, stat);
	return true;
}

void Config3GeneratorTest::cleanup() {
	delete config;
	delete stat;
	delete realtime;
	delete memory;
}

bool Config3GeneratorTest::testSimple() {
	Config3ConfigIniter initer;
	Config3ConfigParser parser(config);
	Config3ConfigGenerator generator(config);

	MdbCoinChangerContext *coins = config->getAutomat()->getCCContext();
	uint8_t coinInitData[] = { 1, 2, 5, 10 };
	uint8_t coinUpdateData[] = { 67, 54, 114, 51 };
	coins->init(1, 1, 10, coinInitData, sizeof(coinInitData), 0x000F);
	coins->update(0x0005, coinUpdateData, sizeof(coinUpdateData));

	const char data1[] =
"DXS*EPHOR00001*VA*V0/6*1\r\n"
"ST*001*0001\r\n"
"AC2**internet*gdata*gdata*1*0*0*0*\r\n"
"AC3*0*1**0**0\r\n"
"IC1******0\r\n"
"IC4*1*7**1\r\n"
"PC1*01*250*0\r\n"
"PC7*01*CA*0*250\r\n"
"PC7*01*DA*1*100\r\n"
"PC9*01*0*3\r\n"
"PC1*02*270*0\r\n"
"PC7*02*CA*0*270\r\n"
"PC7*02*DA*1*100\r\n"
"PC9*02*1*3\r\n"
"PC1*03*250*0\r\n"
"PC7*03*CA*0*250\r\n"
"PC7*03*DA*1*100\r\n"
"PC9*03*2*3\r\n"
"PC1*04*250*0\r\n"
"PC7*04*CA*0*250\r\n"
"PC7*04*DA*1*100\r\n"
"PC9*04*3*2\r\n"
"PC1*05*270*0\r\n"
"PC7*05*CA*0*270\r\n"
"PC7*05*DA*1*100\r\n"
"PC9*05*4*1\r\n"
"G85*22F5\r\n"
"SE*82*0001\r\n"
"DXE*1*1\r\n";

	initer.start();
	initer.procData((const uint8_t*)data1, sizeof(data1));
	initer.complete();
	TEST_NUMBER_EQUAL(false, initer.hasError());
	TEST_NUMBER_EQUAL(Evadts::Result_PriceListNumberNotEqual, config->comparePlanogram(initer.getPrices(), initer.getProducts()));
	TEST_NUMBER_EQUAL(Evadts::Result_OK, config->resizePlanogram(initer.getPrices(), initer.getProducts()));

	parser.start();
	parser.procData((const uint8_t*)data1, sizeof(data1));
	parser.complete();
	TEST_NUMBER_EQUAL(false, parser.hasError());

	Buffer buf(4000);
	generator.reset();
	buf.add(generator.getData(), generator.getLen());
	while(generator.isLast() == false) {
		generator.next();
		buf.add(generator.getData(), generator.getLen());
	}

	const char expected1[] =
"DXS*EPHOR00001*VA*V0/6*1\r\n"
"ST*001*0001\r\n"
"IC1******0\r\n"
"IC4*1*1643**1*50000*0*0*2*1*0*1*0*0*0*0*0*1\r\n"
"AC2**internet*gdata*gdata*1*0*0*1*\r\n"
"FC1*0*1**0**0\r\n"
"FC2**0**\r\n"
"FC3*\r\n"
"FC4*\r\n"
"FC5*\r\n"
"PC1*01*250*0\r\n"
"PC9*01*0*3*\r\n"
"PC7*01*CA*0*250\r\n"
"PC7*01*DA*1*100\r\n"
"PC1*02*270*0\r\n"
"PC9*02*1*3*\r\n"
"PC7*02*CA*0*270\r\n"
"PC7*02*DA*1*100\r\n"
"PC1*03*250*0\r\n"
"PC9*03*2*3*\r\n"
"PC7*03*CA*0*250\r\n"
"PC7*03*DA*1*100\r\n"
"PC1*04*250*0\r\n"
"PC9*04*3*2*\r\n"
"PC7*04*CA*0*250\r\n"
"PC7*04*DA*1*100\r\n"
"PC1*05*270*0\r\n"
"PC9*05*4*1*\r\n"
"PC7*05*CA*0*270\r\n"
"PC7*05*DA*1*100\r\n"
"MC5*0*INTERNET*1\r\n"
"MC5*1*EXT1*0\r\n"
"MC5*2*EXT2*0\r\n"
"MC5*3*USB1*0\r\n"
"MC5*4*QRTYPE*0\r\n"
"MC5*5*ETH1MAC*2089846A9601\r\n"
"MC5*6*ETH1ADDR*192.168.1.200\r\n"
"MC5*7*ETH1MASK*255.255.255.0\r\n"
"MC5*8*ETH1GW*192.168.1.1\r\n"
"G85*4DED\r\n"
"SE*40*0001\r\n"
"DXE*1*1\r\n";
	TEST_SUBSTR_EQUAL(expected1, (const char *)buf.getData(), buf.getLen());
	return true;
}

bool Config3GeneratorTest::testWithCerts() {
	Config3ConfigIniter initer;
	Config3ConfigParser parser(config);
	Config3ConfigGenerator generator(config);

	MdbCoinChangerContext *coins = config->getAutomat()->getCCContext();
	uint8_t coinInitData[] = { 1, 2, 5, 10 };
	uint8_t coinUpdateData[] = { 67, 54, 114, 51 };
	coins->init(1, 1, 10, coinInitData, sizeof(coinInitData), 0x000F);
	coins->update(0x0005, coinUpdateData, sizeof(coinUpdateData));

	const char data1[] =
"DXS*EPHOR00001*VA*V0/6*1\r\n"
"ST*001*0001\r\n"
"IC1******1001\r\n"
"IC4*2***1*30000\r\n"
"AC2*****3\r\n"
"FC1*5*2*apip.orangedata.ru*12001*91.107.67.212*7790\r\n"
//"FC2*012345678901*1001*Тестовая точка*Тестовый адрес\r\n"
"FC2*012345678901*1001*Test point*Test address\r\n"
"FC3*-----BEGIN CERTIFICATE-----\r\n"
"FC3*MIIDYjCCAkoCAQAwDQYJKoZIhvcNAQELBQAwcTELMAkGA1UEBhMCUlUxDzANBgNV\r\n"
"FC3*BAgMBk1vc2NvdzEPMA0GA1UEBwwGTW9zY293MRMwEQYDVQQKDApPcmFuZ2VkYXRh\r\n"
"FC3*MQ8wDQYDVQQLDAZOZWJ1bGExGjAYBgNVBAMMEXd3dy5vcmFuZ2VkYXRhLnJ1MB4X\r\n"
"FC3*DTE4MDMxNTE2NDYwMVoXDTI4MDMxMjE2NDYwMVowfTELMAkGA1UEBhMCUlUxDzAN\r\n"
"FC3*BgNVBAgMBk1vc2NvdzEPMA0GA1UEBwwGTW9zY293MR8wHQYDVQQKDBZPcmFuZ2Vk\r\n"
"FC3*YXRhIHRlc3QgY2xpZW50MRMwEQYDVQQLDApFLWNvbW1lcmNlMRYwFAYDVQQDDA1v\r\n"
"FC3*cmFuZ2VkYXRhLnJ1MIIBIjANBgkqhkiG9w0BAQEFAAOCAQ8AMIIBCgKCAQEAo7XZ\r\n"
"FC3*+VUUo9p+Q0zPmlt1eThA8NmVVAgNXkVDZoz3umyEnnm2d4R5Voxf4y6fuesW3Za8\r\n"
"FC3*/ImKWLbQ3/S/pHZKWiz75ElSfpnYJfMRuLAaqqs0eFfxmHbHi8Mgg9zjAMdILpR6\r\n"
"FC3*eEaP7qeCNRom3Zb6ziYoWEmDC2ZFFu9995rjkn7CtV3noWZveOCGExjM7WTkql8L\r\n"
"FC3*v1PX3ee3fXaEC7Kefxl4O/4w7agEceKRHlc0l3iwVJaKittQwAQd3ieUwoqsxzPH\r\n"
"FC3*dRwB4IU9aI6IjfqteyD51s7xd+ayM/O4j+aJ/HBhJajDHBcGWKytxv0f6YpqPUAc\r\n"
"FC3*25fRAXVa0Gsei6eY/QIDAQABMA0GCSqGSIb3DQEBCwUAA4IBAQCv/Vcxh2lMt8RV\r\n"
"FC3*Al0V9xIst0ZdjH22yTOUCOiH9PZgeagqrjTLT3ycWAdbZZUpzcFSdOmPUsgQ7Eqz\r\n"
"FC3*+TpcY5lmYFInLwJK/Afjqsb5LK2irGKT254p5qzD9rSRlM42wxRzQTA0BWX3mmhi\r\n"
"FC3*zwdrfLAvyCw1gHBbUZNf3eemBCY+8RRGPRAqD2XbyIya1bX0AHLXbx5dBe9EIOG/\r\n"
"FC3*F46WbTlrkR7kc06eiacTiGYwNdcywJ2KOcvmnXPup8Os6KOWe197CIathDHeiG2C\r\n"
"FC3*mQlsQDF/d7W4G/+l6Q66BhfRtuhp99gkT8P8j82X6ChrwbgQ5+vya3SytJ0wmIg2\r\n"
"FC3*67jOKmGK\r\n"
"FC3*-----END CERTIFICATE-----\r\n"
"FC4*-----BEGIN PRIVATE KEY-----\r\n"
"FC4*MIIEvQIBADANBgkqhkiG9w0BAQEFAASCBKcwggSjAgEAAoIBAQCjtdn5VRSj2n5D\r\n"
"FC4*TM+aW3V5OEDw2ZVUCA1eRUNmjPe6bISeebZ3hHlWjF/jLp+56xbdlrz8iYpYttDf\r\n"
"FC4*9L+kdkpaLPvkSVJ+mdgl8xG4sBqqqzR4V/GYdseLwyCD3OMAx0gulHp4Ro/up4I1\r\n"
"FC4*GibdlvrOJihYSYMLZkUW7333muOSfsK1XeehZm944IYTGMztZOSqXwu/U9fd57d9\r\n"
"FC4*doQLsp5/GXg7/jDtqARx4pEeVzSXeLBUloqK21DABB3eJ5TCiqzHM8d1HAHghT1o\r\n"
"FC4*joiN+q17IPnWzvF35rIz87iP5on8cGElqMMcFwZYrK3G/R/pimo9QBzbl9EBdVrQ\r\n"
"FC4*ax6Lp5j9AgMBAAECggEAL5qkrKT54H+bcZR3Vco8iag68g5DJvFEeeIoLDzXmGUP\r\n"
"FC4*10lLLsvdwLYG9/fJyHU86+h2QfT4vr1CVa1EwN0I19n20TYk/91ahgZ9Y7gJuREZ\r\n"
"FC4*q9jeztfTRKfT36Quej54ldrlFe5m0h3xdeGJ5auOeL2Nw8Z0ja8KbhXsCkEG5cTx\r\n"
"FC4*ZvXB0XlFoAJOp8AZvU3ZNBpmpItFlcl2aBXwRCb72DUjLkpnZf2kFDNorc1wFZ2e\r\n"
"FC4*DO/pujT6EtQ1r5qb2kUuj4GpCaHffOB/ukz3dg3bBhompTYdhax0RlZs2vNsUusm\r\n"
"FC4*6oYsUS5nWmJfnrh32Te03Fdzc2U8/XUflJzKL/0QvQKBgQDOpNQvCCxwvthZXART\r\n"
"FC4*q0fl9NY0fxlSqUpxd1BB4DYCg6Sg5kVvfwf7rdb5bbP4aNCC/9m4MgXTD0DGfEhM\r\n"
"FC4*FnYPVNKTzwLMBftBQdzDN6766j5lI49evwnh855EFAR5GyaIWh2n7tT3NUOstogp\r\n"
"FC4*kpwhzsPGH1WkEO1QLcBDyzPI3wKBgQDKz94V8au1EVKuRBR+c5gNJpF+zmUu2t2C\r\n"
"FC4*ZlPtYIuWaxMbqitmeCmNBQQZK+oLQdSUMkgMvYVpKriPk6AgnY7+1F+OOeg+ezPU\r\n"
"FC4*G+J4Vi8Yx/kZPhXoBuW745twux+q8WOBwEj2WeMy5p1F/V3qlu70HA3kbsrXdB+R\r\n"
"FC4*0bFVAxCtowKBgFTtq4M08cbYuORpDCIzGBarvMnQnuC5US43IlYgxzHbVvMGEO2V\r\n"
"FC4*IPvQY7UZ4EitE11zt9CbRoeLEk1BURlsddMxQmabQwQFRVF5tzjIjvLzCPfaWJdR\r\n"
"FC4*Hsetr5M9QuVfQkPx/ZRCdWawjoLSdj3X0rGWYCHySOloR5CXbRiv0DWzAoGAF3XW\r\n"
"FC4*Ldmn0Ckx1EDB0iLS+up0OCPt5m6g4v2tRa8+VmcKbc/Qd2j8/XgQEk1XJHg3+/CZ\r\n"
"FC4*Dwg5T4IGmW0tP7iaGvY8G3qtV9TumOGk3+CwUACJ2xaoeA+cMZDRoUe0ERUdOpwg\r\n"
"FC4*lIavVmsA1GDLpWBSQeCg5sS+KBAhur9z8O6K1lsCgYEAj7TLLE0jLNXRRfkfWzy5\r\n"
"FC4*RsJezMCQS9fjtJrLGB3BbYxqtebP2owp1qjmKMQioW5QjRxRCOyT2KrHjb31hRsp\r\n"
"FC4*Hk3Wi0OKOEuKNwmAZczbjcPH4caPZPeL6LMDtFFMsFX2BW7TnC8FcoVr2KPO/FG/\r\n"
"FC4*xs4KtXC9j5rrvBowJ0LbJ2U=\r\n"
"FC4*-----END PRIVATE KEY-----\r\n"
"FC5*-----BEGIN RSA PRIVATE KEY-----\r\n"
"FC5*MIIEvQIBADANBgkqhkiG9w0BAQEFAASCBKcwggSjAgEAAoIBAQC3ycL8S2HxRptB\r\n"
"FC5*te7yl2uje/s2pRqdXxj6D3ZiBPvPXGqQEtEddnWC6aXc/GuqM1f0C86a7xH6poo7\r\n"
"FC5*Id8lbQ9xEMvMKghRwc0DCkM78TmPpYBosi/uACNO3Kv2QkH2t8lqlqtWIk1m7dFJ\r\n"
"FC5*RgZO9XOc6Zcx/stM5MxHoc//kfVM/mfWDj4FsuYL0SGNR/Z40WrBkGo+3PJsFvqN\r\n"
"FC5*ocFFonRd0TeWHY54T384XQG0vCJg8MqxVPEh6Rs1/uX8NETL5htQ7FAtx54deu9t\r\n"
"FC5*guIZZ5w/RrsKocaP1k1jWglOErcDCtJ3jIdr1afH8ZplQ21a53UFo/2DexVf6xFX\r\n"
"FC5*3G2cj3p3AgMBAAECggEAPUfM+Aq6kZSVWAetsL3EajKAxOuwQCDhVx+ovW4j+DQ8\r\n"
"FC5*Y+WiTEyfShNV9qVD0PBltz3omch1GjpFhQn6OaRvraeIDH9HXttb3FOjr2zzYG4y\r\n"
"FC5*rrYbPSRWoYj63ZWiIP2O7zdl0caGQHezfNcYa2N0NTG99DGc3/q6EnhlvjWQsSbi\r\n"
"FC5*EjmxcPx8fmV1i4DoflMQ383nsixAFapgrROUAtCgMvhWn1kSeoojKd+e4eKZxa/S\r\n"
"FC5*NYulsBJWNFkmo1CZH4YTqlPM+IwYeDUOnOUGNxGurRZ3qQdWs2N2ZQhnrvlh+zpz\r\n"
"FC5*urD2hwAz6gQXP7mxxMR1xHtAD8XQ+w4OiJK6VWjoIQKBgQDdZJvvZrV6tvqNwuTJ\r\n"
"FC5*kDZjbVU0iKkbP61rVE/6JpyzfGeS0WzGBNiCpbK3pJZnatK2nS7i9v8gAfIqGAk8\r\n"
"FC5*1NRKLa7Qbjgw6xHEwL8VZMXzN3KsMXgGM8EziPzicCYT8VBi/kXyV0ORqRz3rMQ+\r\n"
"FC5*JOTkWRrcw943yYyTr84Dn0l0XQKBgQDUhFWJ3lKwOs7AlAAQqR1PjfpcRvSxVZ70\r\n"
"FC5*BxTwnJoIQQyPQ0/OjCc1sit5s+h8xh0MeKSilCmvZerFlgNtvsCd6geSERXbpN+k\r\n"
"FC5*9Vs3jAEkVeKHeUA/afmGqGCocanlarYu7uNRLfvpG7DduHBb4yJale/XGExNnwC0\r\n"
"FC5*N+dkUU284wKBgBaOSojQiQrQm6RXx+F1TOVCXVz102zQRwXZWDCfQHXU5eSCa7ed\r\n"
"FC5*BMYCxbuKDDzLGF68kutSyNlk+VwqiL5m3J4WG2pm4FizimLmVFGEq9pEuu0qORVA\r\n"
"FC5*rp1mhoU3cdm0S0FasJupIlwzw5zEQFYogh11qpP1bK14XlcpoS6jSuONAoGBAJqM\r\n"
"FC5*EljM4X1fhvPtrY5wLeyo56UrxM8h4RK+A7Bncm0GQUf+P4+JxQn7pDpBZ5U1zfI/\r\n"
"FC5*2hqRfS8dAvrl+WBaFGHCy/ahji/JWwrvk4J1wm7WNoMm3l4/h0MyN/jHkDJSxGKl\r\n"
"FC5*P5LNyiDgDmNvueZY66bM2zqlZPgd5bkp3pDJv6rZAoGAaP5e5F1j6s82Pm7dCpH3\r\n"
"FC5*mRZWnfZIKqoNQIq2BO8vA9/WrdFI2C27uNhxCp2ZDMulRdBZcoeHcwJjnyDzg4I4\r\n"
"FC5*gBZ2nSKkVdlN1REoTjLBBdlHi8XKiXzxvpItc2wjNC2AKHaJqj/dnh3bbTAQD1iU\r\n"
"FC5*AxPmmLJYYkhfZ2i1IrTVxZE=\r\n"
"FC5*-----END RSA PRIVATE KEY-----\r\n"
"LC2*CA*1*31*17:00:00*7200\r\n"
"PC1*1*2000*Coffee\r\n"
"PC7*1*CA*0*2000\r\n"
"PC7*1*CA*1*1500\r\n"
"PC7*1*DA*1*3000\r\n"
"PC9*1*1*3*\r\n"
"PC1*2*1000*Tea\r\n"
"PC7*2*CA*0*1000\r\n"
"PC7*2*CA*1*500\r\n"
"PC7*2*DA*1*2000\r\n"
"PC9*2*2*3*\r\n"
"G85*ABCD\r\n"
"SE*96*0001\r\n"
"DXE*1*1\r\n";

	initer.start();
	initer.procData((const uint8_t*)data1, sizeof(data1));
	initer.complete();
	TEST_NUMBER_EQUAL(false, initer.hasError());
	TEST_NUMBER_EQUAL(Evadts::Result_PriceListNumberNotEqual, config->comparePlanogram(initer.getPrices(), initer.getProducts()));
	TEST_NUMBER_EQUAL(Evadts::Result_OK, config->resizePlanogram(initer.getPrices(), initer.getProducts()));

	parser.start();
	parser.procData((const uint8_t*)data1, sizeof(data1));
	parser.complete();
	TEST_NUMBER_EQUAL(false, parser.hasError());

	Buffer buf(8000);
	generator.reset();
	buf.add(generator.getData(), generator.getLen());
	while(generator.isLast() == false) {
		generator.next();
		buf.add(generator.getData(), generator.getLen());
	}

	const char expected1[] =
"DXS*EPHOR00001*VA*V0/6*1\r\n"
"ST*001*0001\r\n"
"IC1******1001\r\n"
"IC4*2*1643**1*30000*0*0*2*1*0*1*0*0*0*0*0*1\r\n"
"AC2**internet*gdata*gdata*3*0*0*1*\r\n"
"FC1*5*2*apip.orangedata.ru*12001*91.107.67.212*7790\r\n"
"FC2*012345678901*1001*Test point*Test address\r\n"
"FC3*-----BEGIN CERTIFICATE-----\r\n"
"FC3*MIIDYjCCAkoCAQAwDQYJKoZIhvcNAQELBQAwcTELMAkGA1UEBhMCUlUxDzANBgNV\r\n"
"FC3*BAgMBk1vc2NvdzEPMA0GA1UEBwwGTW9zY293MRMwEQYDVQQKDApPcmFuZ2VkYXRh\r\n"
"FC3*MQ8wDQYDVQQLDAZOZWJ1bGExGjAYBgNVBAMMEXd3dy5vcmFuZ2VkYXRhLnJ1MB4X\r\n"
"FC3*DTE4MDMxNTE2NDYwMVoXDTI4MDMxMjE2NDYwMVowfTELMAkGA1UEBhMCUlUxDzAN\r\n"
"FC3*BgNVBAgMBk1vc2NvdzEPMA0GA1UEBwwGTW9zY293MR8wHQYDVQQKDBZPcmFuZ2Vk\r\n"
"FC3*YXRhIHRlc3QgY2xpZW50MRMwEQYDVQQLDApFLWNvbW1lcmNlMRYwFAYDVQQDDA1v\r\n"
"FC3*cmFuZ2VkYXRhLnJ1MIIBIjANBgkqhkiG9w0BAQEFAAOCAQ8AMIIBCgKCAQEAo7XZ\r\n"
"FC3*+VUUo9p+Q0zPmlt1eThA8NmVVAgNXkVDZoz3umyEnnm2d4R5Voxf4y6fuesW3Za8\r\n"
"FC3*/ImKWLbQ3/S/pHZKWiz75ElSfpnYJfMRuLAaqqs0eFfxmHbHi8Mgg9zjAMdILpR6\r\n"
"FC3*eEaP7qeCNRom3Zb6ziYoWEmDC2ZFFu9995rjkn7CtV3noWZveOCGExjM7WTkql8L\r\n"
"FC3*v1PX3ee3fXaEC7Kefxl4O/4w7agEceKRHlc0l3iwVJaKittQwAQd3ieUwoqsxzPH\r\n"
"FC3*dRwB4IU9aI6IjfqteyD51s7xd+ayM/O4j+aJ/HBhJajDHBcGWKytxv0f6YpqPUAc\r\n"
"FC3*25fRAXVa0Gsei6eY/QIDAQABMA0GCSqGSIb3DQEBCwUAA4IBAQCv/Vcxh2lMt8RV\r\n"
"FC3*Al0V9xIst0ZdjH22yTOUCOiH9PZgeagqrjTLT3ycWAdbZZUpzcFSdOmPUsgQ7Eqz\r\n"
"FC3*+TpcY5lmYFInLwJK/Afjqsb5LK2irGKT254p5qzD9rSRlM42wxRzQTA0BWX3mmhi\r\n"
"FC3*zwdrfLAvyCw1gHBbUZNf3eemBCY+8RRGPRAqD2XbyIya1bX0AHLXbx5dBe9EIOG/\r\n"
"FC3*F46WbTlrkR7kc06eiacTiGYwNdcywJ2KOcvmnXPup8Os6KOWe197CIathDHeiG2C\r\n"
"FC3*mQlsQDF/d7W4G/+l6Q66BhfRtuhp99gkT8P8j82X6ChrwbgQ5+vya3SytJ0wmIg2\r\n"
"FC3*67jOKmGK\r\n"
"FC3*-----END CERTIFICATE-----\r\n"
"FC4*-----BEGIN PRIVATE KEY-----\r\n"
"FC4*MIIEvQIBADANBgkqhkiG9w0BAQEFAASCBKcwggSjAgEAAoIBAQCjtdn5VRSj2n5D\r\n"
"FC4*TM+aW3V5OEDw2ZVUCA1eRUNmjPe6bISeebZ3hHlWjF/jLp+56xbdlrz8iYpYttDf\r\n"
"FC4*9L+kdkpaLPvkSVJ+mdgl8xG4sBqqqzR4V/GYdseLwyCD3OMAx0gulHp4Ro/up4I1\r\n"
"FC4*GibdlvrOJihYSYMLZkUW7333muOSfsK1XeehZm944IYTGMztZOSqXwu/U9fd57d9\r\n"
"FC4*doQLsp5/GXg7/jDtqARx4pEeVzSXeLBUloqK21DABB3eJ5TCiqzHM8d1HAHghT1o\r\n"
"FC4*joiN+q17IPnWzvF35rIz87iP5on8cGElqMMcFwZYrK3G/R/pimo9QBzbl9EBdVrQ\r\n"
"FC4*ax6Lp5j9AgMBAAECggEAL5qkrKT54H+bcZR3Vco8iag68g5DJvFEeeIoLDzXmGUP\r\n"
"FC4*10lLLsvdwLYG9/fJyHU86+h2QfT4vr1CVa1EwN0I19n20TYk/91ahgZ9Y7gJuREZ\r\n"
"FC4*q9jeztfTRKfT36Quej54ldrlFe5m0h3xdeGJ5auOeL2Nw8Z0ja8KbhXsCkEG5cTx\r\n"
"FC4*ZvXB0XlFoAJOp8AZvU3ZNBpmpItFlcl2aBXwRCb72DUjLkpnZf2kFDNorc1wFZ2e\r\n"
"FC4*DO/pujT6EtQ1r5qb2kUuj4GpCaHffOB/ukz3dg3bBhompTYdhax0RlZs2vNsUusm\r\n"
"FC4*6oYsUS5nWmJfnrh32Te03Fdzc2U8/XUflJzKL/0QvQKBgQDOpNQvCCxwvthZXART\r\n"
"FC4*q0fl9NY0fxlSqUpxd1BB4DYCg6Sg5kVvfwf7rdb5bbP4aNCC/9m4MgXTD0DGfEhM\r\n"
"FC4*FnYPVNKTzwLMBftBQdzDN6766j5lI49evwnh855EFAR5GyaIWh2n7tT3NUOstogp\r\n"
"FC4*kpwhzsPGH1WkEO1QLcBDyzPI3wKBgQDKz94V8au1EVKuRBR+c5gNJpF+zmUu2t2C\r\n"
"FC4*ZlPtYIuWaxMbqitmeCmNBQQZK+oLQdSUMkgMvYVpKriPk6AgnY7+1F+OOeg+ezPU\r\n"
"FC4*G+J4Vi8Yx/kZPhXoBuW745twux+q8WOBwEj2WeMy5p1F/V3qlu70HA3kbsrXdB+R\r\n"
"FC4*0bFVAxCtowKBgFTtq4M08cbYuORpDCIzGBarvMnQnuC5US43IlYgxzHbVvMGEO2V\r\n"
"FC4*IPvQY7UZ4EitE11zt9CbRoeLEk1BURlsddMxQmabQwQFRVF5tzjIjvLzCPfaWJdR\r\n"
"FC4*Hsetr5M9QuVfQkPx/ZRCdWawjoLSdj3X0rGWYCHySOloR5CXbRiv0DWzAoGAF3XW\r\n"
"FC4*Ldmn0Ckx1EDB0iLS+up0OCPt5m6g4v2tRa8+VmcKbc/Qd2j8/XgQEk1XJHg3+/CZ\r\n"
"FC4*Dwg5T4IGmW0tP7iaGvY8G3qtV9TumOGk3+CwUACJ2xaoeA+cMZDRoUe0ERUdOpwg\r\n"
"FC4*lIavVmsA1GDLpWBSQeCg5sS+KBAhur9z8O6K1lsCgYEAj7TLLE0jLNXRRfkfWzy5\r\n"
"FC4*RsJezMCQS9fjtJrLGB3BbYxqtebP2owp1qjmKMQioW5QjRxRCOyT2KrHjb31hRsp\r\n"
"FC4*Hk3Wi0OKOEuKNwmAZczbjcPH4caPZPeL6LMDtFFMsFX2BW7TnC8FcoVr2KPO/FG/\r\n"
"FC4*xs4KtXC9j5rrvBowJ0LbJ2U=\r\n"
"FC4*-----END PRIVATE KEY-----\r\n"
"FC5*-----BEGIN RSA PRIVATE KEY-----\r\n"
"FC5*MIIEvQIBADANBgkqhkiG9w0BAQEFAASCBKcwggSjAgEAAoIBAQC3ycL8S2HxRptB\r\n"
"FC5*te7yl2uje/s2pRqdXxj6D3ZiBPvPXGqQEtEddnWC6aXc/GuqM1f0C86a7xH6poo7\r\n"
"FC5*Id8lbQ9xEMvMKghRwc0DCkM78TmPpYBosi/uACNO3Kv2QkH2t8lqlqtWIk1m7dFJ\r\n"
"FC5*RgZO9XOc6Zcx/stM5MxHoc//kfVM/mfWDj4FsuYL0SGNR/Z40WrBkGo+3PJsFvqN\r\n"
"FC5*ocFFonRd0TeWHY54T384XQG0vCJg8MqxVPEh6Rs1/uX8NETL5htQ7FAtx54deu9t\r\n"
"FC5*guIZZ5w/RrsKocaP1k1jWglOErcDCtJ3jIdr1afH8ZplQ21a53UFo/2DexVf6xFX\r\n"
"FC5*3G2cj3p3AgMBAAECggEAPUfM+Aq6kZSVWAetsL3EajKAxOuwQCDhVx+ovW4j+DQ8\r\n"
"FC5*Y+WiTEyfShNV9qVD0PBltz3omch1GjpFhQn6OaRvraeIDH9HXttb3FOjr2zzYG4y\r\n"
"FC5*rrYbPSRWoYj63ZWiIP2O7zdl0caGQHezfNcYa2N0NTG99DGc3/q6EnhlvjWQsSbi\r\n"
"FC5*EjmxcPx8fmV1i4DoflMQ383nsixAFapgrROUAtCgMvhWn1kSeoojKd+e4eKZxa/S\r\n"
"FC5*NYulsBJWNFkmo1CZH4YTqlPM+IwYeDUOnOUGNxGurRZ3qQdWs2N2ZQhnrvlh+zpz\r\n"
"FC5*urD2hwAz6gQXP7mxxMR1xHtAD8XQ+w4OiJK6VWjoIQKBgQDdZJvvZrV6tvqNwuTJ\r\n"
"FC5*kDZjbVU0iKkbP61rVE/6JpyzfGeS0WzGBNiCpbK3pJZnatK2nS7i9v8gAfIqGAk8\r\n"
"FC5*1NRKLa7Qbjgw6xHEwL8VZMXzN3KsMXgGM8EziPzicCYT8VBi/kXyV0ORqRz3rMQ+\r\n"
"FC5*JOTkWRrcw943yYyTr84Dn0l0XQKBgQDUhFWJ3lKwOs7AlAAQqR1PjfpcRvSxVZ70\r\n"
"FC5*BxTwnJoIQQyPQ0/OjCc1sit5s+h8xh0MeKSilCmvZerFlgNtvsCd6geSERXbpN+k\r\n"
"FC5*9Vs3jAEkVeKHeUA/afmGqGCocanlarYu7uNRLfvpG7DduHBb4yJale/XGExNnwC0\r\n"
"FC5*N+dkUU284wKBgBaOSojQiQrQm6RXx+F1TOVCXVz102zQRwXZWDCfQHXU5eSCa7ed\r\n"
"FC5*BMYCxbuKDDzLGF68kutSyNlk+VwqiL5m3J4WG2pm4FizimLmVFGEq9pEuu0qORVA\r\n"
"FC5*rp1mhoU3cdm0S0FasJupIlwzw5zEQFYogh11qpP1bK14XlcpoS6jSuONAoGBAJqM\r\n"
"FC5*EljM4X1fhvPtrY5wLeyo56UrxM8h4RK+A7Bncm0GQUf+P4+JxQn7pDpBZ5U1zfI/\r\n"
"FC5*2hqRfS8dAvrl+WBaFGHCy/ahji/JWwrvk4J1wm7WNoMm3l4/h0MyN/jHkDJSxGKl\r\n"
"FC5*P5LNyiDgDmNvueZY66bM2zqlZPgd5bkp3pDJv6rZAoGAaP5e5F1j6s82Pm7dCpH3\r\n"
"FC5*mRZWnfZIKqoNQIq2BO8vA9/WrdFI2C27uNhxCp2ZDMulRdBZcoeHcwJjnyDzg4I4\r\n"
"FC5*gBZ2nSKkVdlN1REoTjLBBdlHi8XKiXzxvpItc2wjNC2AKHaJqj/dnh3bbTAQD1iU\r\n"
"FC5*AxPmmLJYYkhfZ2i1IrTVxZE=\r\n"
"FC5*-----END RSA PRIVATE KEY-----\r\n"
"LC2*CA*1*31*17:0:0*7200\r\n"
"PC1*1*2000*Coffee\r\n"
"PC9*1*1*3*\r\n"
"PC7*1*CA*0*2000\r\n"
"PC7*1*CA*1*1500\r\n"
"PC7*1*DA*1*3000\r\n"
"PC1*2*1000*Tea\r\n"
"PC9*2*2*3*\r\n"
"PC7*2*CA*0*1000\r\n"
"PC7*2*CA*1*500\r\n"
"PC7*2*DA*1*2000\r\n"
"MC5*0*INTERNET*1\r\n"
"MC5*1*EXT1*0\r\n"
"MC5*2*EXT2*0\r\n"
"MC5*3*USB1*0\r\n"
"MC5*4*QRTYPE*0\r\n"
"MC5*5*ETH1MAC*2089846A9601\r\n"
"MC5*6*ETH1ADDR*192.168.1.200\r\n"
"MC5*7*ETH1MASK*255.255.255.0\r\n"
"MC5*8*ETH1GW*192.168.1.1\r\n"
"G85*AFCF\r\n"
"SE*105*0001\r\n"
"DXE*1*1\r\n";
	TEST_SUBSTR_EQUAL(expected1, (const char *)buf.getData(), buf.getLen());

	return true;
}

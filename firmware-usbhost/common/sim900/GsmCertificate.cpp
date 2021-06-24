#include "GsmCertificate.h"

#include "utils/include/Event.h"
#include "utils/include/StringParser.h"
#include "timer/include/TimerEngine.h"
//#include "extern/tomcrypt/src/headers/tomcrypt.h"
#include "logger/include/Logger.h"

#include <string.h>

#define AT_FSWRITE_TIMEOUT 	20000

const char data[] =
#if 0
"Bag Attributes\r\n"
"	localKeyID: 84 B6 71 BD 8E A4 13 DB 61 09 92 98 7C C7 99 36 D3 3A 3C 74\r\n"
"subject=/C=RU/ST=Moscow/L=Moscow/O=Orangedata test client/OU=E-commerce/CN=orangedata.ru\r\n"
"issuer=/C=RU/ST=Moscow/L=Moscow/O=Orangedata/OU=Nebula/CN=www.orangedata.ru\r\n"
"-----BEGIN CERTIFICATE-----\r\n"
"MIIDYjCCAkoCAQAwDQYJKoZIhvcNAQELBQAwcTELMAkGA1UEBhMCUlUxDzANBgNV\r\n"
"BAgMBk1vc2NvdzEPMA0GA1UEBwwGTW9zY293MRMwEQYDVQQKDApPcmFuZ2VkYXRh\r\n"
"MQ8wDQYDVQQLDAZOZWJ1bGExGjAYBgNVBAMMEXd3dy5vcmFuZ2VkYXRhLnJ1MB4X\r\n"
"DTE4MDMxNTE2NDYwMVoXDTI4MDMxMjE2NDYwMVowfTELMAkGA1UEBhMCUlUxDzAN\r\n"
"BgNVBAgMBk1vc2NvdzEPMA0GA1UEBwwGTW9zY293MR8wHQYDVQQKDBZPcmFuZ2Vk\r\n"
"YXRhIHRlc3QgY2xpZW50MRMwEQYDVQQLDApFLWNvbW1lcmNlMRYwFAYDVQQDDA1v\r\n"
"cmFuZ2VkYXRhLnJ1MIIBIjANBgkqhkiG9w0BAQEFAAOCAQ8AMIIBCgKCAQEAo7XZ\r\n"
"+VUUo9p+Q0zPmlt1eThA8NmVVAgNXkVDZoz3umyEnnm2d4R5Voxf4y6fuesW3Za8\r\n"
"/ImKWLbQ3/S/pHZKWiz75ElSfpnYJfMRuLAaqqs0eFfxmHbHi8Mgg9zjAMdILpR6\r\n"
"eEaP7qeCNRom3Zb6ziYoWEmDC2ZFFu9995rjkn7CtV3noWZveOCGExjM7WTkql8L\r\n"
"v1PX3ee3fXaEC7Kefxl4O/4w7agEceKRHlc0l3iwVJaKittQwAQd3ieUwoqsxzPH\r\n"
"dRwB4IU9aI6IjfqteyD51s7xd+ayM/O4j+aJ/HBhJajDHBcGWKytxv0f6YpqPUAc\r\n"
"25fRAXVa0Gsei6eY/QIDAQABMA0GCSqGSIb3DQEBCwUAA4IBAQCv/Vcxh2lMt8RV\r\n"
"Al0V9xIst0ZdjH22yTOUCOiH9PZgeagqrjTLT3ycWAdbZZUpzcFSdOmPUsgQ7Eqz\r\n"
"+TpcY5lmYFInLwJK/Afjqsb5LK2irGKT254p5qzD9rSRlM42wxRzQTA0BWX3mmhi\r\n"
"zwdrfLAvyCw1gHBbUZNf3eemBCY+8RRGPRAqD2XbyIya1bX0AHLXbx5dBe9EIOG/\r\n"
"F46WbTlrkR7kc06eiacTiGYwNdcywJ2KOcvmnXPup8Os6KOWe197CIathDHeiG2C\r\n"
"mQlsQDF/d7W4G/+l6Q66BhfRtuhp99gkT8P8j82X6ChrwbgQ5+vya3SytJ0wmIg2\r\n"
"67jOKmGK\r\n"
"-----END CERTIFICATE-----\r\n"
"Bag Attributes\r\n"
"	localKeyID: 84 B6 71 BD 8E A4 13 DB 61 09 92 98 7C C7 99 36 D3 3A 3C 74\r\n"
"Key Attributes: <No Attributes>\r\n"
"-----BEGIN ENCRYPTED PRIVATE KEY-----\r\n"
"MIIFHDBOBgkqhkiG9w0BBQ0wQTApBgkqhkiG9w0BBQwwHAQI4VwsP0wLzTgCAggA\r\n"
"MAwGCCqGSIb3DQIJBQAwFAYIKoZIhvcNAwcECESYe+EqFUdrBIIEyGS9z8qf0Hk4\r\n"
"XDlgYuPoYxkC2dHJZQ7oqXKyBlLZIkSmtaDuYaD4uPEGzTAo8iPQFx8ABl2UO5Kx\r\n"
"aKsqmpXleRfoGn58RbWLlbSbtt9QONeT7DvkQDRMKADfkuIV0in1QFyRRVsN+CP8\r\n"
"H1aEU2WIYx0aCGn/n35pnnqOXzTOTdZSoFIT09rk9fEWHGE1fjtvPXk7/Q6fIpCp\r\n"
"GaxQgUhnH7ejth6LEaMquVq7g5ceGmKB14tSzbXmEIrCPRebLsr+H7Aoq8MExAti\r\n"
"ttpM6D9iDrJOrk8MToRS/bfQoLBDo5EblOQS3e3fC4qycvv2e0I3QTNeIfF+axtJ\r\n"
"X0kXjClciGCKLJtqLhcV8QRH8CXmi6lvSxLOvQh1m01+8s1LXEVB5FPBTGni/o+I\r\n"
"XsWafMOmKTH1SE0W+Mr+QvQr8vmdgv+RSx20qVQNJvNNXNfu9dcOowkPAdClfxZc\r\n"
"iJKpzRZfAmkYxsMGg5ueZ+iBaav4ZrKgNoWyPv1WfN/pE9FHvXvEAGnplks1qo9j\r\n"
"uiq3TQ869l3QuqYYoB40z4n/joxGQAf2I3UBthoPG07EwYykZtXTNx5yVdff4LcS\r\n"
"TADKbEvTUPhh/uWye/JO7nZhODLn7CKWTGLZ8WP+IvkSJnalHuzFaaWRVPNbUFFZ\r\n"
"dKgHMa9zyn7shXUJCsSN5KpWy/KcptshCWr5SuM/OLNLIt3cCnS+qZryqM7cdu+u\r\n"
"+dff9/kvjESmXJiLogovm3AS+cJR2k0C1Hq2IrbdbFu1+c9p9fA4UzPh3SpnClRM\r\n"
"8YToybRzVWu65CdJg5vhDIYPzCF640D3tTgnkNBs1uMzWxm8NoYvvsWY6hZCGnCI\r\n"
"8R/5J2S7RJZjaJP2795RXCNbzUdAgLrEpvp+RQg0vPB/W8EOvv/8tSwW36tQGufV\r\n"
"KH+Hi7uDm5CDmB2/yw8E/V5rlzlp5McwTaJG/0LkIcHG9DRSprAENac2JSnQY1lq\r\n"
"hMqOWyQimTzWE3cOCs/Q+ZcgFolGcuvzvPkueqsMHenis+hKHC7uKNiCN/GyA9iJ\r\n"
"mQqBFYn8bLlTDfU2p0ZRDrils8G5ABy9zvIh6wEAin4nhvyKN443FjXQoeihqPOJ\r\n"
"iHQyoRwNZdUtVjBvPF+MCt2uv/6UkLCK7WFpEKgJGLfgp1x5MWlyggdWjqtYkuJs\r\n"
"T5ADZhLjqqmAeRWnkzk2c/pOnaLeNNmqBkepMS9zUss9MNcIWCSK6oP0Kt+yXGAr\r\n"
"dbHlbiVbDLW2+h6pvj3N6Ec/2uOKKn430VFBo/zquGGmSk+3ixDvHEOyK8kkNluP\r\n"
"6DrLgAiTId72OApVrzcKs9YoXMMHk2TWgURNYmYvCE0rpQ0DHgoUt4AszsP4yWQq\r\n"
"32ZBV7oYpX2Jgc9K8npkfH2MPg5bH58U9ApQrREgS+5RHaewCgCY3l3SNT6WkLWb\r\n"
"Ld7bPPcaDcmzRl5aMfqtftpVIGQXZokPaSERf89Fd09MwKLmZfTARL3b+X1sEuuV\r\n"
"4sy4ni3RlORYZapEDL5HMTTHMFJpUx6dOy3wRyK0PXtxvQmEtaeMk0tRUzJ1E4Ux\r\n"
"rrwpJ0tZZ8OW0lVMuSRNoj7InDtlFJRMT8mPFm43CpYJyRHoEkyUn6/Mb6eMttJd\r\n"
"K9nDbF3hWOoTW4IYHha6NA==\r\n"
"-----END ENCRYPTED PRIVATE KEY-----";
#else
"-----BEGIN PRIVATE KEY-----\n"
"MIIEvQIBADANBgkqhkiG9w0BAQEFAASCBKcwggSjAgEAAoIBAQCjtdn5VRSj2n5D\n"
"TM+aW3V5OEDw2ZVUCA1eRUNmjPe6bISeebZ3hHlWjF/jLp+56xbdlrz8iYpYttDf\n"
"9L+kdkpaLPvkSVJ+mdgl8xG4sBqqqzR4V/GYdseLwyCD3OMAx0gulHp4Ro/up4I1\n"
"GibdlvrOJihYSYMLZkUW7333muOSfsK1XeehZm944IYTGMztZOSqXwu/U9fd57d9\n"
"doQLsp5/GXg7/jDtqARx4pEeVzSXeLBUloqK21DABB3eJ5TCiqzHM8d1HAHghT1o\n"
"joiN+q17IPnWzvF35rIz87iP5on8cGElqMMcFwZYrK3G/R/pimo9QBzbl9EBdVrQ\n"
"ax6Lp5j9AgMBAAECggEAL5qkrKT54H+bcZR3Vco8iag68g5DJvFEeeIoLDzXmGUP\n"
"10lLLsvdwLYG9/fJyHU86+h2QfT4vr1CVa1EwN0I19n20TYk/91ahgZ9Y7gJuREZ\n"
"q9jeztfTRKfT36Quej54ldrlFe5m0h3xdeGJ5auOeL2Nw8Z0ja8KbhXsCkEG5cTx\n"
"ZvXB0XlFoAJOp8AZvU3ZNBpmpItFlcl2aBXwRCb72DUjLkpnZf2kFDNorc1wFZ2e\n"
"DO/pujT6EtQ1r5qb2kUuj4GpCaHffOB/ukz3dg3bBhompTYdhax0RlZs2vNsUusm\n"
"6oYsUS5nWmJfnrh32Te03Fdzc2U8/XUflJzKL/0QvQKBgQDOpNQvCCxwvthZXART\n"
"q0fl9NY0fxlSqUpxd1BB4DYCg6Sg5kVvfwf7rdb5bbP4aNCC/9m4MgXTD0DGfEhM\n"
"FnYPVNKTzwLMBftBQdzDN6766j5lI49evwnh855EFAR5GyaIWh2n7tT3NUOstogp\n"
"kpwhzsPGH1WkEO1QLcBDyzPI3wKBgQDKz94V8au1EVKuRBR+c5gNJpF+zmUu2t2C\n"
"ZlPtYIuWaxMbqitmeCmNBQQZK+oLQdSUMkgMvYVpKriPk6AgnY7+1F+OOeg+ezPU\n"
"G+J4Vi8Yx/kZPhXoBuW745twux+q8WOBwEj2WeMy5p1F/V3qlu70HA3kbsrXdB+R\n"
"0bFVAxCtowKBgFTtq4M08cbYuORpDCIzGBarvMnQnuC5US43IlYgxzHbVvMGEO2V\n"
"IPvQY7UZ4EitE11zt9CbRoeLEk1BURlsddMxQmabQwQFRVF5tzjIjvLzCPfaWJdR\n"
"Hsetr5M9QuVfQkPx/ZRCdWawjoLSdj3X0rGWYCHySOloR5CXbRiv0DWzAoGAF3XW\n"
"Ldmn0Ckx1EDB0iLS+up0OCPt5m6g4v2tRa8+VmcKbc/Qd2j8/XgQEk1XJHg3+/CZ\n"
"Dwg5T4IGmW0tP7iaGvY8G3qtV9TumOGk3+CwUACJ2xaoeA+cMZDRoUe0ERUdOpwg\n"
"lIavVmsA1GDLpWBSQeCg5sS+KBAhur9z8O6K1lsCgYEAj7TLLE0jLNXRRfkfWzy5\n"
"RsJezMCQS9fjtJrLGB3BbYxqtebP2owp1qjmKMQioW5QjRxRCOyT2KrHjb31hRsp\n"
"Hk3Wi0OKOEuKNwmAZczbjcPH4caPZPeL6LMDtFFMsFX2BW7TnC8FcoVr2KPO/FG/\n"
"xs4KtXC9j5rrvBowJ0LbJ2U=\n"
"-----END PRIVATE KEY-----\n";
#endif

namespace Gsm {

Certificate::Certificate(TimerEngine *timers, CommandProcessor *commandProcessor) :
	timers(timers),
	commandProcessor(commandProcessor),
	state(State_Idle),
	path(16, 16)
{
	this->timer = timers->addTimer<Certificate, &Certificate::procTimer>(this);
	this->command = new Command(this);
}

Certificate::~Certificate() {
	delete this->command;
	this->timers->deleteTimer(this->timer);
}

void Certificate::setObserver(EventObserver *observer) {
	this->courier.setRecipient(observer);
}

void Certificate::save() {
	LOG_DEBUG(LOG_TCPIP, "save " << state);
	gotoStateFsDrive();
}

void Certificate::procTimer() {
	LOG_DEBUG(LOG_TCPIP, "procTimer " << state);
	switch(state) {
	default: LOG_ERROR(LOG_TCPIP, "Unwaited timeout: " << state);
	}
}

void Certificate::procResponse(Command::Result result, const char *data) {
	LOG_DEBUG(LOG_TCPIP, "procResponse " << state);
	switch(state) {
	case State_FsDrive: stateFsDriveResponse(result, data); break;
	case State_FsCreate: stateFsCreateResponse(result); break;
	case State_FsWrite: stateFsWriteResponse(result); break;
	case State_FsFileSize: stateFsFileSizeResponse(result, data); break;
	case State_FsRead: stateFsReadResponse(result); break;
	case State_SslSetCert: stateSslSetCertResponse(result); break;
	default: LOG_ERROR(LOG_TCPIP, "Unwaited response: " << state << "," << result);
	}
}

void Certificate::procEvent(const char *data) {
	LOG_DEBUG(LOG_TCPIP, "procEvent " << state);
	switch(state) {
	case State_SslSetCertWait: stateSslSetCertWaitEvent(data); break;
	default:;
	}
}

void Certificate::proc(Event *event) {
	(void)event;
	LOG_DEBUG(LOG_TCPIP, "proc");
	switch(state) {
	default:;
	}
}

void Certificate::gotoStateFsDrive() {
	LOG_DEBUG(LOG_TCPIP, "gotoStateFsDrive");
	command->set(Command::Type_Result);
	command->setText() << "AT+FSDRIVE=0";
	if(commandProcessor->execute(command) == false) {
		LOG_ERROR(LOG_SIM, "Write failed");
		return;
	}
	state = State_FsDrive;
}

void Certificate::stateFsDriveResponse(Command::Result result, const char *data) {
	LOG_DEBUG(LOG_TCPIP, "stateFsDriveResponse");
	if(result != Command::Result_OK) {
		LOG_ERROR(LOG_SIM, "Write failed " << result);
		return;
	}
	StringParser parser(data, strlen(data));
	if(parser.compareAndSkip("+FSDRIVE: ") == false) {
		return;
	}
	if(parser.getValue("", (char*)path.getData(), 1) == 0) {
		LOG_ERROR(LOG_SIM, "Write result");
		return;
	}
	path.setLen(1);
	path << ":\\cert.crt";
	gotoStateFsCreate();
}

void Certificate::gotoStateFsCreate() {
	LOG_DEBUG(LOG_TCPIP, "gotoStateFsCreate");
	command->set(Command::Type_Result);
	command->setText() << "AT+FSCREATE=" << path.getString();
	if(commandProcessor->execute(command) == false) {
		LOG_ERROR(LOG_TCPIP, "sendCommand failed");
		return;
	}
	state = State_FsCreate;
}

void Certificate::stateFsCreateResponse(Command::Result result) {
	LOG_DEBUG(LOG_TCPIP, "stateFsCreateResponse");
	if(result != Command::Result_OK) {
		LOG_ERROR(LOG_SIM, "Create failed " << result);
		return;
	}
	gotoStateFsWrite();
}

void Certificate::gotoStateFsWrite() {
#if 0
	LOG_DEBUG(LOG_TCPIP, "gotoStateFsWrite");
	long unsigned int dataLen = strlen(data);
	long unsigned int bufLen = sizeof(buf);
	int err = base64_decode((uint8_t*)data, dataLen, buf, &bufLen);
	if(err != CRYPT_OK) {
		LOG_ERROR(LOG_SIM, "base64_decode failed");
		return;
	}

	LOG_HEX(buf, bufLen);
	command->set(Command::Type_SendDataWithoutEcho, "", AT_FSWRITE_TIMEOUT);
	command->setText() << "AT+FSWRITE=cert.key,0," << bufLen << ",10";
	command->setData(buf, bufLen);
	if(commandProcessor->execute(command) == false) {
		LOG_ERROR(LOG_SIM, "Write failed");
		return;
	}
	state = State_FsWrite;
#else
	LOG_DEBUG(LOG_TCPIP, "gotoStateFsWrite");
	uint16_t i = 0;
	for(; data[i] != '\0'; i++) {
		buf[i] = data[i];
	}
	command->set(Command::Type_SendData, "", AT_FSWRITE_TIMEOUT);
	command->setText() << "AT+FSWRITE=" << path.getString() << ",0," << i << ",20";
	command->setData((uint8_t*)buf, i);
	if(commandProcessor->execute(command) == false) {
		LOG_ERROR(LOG_SIM, "Write failed");
		return;
	}
	state = State_FsWrite;
#endif
}

void Certificate::stateFsWriteResponse(Command::Result result) {
	LOG_DEBUG(LOG_TCPIP, "stateFsWriteResponse");
	if(result != Command::Result_OK) {
		LOG_ERROR(LOG_SIM, "Write failed " << result);
		return;
	}
#if 0
	gotoStateSslSetCert();
#else
	gotoStateFsFileSize();
#endif
}

void Certificate::gotoStateFsFileSize() {
	LOG_DEBUG(LOG_TCPIP, "gotoStateFsFileSize");
	command->set(Command::Type_Result);
	command->setText() << "AT+FSFLSIZE=" << path.getString();
	if(commandProcessor->execute(command) == false) {
		LOG_ERROR(LOG_SIM, "Write failed");
		return;
	}
	state = State_FsFileSize;
}

void Certificate::stateFsFileSizeResponse(Command::Result result, const char *data) {
	LOG_DEBUG(LOG_TCPIP, "stateFsFileSizeResponse");
	if(result != Command::Result_OK) {
		LOG_ERROR(LOG_SIM, "Write failed " << result);
		return;
	}
	StringParser parser(data, strlen(data));
	if(parser.compareAndSkip("+FSFLSIZE: ") == false) {
		return;
	}
	if(parser.getNumber(&filesize) == false) {
		LOG_ERROR(LOG_SIM, "Write result");
		return;
	}
#if 0
	gotoStateSslSetCert();
#else
	gotoStateFsRead();
#endif
}

void Certificate::gotoStateFsRead() {
	LOG_DEBUG(LOG_TCPIP, "gotoStateFsRead");
	command->set(Command::Type_RecvData);
	command->setText() << "AT+FSREAD=" << path.getString() << ",0," << filesize << ",0";
	command->setData(buf, filesize);
	if(commandProcessor->execute(command) == false) {
		LOG_ERROR(LOG_SIM, "Write failed");
		return;
	}
	state = State_FsRead;
}

void Certificate::stateFsReadResponse(Command::Result result) {
	LOG_DEBUG(LOG_TCPIP, "stateFsReadResponse");
	if(result != Command::Result_OK) {
		LOG_ERROR(LOG_SIM, "Write failed " << result);
		return;
	}
	LOG_STR(buf, filesize);
	gotoStateSslSetCert();
}

void Certificate::gotoStateSslSetCert() {
	LOG_DEBUG(LOG_TCPIP, "gotoStateSslSetCert");
	char password[] = "1234";
	command->set(Command::Type_Result);
	command->setText() << "AT+SSLSETCERT=" << path.getString() << "," << password;
	if(commandProcessor->execute(command) == false) {
		LOG_ERROR(LOG_TCPIP, "sendCommand failed");
		return;
	}
	state = State_SslSetCert;
}

void Certificate::stateSslSetCertResponse(Command::Result result) {
	LOG_DEBUG(LOG_TCPIP, "stateSslSetCertResponse");
	if(result != Command::Result_OK) {
		LOG_ERROR(LOG_SIM, "Write failed " << result);
		return;
	}
	gotoStateSslSetCertWait();
}

void Certificate::gotoStateSslSetCertWait() {
	LOG_DEBUG(LOG_TCPIP, "gotoStateSslSetCertWait");
	state = State_SslSetCertWait;
}

void Certificate::stateSslSetCertWaitEvent(const char *data) {
	LOG_DEBUG(LOG_TCPIP, "stateSslSetCertWaitEvent");
	StringParser parser(data, strlen(data));
	if(parser.compareAndSkip("+SSLSETCERT: ") == false) {
		return;
	}
	uint16_t result;
	if(parser.getNumber(&result) == false) {
		LOG_ERROR(LOG_SIM, "Write result");
		return;
	}
	if(result == 0) {
		LOG_ERROR(LOG_SIM, ">>>>>>>>>>>>>>>>>>>SUCCEDD");
	} else {
		LOG_ERROR(LOG_SIM, ">>>>>>>>>>>>>>>>>>>FAILED");
	}
}

}

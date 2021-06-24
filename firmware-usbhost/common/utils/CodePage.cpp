#include "include/CodePage.h"
#include "utils/include/StringParser.h"
#include "utils/include/Hex.h"

#include <ctype.h>

void convertCp866ToWin1251(uint8_t *str, uint16_t strLen) {
	for(uint16_t i = 0; i < strLen; i++) {
		uint8_t c = str[i];
		if(128 <= c && c <= 175) {
			str[i] = c + 64;
		}
		if(224 <= c && c <= 239) {
			str[i] = c + 16;
		}
	}
}

void convertWin1251ToCp866(uint8_t *str, uint16_t strLen) {
	for(uint16_t i = 0; i < strLen; i++) {
		uint8_t c = str[i];
		if(192 <= c && c <= 239) {
			str[i] = c - 64;
		}
		if(240 <= c) {
			str[i] = c - 16;
		}
	}
}

bool stringToVersion(const char *data, uint16_t dataLen, uint32_t *result) {
	StringParser parser(data, dataLen);
	uint32_t digit1 = 0;
	if(parser.getNumber(&digit1) == false) {
		return false;
	}
	parser.skipEqual(".");
	uint32_t digit2 = 0;
	if(parser.getNumber(&digit2) == false) {
		return false;
	}
	parser.skipEqual(".");
	uint32_t digit3 = 0;
	if(parser.getNumber(&digit3) == false) {
		return false;
	}
	*result = ((digit1 << 24) & 0xFF000000) | ((digit2 << 16) & 0x00FF0000) | (digit3 & 0x0000FFFF);
	return true;
}

uint16_t convertWin1251ToUtf8(const uint8_t *src, uint16_t srcLen, uint8_t *dst, uint16_t dstSize) {
	uint16_t d = 0;
	for(uint16_t s = 0; s < srcLen; s++) {
		uint8_t b = src[s];
		if(b < 128) {
			if((dstSize - d) < 1) {
				return d;
			}
			dst[d] = b;
			d++;
		} else {
			if((dstSize - d) < 2) {
				return d;
			}
			if(b >= 0xC0) {
				if(b < 0xF0) {
					dst[d] = 0xD0;
					d++;
					dst[d] = b - 0x30;
					d++;
				} else {
					dst[d] = 0xD1;
					d++;
					dst[d] = b - 0x70;
					d++;
				}
			} else if(b == 0xA8) {
				dst[d] = 0xD0;
				d++;
				dst[d] = 0x81;
				d++;
			} else if(b == 0xB8) {
				dst[d] = 0xD1;
				d++;
				dst[d] = 0x91;
				d++;
			}
		}
	}
	return d;
}

// http://i.voenmeh.ru/kafi5/Kam.loc/inform/UTF-8.htm
uint16_t convertUtf8ToWin1251(const uint8_t *src, uint16_t srcLen, uint8_t *dst, uint16_t dstSize) {
	uint16_t d = 0;
	uint8_t Lex2Mask = 0xE0;
	uint8_t Lex2Prefix = 0xC0;
	for(uint16_t s = 0; s < srcLen && d < dstSize; s++) {
		uint8_t b1 = src[s];
		if(b1 < 128) {
			dst[d] = b1;
			d++;
		} else if((b1 & Lex2Mask) == Lex2Prefix) {
			if((srcLen - s) < 2) {
				return d;
			}
			s++;
			uint8_t b2 = src[s];
			if(b1 == 0xD0) {
				if(b2 == 0x81) {
					dst[d] = 0xA8;
					d++;
				} else {
					dst[d] = b2 + 0x30;
					d++;
				}
			} else if(b1 == 0xD1) {
				if(b2 == 0x91) {
					dst[d] = 0xB8;
					d++;
				} else {
					dst[d] = b2 + 0x70;
					d++;
				}
			}
		}
	}
	return d;
}

void convertUtf8ToWin1251(const uint8_t *src, uint16_t srcLen, StringBuilder *dst) {
	uint8_t Lex2Mask = 0xE0;
	uint8_t Lex2Prefix = 0xC0;
	for(uint16_t s = 0; s < srcLen; s++) {
		uint8_t b1 = src[s];
		if(b1 < 128) {
			dst->add(b1);
		} else if((b1 & Lex2Mask) == Lex2Prefix) {
			if((srcLen - s) < 2) {
				return;
			}
			s++;
			uint8_t b2 = src[s];
			if(b1 == 0xD0) {
				if(b2 == 0x81) {
					dst->add(0xA8);
				} else {
					dst->add(b2 + 0x30);
				}
			} else if(b1 == 0xD1) {
				if(b2 == 0x91) {
					dst->add(0xB8);
				} else {
					dst->add(b2 + 0x70);
				}
			}
		}
	}
}

const uint16_t win1251ToUnicode[] = {
	0x0000, 0x0001, 0x0002, 0x0003, 0x0004, 0x0005, 0x0006, 0x0007,
	0x0008, 0x0009, 0x000A, 0x000B, 0x000C, 0x000D, 0x000E, 0x000F,
	0x0010, 0x0011, 0x0012, 0x0013, 0x0014, 0x0015, 0x0016, 0x0017,
	0x0018, 0x0019, 0x001A, 0x001B, 0x001C, 0x001D, 0x001E, 0x001F,
	// 0x20
	0x0020, 0x0021, 0x0022, 0x0023, 0x0024, 0x0025, 0x0026, 0x0027,
	0x0028, 0x0029, 0x002A, 0x002B, 0x002C, 0x002D, 0x002E, 0x002F,
	0x0030, 0x0031, 0x0032, 0x0033, 0x0034, 0x0035, 0x0036, 0x0037,
	0x0038, 0x0039, 0x003A, 0x003B, 0x003C, 0x003D, 0x003E, 0x003F,
	// 0x40
	0x0040, 0x0041, 0x0042, 0x0043, 0x0044, 0x0045, 0x0046, 0x0047,
	0x0048, 0x0049, 0x004A, 0x004B, 0x004C, 0x004D, 0x004E, 0x004F,
	0x0050, 0x0051, 0x0052, 0x0053, 0x0054, 0x0055, 0x0056, 0x0057,
	0x0058, 0x0059, 0x005A, 0x005B, 0x005C, 0x005D, 0x005E, 0x005F,
	// 0x60
	0x0060, 0x0061, 0x0062, 0x0063, 0x0064, 0x0065, 0x0066, 0x0067,
	0x0068, 0x0069, 0x006A, 0x006B, 0x006C, 0x006D, 0x006E, 0x006F,
	0x0070, 0x0071, 0x0072, 0x0073, 0x0074, 0x0075, 0x0076, 0x0077,
	0x0078, 0x0079, 0x007A, 0x007B, 0x007C, 0x007D, 0x007E, 0x007F,
	// 0x80
	0x0402, 0x0403, 0x201A, 0x0453, 0x201E, 0x2026, 0x2020, 0x2021,
	0x20AC, 0x2030, 0x0409, 0x2039, 0x040A, 0x040C, 0x040B, 0x040F,
	0x0452, 0x2018, 0x2019, 0x201C, 0x201D, 0x2022, 0x2013, 0x2014,
	0xFFFF, 0x2122, 0x0459, 0x203A, 0x045A, 0x045C, 0x045B, 0x045F,
	// 0xA0
	0x00A0, 0x040E, 0x045E, 0x0408, 0x00A4, 0x0490, 0x00A6, 0x00A7,
	0x0401, 0x00A9, 0x0404, 0x00AB, 0x00AC, 0x00AD, 0x00AE, 0x0407,
	0x00B0, 0x00B1, 0x0406, 0x0456, 0x0491, 0x00B5, 0x00B6, 0x00B7,
	0x0451, 0x2116, 0x0454, 0x00BB, 0x0458, 0x0405, 0x0455, 0x0457,
	// 0xC0
	0x0410, 0x0411, 0x0412, 0x0413, 0x0414, 0x0415, 0x0416, 0x0417,
	0x0418, 0x0419, 0x041A, 0x041B, 0x041C, 0x041D, 0x041E, 0x041F,
	0x0420, 0x0421, 0x0422, 0x0423, 0x0424, 0x0425, 0x0426, 0x0427,
	0x0428, 0x0429, 0x042A, 0x042B, 0x042C, 0x042D, 0x042E, 0x042F,
	// 0xE0
	0x0430, 0x0431, 0x0432, 0x0433, 0x0434, 0x0435, 0x0436, 0x0437,
	0x0438, 0x0439, 0x043A, 0x043B, 0x043C, 0x043D, 0x043E, 0x043F,
	0x0440, 0x0441, 0x0442, 0x0443, 0x0444, 0x0445, 0x0446, 0x0447,
	0x0448, 0x0449, 0x044A, 0x044B, 0x044C, 0x044D, 0x044E, 0x044F,
};

char convertUnicodeToWin1251(const char *src, uint16_t srcLen) {
	if(srcLen < 4) { return '?'; }
	uint16_t s = 0;
	for(uint16_t k = 0; k < 4; k++) {
		uint8_t c = hexToDigit(src[k]);
		if(c == 255) { return '?'; }
		s = (s << 4) | c;
	}
	uint16_t size = sizeof(win1251ToUnicode)/sizeof(win1251ToUnicode[0]);
	for(uint16_t i = 0; i < size; i++) {
		if(win1251ToUnicode[i] == s) { return (char)i; }
	}
	return '?';
}

// http://json.org/
void convertWin1251ToJsonUnicode(const char *src, StringBuilder *dst) {
	uint8_t *s = (uint8_t*)src;
	for(; *s != '\0'; s++) {
		uint8_t b = *s;
		if(b < 32) {
			switch(b) {
			case '\b': dst->add('\\'); dst->add('b'); break;
			case '\f': dst->add('\\'); dst->add('f'); break;
			case '\n': dst->add('\\'); dst->add('n'); break;
			case '\r': dst->add('\\'); dst->add('r'); break;
			case '\t': dst->add('\\'); dst->add('t'); break;
			default: continue;
			}
		} else if(b < 128) {
			switch(b) {
			case '"':  dst->add('\\'); dst->add('"'); break;
			case '\\': dst->add('\\'); dst->add('\\'); break;
			case '/':  dst->add('\\'); dst->add('/'); break;
			case 0x7F: continue;
			default: dst->add(b);
			}
		} else {
			uint16_t u = win1251ToUnicode[b];
			if(u == 0xFFFF) {
				continue;
			}
			uint8_t u1 = (u >> 8);
			uint8_t u0 = u;
			dst->add('\\');
			dst->add('u');
			dst->addHex(u1);
			dst->addHex(u0);
		}
	}
}

void convertJsonUnicodeToWin1251(const char *src, uint16_t srcLen, StringBuilder *dst) {
	char escapeSymbol = '\\';
	bool escape = false;
	dst->clear();
	for(uint16_t i = 0; i < srcLen; i++) {
		char b = src[i];
		if(escape == false) {
			if(b == escapeSymbol) {
				escape = true;
			} else {
				dst->add(b);
			}
		} else {
			escape = false;
			switch(b) {
			case 'b': dst->add('\b'); break;
			case 'f': dst->add('\f'); break;
			case 'n': dst->add('\n'); break;
			case 'r': dst->add('\r'); break;
			case 't': dst->add('\t'); break;
			case '"': dst->add('"'); break;
			case '\\': dst->add('\\'); break;
			case '/': dst->add('/'); break;
			case 'u': {
				i += 1;
				uint16_t tailLen = srcLen - i;
				if(tailLen < 4) { return; }
				dst->add(convertUnicodeToWin1251(src + i, tailLen));
				i += 3;
				break;
			}
			default: {
				dst->add('\\');
				dst->add(b);
			}
			}
		}
	}
}

uint8_t base64Char[] = {
	'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J',
	'K', 'L', 'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T',
	'U', 'V', 'W', 'X', 'Y', 'Z', 'a', 'b', 'c', 'd',
	'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n',
	'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x',
	'y', 'z', '0', '1', '2', '3', '4', '5', '6', '7',
	'8', '9', '+', '/'
};

static inline bool isBase64(uint8_t c) {
	return (isalnum(c) || (c == '+') || (c == '/'));
}

uint8_t findBase64(uint8_t c) {
	uint8_t max = sizeof(base64Char);
	for(uint8_t i = 0; i < max; i++) {
		if(base64Char[i] == c) {
			return i;
		}
	}
	return max;
}

uint16_t convertBinToBase64(const uint8_t *bin, uint16_t binLen, uint8_t *dst, uint16_t dstSize) {
	uint8_t char_array_3[3];
	uint8_t char_array_4[4];
	int i = 0;
	uint16_t dstLen = 0;
	for(int r = 0; r < binLen; r++) {
		char_array_3[i++] = bin[r];
		if(i == 3) {
			char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
			char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
			char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
			char_array_4[3] = char_array_3[2] & 0x3f;

			if((dstSize - dstLen) < 4) { return 0; }
			for(i = 0; (i < 4) ; i++) {
				dst[dstLen] = base64Char[char_array_4[i]];
				dstLen++;
			}
			i = 0;
		}
	}

	int j = 0;
	if(i) {
		for(j = i; j < 3; j++) {
			char_array_3[j] = '\0';
		}

		char_array_4[0] = ( char_array_3[0] & 0xfc) >> 2;
		char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
		char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);

		if((dstSize - dstLen) < 4) { return 0; }
		for(j = 0; (j < i + 1); j++) {
			dst[dstLen] = base64Char[char_array_4[j]];
			dstLen++;
		}

		while((i++ < 3)) {
			dst[dstLen] = '=';
			dstLen++;
		}
	}

	return dstLen;
}

uint16_t convertBinToBase64(const uint8_t *bin, uint16_t binLen, StringBuilder *dst) {
	uint8_t char_array_3[3];
	uint8_t char_array_4[4];
	int i = 0;
	uint16_t dstLen = 0;
	for(int r = 0; r < binLen; r++) {
		char_array_3[i++] = bin[r];
		if(i == 3) {
			char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
			char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
			char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
			char_array_4[3] = char_array_3[2] & 0x3f;

//			if((dstSize - dstLen) < 4) { return 0; }
			for(i = 0; (i < 4) ; i++) {
				dst->add(base64Char[char_array_4[i]]);
			}
			i = 0;
		}
	}

	int j = 0;
	if(i) {
		for(j = i; j < 3; j++) {
			char_array_3[j] = '\0';
		}

		char_array_4[0] = ( char_array_3[0] & 0xfc) >> 2;
		char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
		char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);

//		if((dstSize - dstLen) < 4) { return 0; }
		for(j = 0; (j < i + 1); j++) {
			dst->add(base64Char[char_array_4[j]]);
		}

		while((i++ < 3)) {
			dst->add('=');
		}
	}

	return dstLen;
}

uint16_t convertBase64ToBin(const uint8_t *src, uint16_t srcLen, uint8_t *dst, uint16_t dstSize) {
	uint8_t char_array_3[3];
	uint8_t char_array_4[4];
	uint16_t dstLen = 0;

	int in_len = srcLen;
	int i = 0;
	int j = 0;
	int in_ = 0;
	while(in_len-- && (src[in_] != '=') && isBase64(src[in_])) {
		char_array_4[i++] = src[in_]; in_++;
		if(i ==4) {
			for(i = 0; i <4; i++) {
				char_array_4[i] = findBase64(char_array_4[i]);
			}

			char_array_3[0] = ( char_array_4[0] << 2       ) + ((char_array_4[1] & 0x30) >> 4);
			char_array_3[1] = ((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2);
			char_array_3[2] = ((char_array_4[2] & 0x3) << 6) +   char_array_4[3];

			for(i = 0; (i < 3); i++) {
				dst[dstLen] = char_array_3[i];
				dstLen++;
			}
			i = 0;
		}
	}

	if(i) {
		for(j = 0; j < i; j++) {
			char_array_4[j] = findBase64(char_array_4[j]);
		}

		char_array_3[0] = (char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4);
		char_array_3[1] = ((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2);

		for(j = 0; (j < i - 1); j++) {
			dst[dstLen] = char_array_3[j];
			dstLen++;
		}
	}

	return dstLen;
}

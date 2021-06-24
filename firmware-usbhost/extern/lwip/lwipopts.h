#ifndef _LWIP_LWIPOPTS_H
#define _LWIP_LWIPOPTS_H

#include "config.h"
#include "common/platform/include/platform.h"

#define NO_SYS 1 // standalone mode - без операционной системы

// Настройка логов
#ifdef LOGGING
#ifdef LOG_LWIP
#define LWIP_DBG_MIN_LEVEL	LWIP_DBG_LEVEL_ALL
//#define LWIP_DEBUG			1
//#define ETHARP_DEBUG		LWIP_DBG_ON
//#define MEMP_DEBUG  		LWIP_DBG_ON
//#define PBUF_DEBUG  		LWIP_DBG_ON
//#define NETIF_DEBUG 		LWIP_DBG_ON
#define ICMP_DEBUG  		LWIP_DBG_ON
//#define INET_DEBUG  		LWIP_DBG_ON
//#define IP_DEBUG    		LWIP_DBG_ON
#define TCP_DEBUG			LWIP_DBG_ON
#define TCP_OUTPUT_DEBUG	LWIP_DBG_ON
#define DNS_DEBUG			LWIP_DBG_ON
#endif
#endif

// Настройка блокировок
#if 1
#define SYS_LIGHTWEIGHT_PROT 0 // отключаем поддержку мьютексов (смотри lwip/include/lwip/sys.h:363)
#else //todo: реализовать поддержку защиты от прерываний
#define SYS_ARCH_DECL_PROTECT(lev)   sys_prot_t lev
#define SYS_ARCH_PROTECT(lev)   lev = sys_arch_protect()
#define SYS_ARCH_UNPROTECT(lev)   sys_arch_unprotect(lev)
#endif

// Настройка протоколов
#define LWIP_ETHERNET	1 // включаем ETHERNET
#define LWIP_ARP		1 // включаем ARP
#define LWIP_NETCONN    0 // запрет ещё чего-то
#define LWIP_SOCKET     0 // запрет применения сокетов (доступно только в ОС)
#define LWIP_IPV6		0 // отключаем IP6
#define LWIP_SNMP		0 // отключаем SNMP
#define LWIP_RAW		0
#define LWIP_ICMP		1 // включаем ICMP
#define LWIP_UDP		1 // включаем UDP
#define LWIP_TCP		1 // включаем TCP
#define LWIP_DHCP		0
#define LWIP_STATS		0
#define LWIP_DNS		1 // включаем DNS

// Настройка памяти
#if 0
#define MEM_SIZE		16*1024 // размер кучи для пакетов
#define PBUF_POOL_SIZE	64 // количество пакетов (pbuf) в пуле
#else
#define MEM_SIZE		8*1024 // размер кучи для пакетов
#define PBUF_POOL_SIZE	32 // количество пакетов (pbuf) в пуле (уменьшено с 64)
#endif

// Тонкая настройка
#define LWIP_DONT_PROVIDE_BYTEORDER_FUNCTIONS 1 // отключаем объявление библиотекой LWIP своих версий макросов htons и ntohs

#endif

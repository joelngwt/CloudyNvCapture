/*!
 * \brief
 * A simple logging class
 *
 * \file
 *
 * This logger can log either into a file, or the standard output.
 *
 * \copyright
 * CopyRight 1993-2016 NVIDIA Corporation.  All rights reserved.
 * NOTICE TO LICENSEE: This source code and/or documentation ("Licensed Deliverables")
 * are subject to the applicable NVIDIA license agreement
 * that governs the use of the Licensed Deliverables.
 */

#pragma once

#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <time.h>
#include <winsock.h>
#include <windows.h>

#pragma comment(lib, "ws2_32.lib")

namespace simplelogger{

enum LogLevel {
	TRACE,
	DEBUG,
	INFO,
	WARN,
	ERR
};

class Logger {
public:
	Logger(LogLevel level, bool bPrintTimeStamp) : level(level), bPrintTimeStamp(bPrintTimeStamp) {
		InitializeCriticalSection(&cs);
	}
	virtual ~Logger() {
		DeleteCriticalSection(&cs);
	}
	virtual std::ostream& GetStream() = 0;
	virtual void FlushStream() {}
	bool ShouldLogFor(LogLevel l) {
		return l >= level;
	}
	char* GetLead(LogLevel l, char *szFile, int nLine, char *szFunc) {
		if (l < TRACE || l > ERR) {
			return "[?????] ";
		}
		char *szLevels[] = {"TRACE", "DEBUG", "INFO", "WARN", "ERROR"};
		if (bPrintTimeStamp) {
			time_t t = time(NULL);
			struct tm tm;
			localtime_s(&tm, &t);
			sprintf_s(szLead, sizeof(szLead), "[%-5s][%02d:%02d:%02d] ", 
				szLevels[l], tm.tm_hour, tm.tm_min, tm.tm_sec);
		} else {
			sprintf_s(szLead, sizeof(szLead), "[%-5s] ", szLevels[l]);
		}
		return szLead;
	}
	void EnterCriticalSection() {
		::EnterCriticalSection(&cs);
	}
	void LeaveCriticalSection() {
		::LeaveCriticalSection(&cs);
	}
private:
	LogLevel level;
	char szLead[80];
	bool bPrintTimeStamp;
	CRITICAL_SECTION cs;
};

class LoggerFactory {
public:
	static Logger* CreateFileLogger(std::string strFilePath, 
			LogLevel level = DEBUG, bool bPrintTimeStamp = true) {
		return new FileLogger(strFilePath, level, bPrintTimeStamp);
	}
	static Logger* CreateConsoleLogger(LogLevel level = DEBUG, 
			bool bPrintTimeStamp = true) {
		return new ConsoleLogger(level, bPrintTimeStamp);
	}
	static Logger* CreateUdpLogger(char *szHost, unsigned uPort, LogLevel level = DEBUG, 
			bool bPrintTimeStamp = true) {
		return new UdpLogger(szHost, uPort, level, bPrintTimeStamp);
	}
private:
	LoggerFactory() {}

	class FileLogger : public Logger {
	public:
		FileLogger(std::string strFilePath, LogLevel level, bool bPrintTimeStamp) 
		: Logger(level, bPrintTimeStamp) {
			pFileOut = new std::ofstream();
			pFileOut->open(strFilePath.c_str());
		}
		~FileLogger() {
			pFileOut->close();
		}
		std::ostream& GetStream() {
			return *pFileOut;
		}
	private:
		std::ofstream *pFileOut;
	};

	class ConsoleLogger : public Logger {
	public:
		ConsoleLogger(LogLevel level, bool bPrintTimeStamp) 
		: Logger(level, bPrintTimeStamp) {}
		std::ostream& GetStream() {
			return std::cout;
		}
	};

	class UdpLogger : public Logger {
	private:
		class UdpOstream : public std::ostream {
		public:
			UdpOstream(char *szHost, unsigned short uPort) : std::ostream(&sb), socket(INVALID_SOCKET){
				WSADATA w;
				if (WSAStartup(0x0101, &w) != 0) {
					fprintf(stderr, "WSAStartup() failed.\n");
					return;
				}
				socket = ::socket(AF_INET, SOCK_DGRAM, 0);
				if (socket == INVALID_SOCKET) {
					fprintf(stderr, "socket() failed.\n");
					WSACleanup();
					return;
				}
				unsigned int b1, b2, b3, b4;
				sscanf_s(szHost, "%u.%u.%u.%u", &b1, &b2, &b3, &b4);
				struct sockaddr_in s = {AF_INET, htons(uPort), {b1, b2, b3, b4}};
				server = s;
			}
			~UdpOstream() {
				if (socket == INVALID_SOCKET) {
					return;
				}
				closesocket(socket);
				WSACleanup();
			}
			void Flush() {
				if (sendto(socket, sb.str().c_str(), (int)sb.str().length() + 1, 
						0, (struct sockaddr *)&server, (int)sizeof(sockaddr_in)) == -1) {
					fprintf(stderr, "sendto() failed.\n");
				}
				sb.str("");
			}

		private:
			std::stringbuf sb;
			SOCKET socket;
			struct sockaddr_in server;
		};
	public:
		UdpLogger(char *szHost, unsigned uPort, LogLevel level, bool bPrintTimeStamp) 
		: Logger(level, bPrintTimeStamp), udpOut(szHost, uPort) {}
		UdpOstream& GetStream() {
			return udpOut;
		}
		virtual void FlushStream() {
			udpOut.Flush();
		}
	private:
		UdpOstream udpOut;
	};
};

}

#define LOG(pLogger, event, level) \
	do {													\
		if (!pLogger || !pLogger->ShouldLogFor(level)) {	\
			break;											\
		}													\
		pLogger->EnterCriticalSection();					\
		pLogger->GetStream()								\
			<< pLogger->GetLead(level, __FILE__, __LINE__,	\
				__FUNCTION__)								\
			<< event << std::endl;							\
		pLogger->FlushStream();								\
		pLogger->LeaveCriticalSection();					\
	} while (0);

#define LOG_TRACE(pLogger, event)	LOG(pLogger, event, simplelogger::TRACE)
#define LOG_DEBUG(pLogger, event)	LOG(pLogger, event, simplelogger::DEBUG)
#define LOG_INFO(pLogger, event)	LOG(pLogger, event, simplelogger::INFO)
#define LOG_WARN(pLogger, event)	LOG(pLogger, event, simplelogger::WARN)
#define LOG_ERROR(pLogger, event)	LOG(pLogger, event, simplelogger::ERR)

/*
 * utils.hpp
 *
 *  Created on: Mar 18, 2015
 *      Author: E9848823
 */

#ifndef UTILS_H_
#define UTILS_H_

#include <stdio.h>
#include <string.h>
#include <iostream>
#include <stdlib.h>
#include <sys/types.h>

typedef enum {
	tfFILE_NAME,
	tfFULL
} ETimeFormat;

std::string GetTimeString(ETimeFormat time_format);
void LogMsg(std::string msg);
void Error(std::string msg);
void EnableDebugMode(int level);
void ErrorCode(std::string msg, int code = 0);
void PrintXmlLine_Char(FILE *file, std::string tag, char value);
void PrintXmlLine_String(FILE *file, std::string tag, char *value);

//class TOutputSettings {
//	private:
//		static char separator;
//		static char commas;
//	public:
//		void TOutputSettings(char separator = ',', char commas = '"'):separator(separator), commas(commas){};
//};

#endif /* UTILS_H_ */

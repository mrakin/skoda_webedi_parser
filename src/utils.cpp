/*
 * utils.c
 *
 *  Created on: Mar 18, 2015
 *      Author: Marek
 */

#include "utils.h"

#include <iostream>
#include <string> // for std::string
#include <cassert>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <dirent.h>
#include <sys/stat.h>
#include <fstream>
#include <dirent.h>
#include <curl/curl.h>
#include <curl/easy.h>
#include <sys/stat.h>
#include <sstream>
#include <ctime>

using namespace std;

static int debug_level = 1;

string GetTimeString(ETimeFormat time_format)
{
	char rslt[50] = "time";
	time_t rawtime = time(NULL);
	struct tm * timeinfo;

	time (&rawtime);
	timeinfo = localtime (&rawtime);

	switch (time_format)
	{
		case tfFILE_NAME:
			sprintf(rslt, "%.04d%.02d%.02d_%.02d%.02d", (timeinfo->tm_year + 1900), (timeinfo->tm_mon + 1), (timeinfo->tm_mday), (timeinfo->tm_hour), (timeinfo->tm_min));
			break;

		case tfFULL:
			sprintf(rslt, "%.02d.%.02d.%.04d %.02d:%.02d:%.02d", (timeinfo->tm_mday), (timeinfo->tm_mon + 1), (timeinfo->tm_year + 1900), (timeinfo->tm_hour), (timeinfo->tm_min), (timeinfo->tm_sec));
			break;

		default:
			break;
	}
	return rslt;
}

static void PrintMsg(string msg)
{
	cout <<  GetTimeString(tfFULL) << ": " << msg << endl;
}

void LogMsg(string msg)
{
	if (debug_level >= 2)
		PrintMsg(msg);
}

void Error(string msg)
{
	if (debug_level >= 1)
		PrintMsg("Error: " + msg);
}

void ErrorCode(string msg, int code)
{
	std::stringstream temp_stream;
	temp_stream << msg << ": " << code;

	string temp_string = temp_stream.str();
	Error(temp_string);
}


void CriticalError(string msg)
{
	PrintMsg(msg);

	exit(-1);
}

void EnableDebugMode(int level)
{
	debug_level = level;
}

void PrintXmlLine_Char(FILE *file, string tag, char value)
{
	fprintf(file, "<%s>%c</%s>\r\n", tag.c_str(), value, tag.c_str());
}

void PrintXmlLine_String(FILE *file, string tag, char *value)
{
	fprintf(file, "<%s>%s</%s>\r\n", tag.c_str(), value ? value : "", tag.c_str());
}

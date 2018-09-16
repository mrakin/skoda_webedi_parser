/*
 * parsexml.hpp
 *
 *  Created on: Jul 17, 2014
 *      Author: E9848823
 */

#ifndef PARSEXML_H_
#define PARSEXML_H_

#include <iostream>
#include <string> // for std::string
#include <cassert>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <dirent.h>
#include <fstream>
#include <dirent.h>

#include "pugiconfig.h"
#include "pugixml.h"

using namespace std;

int ReadFile( const string & src, FILE *output_file, FILE *output_file_all);
int ParseFormating(void);
int PrintHeader(FILE *output_file);
int PrintDefaultFormatingFile(void);
int PrintAllMonths(void);
int ParseCategories(void);
void SetSeparator(char separator);
char GetSeparator(void);
int PrintCategory(FILE *file, char *p_part_number);

#endif /* PARSEXML_H_ */

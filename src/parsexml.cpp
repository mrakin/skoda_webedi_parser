/*
 * parsexml.cpp
 *
 *  Created on: Jul 17, 2014
 *      Author: Marek
 */
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
#include <vector>
#include "pugiconfig.h"
#include "pugixml.h"
#include "utils.h"

using namespace std;

typedef struct {
	int pos_counter;
	string buffer;
	char separator;
	char commas;
	int zero_count_counter;
	int future_order_counter;
	bool all_months;
} sOutput;

static sOutput output = {
	0,
	"",
	',',
	'"',
	0,
	0,
	false
};

const char* node_types[] =
{
    "null", "document", "element", "pcdata", "cdata", "comment", "pi", "declaration"
};

typedef enum {
	psNONE, psPARSE_HEADER, psPARSE_POS, psPARSE_POSNR, psPARSE_TERM, psPARSE_AMOUNT
} EParserState;

typedef struct {
	string name;
	pugi::xml_node *node;
	string print_name;
} SParserItem;

static EParserState parser_state = psNONE;

static SParserItem header_items[] =
{
		{"CATEGORY", NULL, "none"},
		{"CATEGORY_2", NULL, "none"},
		{"SUBSTITUTE", NULL, "none"},
		{"PARTNUMBER", NULL, "none"},
		{"PARTNAME", NULL, "none"},
		{"ORDERNUMBER", NULL, "none"},
		{"DATEFROM", NULL, "none"},
		{"MESUNIT", NULL, "none"},
		{"CUSTOMER", NULL, "none"},
		{"FACTORY", NULL, "none"},
		{"CALLOFNUMBER", NULL, "none"},
		{"CALLOFDATE", NULL, "none"},
		{"DEPOSITPLACECODE", NULL, "none"},
		{"DEPOSITPLACE", NULL, "none"},
		{"DEPOSITPLACEADDRESS", NULL, "none"},
		{"ISSUEDCODE", NULL, "none"},
		{"ISSUEDNAME", NULL, "none"},
		{"ISSUEDCONTACT", NULL, "none"},
		{"LASTSHIPMENDATE", NULL, "none"},
		{"REALCUMUL", NULL, "none"},
		{"LASTDELIVSHEETNR1", NULL, "none"},
		{"LASTDELIVSHEETAMOUNT1", NULL, "none"},
		{"FREETEXT1", NULL, "none"},
		{"FREETEXT2", NULL, "none"},
		{"FREETEXT3", NULL, "none"},
		{"POSNR", NULL, "none"},
		{"TERM", NULL, "none"},
		{"AMOUNT", NULL, "none"},
		{"", NULL, "none"}
};

typedef struct {
	string part_name;
	string category;
} SCategory;

vector<SCategory> Categories;
vector<SCategory> Categories_2;

SParserItem *GetItemPtr(SParserItem *p_item, string node_name);
#define GetHeaderItemPtr(node_name) (GetItemPtr(header_items, node_name))


void PrintItemToOutput(FILE *file, char *data)
{
	char *replace;

	while ((replace = strchr(data, output.commas)))
		*replace = ' ';

	while ((replace = strchr(data, output.separator)))
		*replace = ' ';

	fprintf(file, "%c%s%c%c", output.commas, data, output.commas, output.separator);
//	output.buffer += output.commas + data + output.commas + output.separator;
}

void PrintHeaderCategory(FILE *file, vector<SCategory> &categories)
{
	SParserItem *p_partnumber = GetHeaderItemPtr("PARTNUMBER");
	if ((p_partnumber != NULL) && (p_partnumber->node != NULL))
	{
		unsigned int i;
		for (i = 0; i < categories.size(); i++)
		{
			if (!strcmp(categories[i].part_name.c_str(), (char *)p_partnumber->node->first_child().value()))
				break;
		}
		if (i != categories.size())
			PrintItemToOutput(file, (char *)categories[i].category.c_str());
		else
			PrintItemToOutput(file, (char *)"");
	}
}

int PrintHeaderItems(FILE *file)
{
	SParserItem *p_header_item = header_items;

	while (p_header_item->name != "")
	{
		if (!strcmp(p_header_item->name.c_str(), "POSNR"))
		{
			;
		}
		else if (!strcmp(p_header_item->name.c_str(), "TERM"))
		{
			;
		}
		else if (!strcmp(p_header_item->name.c_str(), "AMOUNT"))
		{
			;
		}
		else
		if (p_header_item->print_name != "none")
		{
			if (!strcmp(p_header_item->name.c_str(), "CATEGORY"))
			{
				PrintHeaderCategory(file, Categories);
			}
			else if (!strcmp(p_header_item->name.c_str(), "CATEGORY_2"))
			{
				PrintHeaderCategory(file, Categories_2);
			}
			else
			{
				if (p_header_item->node != NULL)
					PrintItemToOutput(file, (char *)p_header_item->node->first_child().value());
				else
					PrintItemToOutput(file, (char *)"");
			}
		}
		p_header_item++;
	}

	return 0;
}

void InitHeaderItems(void)
{
	SParserItem *p_header_item = header_items;
	parser_state = psNONE;

	while (p_header_item->name != "")
	{
		delete (p_header_item->node);
		p_header_item->node = NULL;

		p_header_item++;
	}
}

SParserItem *GetItemPtr(SParserItem *p_item, string node_name)
{
	while (p_item->name != "")
	{
		if (p_item->name == node_name)
			return p_item;

		p_item++;
	}

	return NULL;
}

typedef struct PosItem {
    string *p_data;
    struct PosItem *next;
} SPosItem;

//static struct to hold info about first pointer
static SPosItem pos_item = {NULL, NULL};

static int PosItem_Add(string data)
{
	SPosItem *p_pos_item = &pos_item;

	while (p_pos_item->next)
		p_pos_item = p_pos_item->next;

	p_pos_item->next = (SPosItem *)malloc(sizeof(SPosItem));
	if (!p_pos_item->next)
		return -1;

	p_pos_item->next->p_data = new string;

	p_pos_item->next->next = NULL;
	*p_pos_item->next->p_data = data;

	return 0;
}


static int PosItem_Clear(void)
{
	SPosItem *p_pos_item = pos_item.next;

	output.pos_counter = 0;

	while (p_pos_item)
	{
		SPosItem *p_next_pos_item = p_pos_item->next;

		delete p_pos_item->p_data;
		free(p_pos_item);
		p_pos_item = p_next_pos_item;
	}

	pos_item.next = NULL;

	return 0;
}

int SkipFutureRecord(string *date)
{
	if (output.all_months)
		return 0;

	char *date_str = (char *)date->c_str();

	time_t rawtime, today;
	struct tm * timeinfo;

	time ( &rawtime );
	today = rawtime;
	timeinfo = localtime ( &rawtime );
	timeinfo->tm_year = 0;

//	31.12.2015
	if (strchr(date_str, '.'))
	{
		if (isdigit(*date_str))
			timeinfo->tm_mday = *date_str - '0';
		else
			return -1;

		date_str++;

		if (*date_str != '.')
		{
			if (isdigit(*date_str))
			{
				timeinfo->tm_mday *= 10;
				timeinfo->tm_mday += *date_str - '0';
			}
			else
				return -1;

			date_str++;
		}

		if (*date_str != '.')
			return -1;

		date_str++;

		if (isdigit(*date_str))
			timeinfo->tm_mon = *date_str - '0';
		else
			return -1;

		date_str++;

		if (*date_str != '.')
		{
			if (isdigit(*date_str))
			{
				timeinfo->tm_mon *= 10;
				timeinfo->tm_mon += *date_str - '0';
			}
			else
				return -1;

			date_str++;
		}

		if (*date_str != '.')
			return -1;

		for (int i = 0; i < 4; i++)
		{
			date_str++;
			timeinfo->tm_year *= 10;

			if (isdigit(*date_str))
				timeinfo->tm_year += *date_str - '0';
			else
				return -1;

		}

		timeinfo->tm_mon -= 1;
		timeinfo->tm_year -= 1900;

		rawtime = mktime (timeinfo);
	}
//	23 2015-26 2015
	else if (strchr(date_str, '-'))
	{
		int week = 0;

		if (isdigit(*date_str))
			week = *date_str - '0';
		else
			return -1;

		date_str++;

		if (*date_str != ' ')
		{
			if (isdigit(*date_str))
			{
				week *= 10;
				week += *date_str - '0';
			}
			else
				return -1;

			date_str++;
		}

		if (*date_str != ' ')
			return -1;

		date_str++;

		for (int i = 0; i < 4; i++)
		{
			timeinfo->tm_year *= 10;

			if (isdigit(*date_str))
				timeinfo->tm_year += *date_str - '0';
			else
				return -1;

			date_str++;
		}

		if (*date_str != '-')
			return -1;

		date_str++;

		/****************   Second Part ***************/
		if (!isdigit(*date_str))
			return -1;

		date_str++;

		if (*date_str != ' ')
		{
			if (!isdigit(*date_str))
				return -1;

			date_str++;
		}

		if (*date_str != ' ')
			return -1;

		for (int i = 0; i < 4; i++)
		{
			date_str++;

			if (!isdigit(*date_str))
				return -1;
		}

		timeinfo->tm_year -= 1900;
		timeinfo->tm_mon = 0;
		timeinfo->tm_mday = 1;

		rawtime = mktime (timeinfo);

		rawtime -= 60 * 60 * 24 * timeinfo->tm_wday;
		rawtime += week * 7 *  60 * 60 * 24;
	}
	else
		return -1;

	if ((rawtime - today) / 60 / 60 / 24 > 63)
		return 1;

	return 0;
}

int PrintEachPos(FILE *file, bool print_all)
{
	SPosItem *p_pos_item = pos_item.next;

	SPosItem *p_posnr;
	SPosItem *p_term;
	SPosItem *p_amount;

	while (p_pos_item)
	{
		for (int i = 0; i < 3; i++)
		{
			switch (i)
			{
				case 0:
					p_posnr = p_pos_item;
					break;
				case 1:
					p_term = p_pos_item;
					break;
				case 2:
					p_amount = p_pos_item;
					break;
				default:
					break;
			}
			p_pos_item = p_pos_item->next;
		}

		if (!print_all
		 && ( !strcmp((char *)p_amount->p_data->c_str(), "0.0")
 	       || !strcmp((char *)p_amount->p_data->c_str(), "0") ) )
		{
			LogMsg("Pocet 0.0 -> zaznam preskocen");
			output.zero_count_counter++;
		}
		else if (!print_all
			  && SkipFutureRecord(p_term->p_data) > 0)
		{
			LogMsg("Termin > 2 mesice od ted -> zaznam preskocen");
			output.future_order_counter++;
		}
		else
		{
			PrintHeaderItems(file);

			SParserItem *p_header_item = GetHeaderItemPtr("POSNR");
			if ((p_header_item != NULL) && strcmp(p_header_item->print_name.c_str(), "none"))
				PrintItemToOutput(file, (char *)p_posnr->p_data->c_str());

			p_header_item = GetHeaderItemPtr("TERM");
			if ((p_header_item != NULL) && strcmp(p_header_item->print_name.c_str(), "none"))
				PrintItemToOutput(file, (char *)p_term->p_data->c_str());

			p_header_item = GetHeaderItemPtr("AMOUNT");
			if ((p_header_item != NULL) && strcmp(p_header_item->print_name.c_str(), "none"))
			{
				char buf[20];
				char *p_dot;
				strcpy(buf, (char *)p_amount->p_data->c_str());

				if ((p_dot = strchr(buf, '.')))
					*p_dot = '\0';

				PrintItemToOutput(file, buf);
			}

			fprintf(file, "%s\r\n", output.buffer.c_str());
		}
	}

	return 0;
}

/******************************************************************************/
int PrintAll(FILE *file)
{
	if (pos_item.next != NULL)
		return PrintEachPos(file, true);
	else
	{
		PrintHeaderItems(file);

		fprintf(file, "\r\n");

		return 0;
	}
}

/******************************************************************************/
int PrintValidOnly(FILE *file)
{
	return PrintEachPos(file, false);
}

/******************************************************************************/
int PrintCategory(FILE *file, char *p_part_number)
{
	if (file && p_part_number)
	{
		fprintf(file, "\t<Part>\r\n");
		{
			fprintf(file, "\t\t");
			PrintXmlLine_String(file, "Name", p_part_number);
			fprintf(file, "\t\t");
			PrintXmlLine_String(file, "Group", (char *)"");
			fprintf(file, "\t\t");
			PrintXmlLine_String(file, "Group_2", (char *)"");
		}
		fprintf(file, "\t</Part>\r\n");
	}
	return 0;
}

/******************************************************************************/
//[code_traverse_walker_impl
struct simple_walker: pugi::xml_tree_walker
{
    virtual bool for_each(pugi::xml_node& node)
    {
#if 1
    	if (node.type() != 2 /* element */)
    		return true;

    	switch (parser_state)
    	{
    		case psNONE:
    			if (!strcmp(node.name(), "HEADER"))
    			{
    				parser_state = psPARSE_HEADER;
    			}
    			break;

    		case psPARSE_HEADER:
    		{
    	    	SParserItem *p_header_item = GetHeaderItemPtr(node.name());

    	    	if (p_header_item != NULL)
    	    	{
    	    		p_header_item->node = new pugi::xml_node;

    	    		*p_header_item->node = node;
    	    	}
    	    	else
    	    	if (!strcmp(node.name(), "POS"))
    	    	{
    	    		parser_state = psPARSE_POSNR;	//psPARSE_POS + 1 for the next loop
    	    	}
    	    	else
    	    	{
    	    		Error("Not parsed!!!");
    	    		Error(node.name());
    	    	}
    	    	break;
    		}

    		case psPARSE_POS:
    		{
    			if (strcmp(node.name(), "POS"))
    				Error("Parsing POS !!!");
    			parser_state = psPARSE_POSNR;
    			break;
    		}

    		case psPARSE_POSNR:
    		{
    			if (strcmp(node.name(), "POSNR"))
    				Error("Parsing POSNR !!!");
       			parser_state = psPARSE_TERM;

    			PosItem_Add(node.first_child().value());
    			break;
    		}

    		case psPARSE_TERM:
    		{
    			if (strcmp(node.name(), "TERM"))
    				Error("Parsing TERM !!!");
       			parser_state = psPARSE_AMOUNT;
       			PosItem_Add(node.first_child().value());
    			break;
    		}

    		case psPARSE_AMOUNT:
    		{
    			if (strcmp(node.name(), "AMOUNT"))
    				Error("Parsing AMOUNT !!!");
       			parser_state = psPARSE_POS;
       			PosItem_Add(node.first_child().value());
    			break;
    		}

    		default:
    			break;
    	}


#else
    	for (int i = 0; i < depth(); ++i)
        	fprintf(parse_output_file, "  "); // indentation

        fprintf(parse_output_file, "%s: name='%s', value='%s'\n", node_types[node.type()], node.name(), node.value());
#endif

        return true; // continue traversal
    }
};
//]


/******************************************************************************/
int ReadFile( const string & src, FILE *output_file, FILE *output_file_all )
{
	pugi::xml_document doc;

    pugi::xml_parse_result result = doc.load_buffer( src.c_str( ), src.length( ) );

    output.buffer = "";

    if (result.status == pugi::status_ok)
    {
    	InitHeaderItems();

    	PosItem_Clear();

    	//parse XML
    	simple_walker walker;
		doc.traverse(walker);

		PrintAll(output_file_all);
		PrintValidOnly(output_file);

//	    output.buffer += "\r\n";
//	    fprintf(output_file, output.buffer.c_str());

		return 0;
    }
    return -1;
}

/******************************************************************************/
//[code_traverse_walker_impl
struct simple_walker_for_formating: pugi::xml_tree_walker
{
    virtual bool for_each(pugi::xml_node& node)
    {
    	if ( node.type() != 2 ) return true;

		SParserItem *p_header_item = GetHeaderItemPtr(node.name());

		if (p_header_item != NULL)
		{
			p_header_item->print_name = node.first_child().value();
		}

		return true; // continue traversal
  	}
};

/******************************************************************************/
int ParseFormating(void)
{
	pugi::xml_document doc;
	pugi::xml_parse_result result = doc.load_file("formatting.xml");
	pugi::xml_node node;

	if (result.status == pugi::status_ok)
    {
    	//parse XML
		simple_walker_for_formating walker;
		doc.traverse(walker);

		return 0;
    }

	return -1;
}

/******************************************************************************/
int PrintHeader(FILE *output_file)
{
	SParserItem *p_header_item = header_items;

	if (output_file == NULL)
		return -1;

	while (p_header_item->name != "")
	{
		if (strcmp(p_header_item->print_name.c_str(), "none"))
		{
			PrintItemToOutput(output_file, (char *)p_header_item->print_name.c_str());
		}

		p_header_item++;
	}

	fprintf(output_file, "\r\n");

	return 0;
}

/******************************************************************************/
int PrintDefaultFormatingFile(void)
{
	SParserItem *p_header_item = header_items;

	FILE *output_file = fopen("./formatting.xml", "w");

	if (output_file)
	{
		fprintf(output_file, "<Formatting>\r\n");
		while (p_header_item->name != "")
		{
			PrintXmlLine_String(output_file, p_header_item->name, (char *)p_header_item->name.c_str());

			p_header_item++;
		}
		fprintf(output_file, "</Formatting>\r\n");

		fclose(output_file);
	}
	else
	{
		Error("Creating Formatting File");

		return -1;
	}

	return 0;
}

/******************************************************************************/
int PrintAllMonths(void)
{
	output.all_months = true;

	return 0;
}

/******************************************************************************/
struct find_Name { bool operator()(pugi::xml_node node) const { return (!strcmp(node.name(), "Name"));} };
struct find_Group { bool operator()(pugi::xml_node node) const { return (!strcmp(node.name(), "Group"));} };
struct find_Group_2 { bool operator()(pugi::xml_node node) const { return (!strcmp(node.name(), "Group_2"));} };

/******************************************************************************/
//[code_traverse_walker_impl
struct simple_walker_for_categories: pugi::xml_tree_walker
{
    virtual bool for_each(pugi::xml_node& node)
    {
#if 1
    	if (depth() == 1)
    	{
    		if (!strcmp(node.name(), "Part"))
    		{
    			pugi::xml_node node_name;
    			pugi::xml_node node_group;
    			pugi::xml_node node_group_2;

    			if (!(node_name = node.find_node(find_Name())))
					LogMsg("Name node not found");

				if (!(node_group = node.find_node(find_Group())))
					LogMsg("Group node not found");

				if (!(node_group_2 = node.find_node(find_Group_2())))
					; //LogMsg("Group 2 node not found");

    			if (node_name)
    			{
    				if (node_group)
    				{
    					SCategory category = {node_name.first_child().value(),
    							 	 	  node_group.first_child().value()};

    					Categories.push_back(category);
    				}
    				if (node_group_2)
    				{
    					SCategory category = {node_name.first_child().value(),
    							 	 	  node_group_2.first_child().value()};

    					Categories_2.push_back(category);
    				}
    			}
    		}
    	}
#else
    	char buf[200];
    	char *bufp = buf;

       	for (int i = 0; i < depth(); ++i)
       		bufp += sprintf(bufp, "  "); // indentation

        sprintf(bufp, "%s: name='%s', value='%s'\n", node_types[node.type()], node.name(), node.value());
        LogMsg(buf);
#endif
		return true; // continue traversal
  	}
};

/******************************************************************************/
int ParseCategories(void)
{
	pugi::xml_document doc;
	pugi::xml_parse_result result = doc.load_file("categories.xml");
	pugi::xml_node node;

	if (result.status == pugi::status_ok)
    {
    	//parse XML
		simple_walker_for_categories walker;
		doc.traverse(walker);

		return 0;
    }

	return -1;
}

/******************************************************************************/
void SetSeparator(char separator)
{
	output.separator = separator;
}

/******************************************************************************/
char GetSeparator(void)
{
	return output.separator;
}

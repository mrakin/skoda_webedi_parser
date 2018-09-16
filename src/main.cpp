#include <iostream>
#include <vector>
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
#include <unistd.h>
#include <sstream>
#include <list>
#include "curl.h"
#include "files.h"
#include "parsexml.h"
#include "pugiconfig.h"
#include "pugixml.h"
#include "utils.h"

using namespace std;

#define VERSION "2.0"

static int reference_counter = 0;
static int part_number_counter = 0;

#define PROXY_STR "proxy"
#define GROUP_STR "group"
#define USER_STR "user"
#define PASSWORD_STR "password"

typedef struct {
	string proxy;
	string group;
	string user;
	string password;
} sConfiguration;

static sConfiguration configuration = {
	"","","",""
};

struct find_PROXY_STR { bool operator()(pugi::xml_node node) const { return (!strcmp(node.name(), "proxy"));} };
struct find_GROUP_STR { bool operator()(pugi::xml_node node) const { return (!strcmp(node.name(), GROUP_STR));} };
struct find_USER_STR { bool operator()(pugi::xml_node node) const { return (!strcmp(node.name(), USER_STR));} };
struct find_PASSWORD_STR { bool operator()(pugi::xml_node node) const { return (!strcmp(node.name(), PASSWORD_STR));} };
struct find_SEPARATOR_STR { bool operator()(pugi::xml_node node) const { return (!strcmp(node.name(), "separator"));} };

/*
 * Parse configuration file.
 */
int ParseInitFile(string file_name)
{
	int rslt = -1;

	pugi::xml_document doc;
	pugi::xml_parse_result result = doc.load_file(file_name.c_str());
	pugi::xml_node node;

	if (result.status == pugi::status_ok)
    {
    	rslt = 0;

    	if ((node = doc.find_node(find_PROXY_STR())))
    	{
    		configuration.proxy = node.first_child().value();
    	}
    	else
    	{
    		LogMsg("Proxy not found");
    		rslt = -4;
    	}
    	if ((node = doc.find_node(find_SEPARATOR_STR())))
    	{
    		string separator = node.first_child().value();
    		SetSeparator(*separator.c_str());
    	}
    	else
    	{
    		LogMsg("Separator not found");
    		rslt = -8;
    	}
    	if ((node = doc.find_node(find_GROUP_STR())))
    	{
    		configuration.group = node.first_child().value();
    	}
    	else
    	{
    		LogMsg("Group not found");
    		rslt = -5;
    	}
    	if ((node = doc.find_node(find_USER_STR())))
    	{
    		configuration.user = node.first_child().value();
    	}
    	else
    	{
    		LogMsg("User not found");
    		rslt = -6;
    	}
    	if ((node = doc.find_node(find_PASSWORD_STR())))
    	{
    		configuration.password = node.first_child().value();
    	}
    	else
    	{
    		LogMsg("Password not found");
    		rslt = -7;
    	}
    }
    else
    	Error("Config file not found");

    return rslt;
}

/*******************************************************************************
 *
 */
int InitFormatting(void)
{
	FILE *file = fopen("./formatting.xml", "rb");

	if (file == NULL)
	{
		if (PrintDefaultFormatingFile() == 0)
			return -2;
	}
	else
		fclose(file);

	if (ParseFormating() != 0)
	{
		Error("Parsing Formatting File");

		return -1;
	}
	return 0;
}

/*******************************************************************************
 * Open init file and read configuration or create new file and store defaults.
 */
int InitConfiguration(void)
{
	LogMsg("Init Configuration");

	if (ParseInitFile("./config.xml") != 0)
	{
		FILE *config_file = fopen("./config.xml", "w");

		if (config_file)
		{
			fprintf(config_file, "<Root>\r\n");
			PrintXmlLine_Char(config_file, "separator", GetSeparator());
			PrintXmlLine_String(config_file, PROXY_STR, (char *)configuration.proxy.c_str());
			PrintXmlLine_String(config_file, GROUP_STR, (char *)""/*(char *)configuration.group.c_str()*/);
			PrintXmlLine_String(config_file, USER_STR, (char *)""/*(char *)configuration.user.c_str()*/);
			PrintXmlLine_String(config_file, PASSWORD_STR, (char *)""/* (char *)configuration.password.c_str()*/);
			fprintf(config_file, "</Root>\r\n");
			fclose(config_file);
		}
		else
			Error("Configuration file not created");
	}

	InitFormatting();

	ParseCategories();

	return 0;
}

/*******************************************************************************
 *
 */
string GetSecreteCode( const string & src )
{
	const string val = "value=\"";
	size_t pos = src.find( "j_id1:javax.faces.ViewState:1" );
	if ( pos == string::npos )
	{
		cout << "secret code not found!" << endl;
		return "";
	}
	string temp = src.substr ( pos );
	pos = temp.find( val );
	pos += ( val.length() );
	temp = temp.substr ( pos );
	pos = temp.find( "\"" );
	temp = temp.substr( 0, pos );

	return temp;
}

/*******************************************************************************
 *
 */
vector<string> GetDataRkList( const string & src )
{
	size_t pos;
	const string val = "data-rk=\"";
	vector<string> data_rk_list;
	string rk, temp = src;

	while ( ( pos = temp.find( val ) ) != string::npos )
	{
		pos += ( val.length() );
		temp = temp.substr ( pos );
		pos = temp.find( "\"" );
		rk = temp.substr( 0, pos );

		data_rk_list.push_back( rk );
	}

	return data_rk_list;
}


/*******************************************************************************
 *
 */
bool GetLoginPage( CurlWrapper & curl_wrapper )
{
	return curl_wrapper.HTTP_Get( "https://web3.teledin.cz/WebEdi2/faces/login.xhtml"
							  , "https://web3.teledin.cz/WebEdi2/faces/login.xhtml"
							  , { } );
}

/*******************************************************************************
 *
 */
bool PostLoginPage( CurlWrapper & curl_wrapper, const string & session_id, const string & code )
{
	string url = "https://web3.teledin.cz/WebEdi2/faces/login.xhtml;jsessionid="
	           + session_id;

	list<string> headers = {
		  "Faces-Request: partial/ajax"
		, "Accept: application/xml, text/xml, */*; q=0.01"
		, "Accept-Language: en-US,en;q=0.5"
		, "Accept-Encoding: gzip, deflate, br"
		, "Content-Type: application/x-www-form-urlencoded; charset=UTF-8"
		, "X-Requested-With: XMLHttpRequest"
		, "Connection: keep-alive"
	};

	string post_data = "javax.faces.partial.ajax=true&javax.faces.source=loginForm%3Aj_idt40%3Aj_idt52&"
					   "javax.faces.partial.execute=loginForm%3Aj_idt40%3Aj_idt52+loginForm%3Aj_idt40%3A"
					   "groupname+loginForm%3Aj_idt40%3Ausername+loginForm%3Aj_idt40%3Apassword&"
					   "javax.faces.partial.render=loginForm%3Aj_idt40%3AloginPanel+growl&loginForm%3A"
					   "j_idt40%3Aj_idt52=loginForm%3Aj_idt40%3Aj_idt52&loginForm=loginForm&loginForm%3A"
					   "j_idt40%3Agroupname="
					 + configuration.group
					 + "&loginForm%3Aj_idt40%3Ausername="
					 + configuration.user
					 + "&loginForm%3Aj_idt40%3A"
					   "password="
					 + configuration.password
					 + "&loginForm%3Aj_idt40%3AownerNumber=&loginForm%3Aj_idt40%3A"
					   "ownerIndex=&loginForm%3Aj_idt40%3Aduns=&loginForm%3Aj_idt40%3Aname=&loginForm%3A"
					   "j_idt40%3Aemail=&loginForm%3Aj_idt40%3AresponsibleRepresentative=&loginForm%3Aj_idt"
					   "40%3Astreet=&loginForm%3Aj_idt40%3Acity=&loginForm%3Aj_idt40%3Apostcode=&loginForm%3A"
					   "j_idt40%3Acountry=&loginForm%3Aj_idt40%3Aphone=&loginForm%3Aj_idt40%3Amobile=&loginForm%3A"
					   "j_idt40%3Afax=&loginForm%3Aj_idt40%3Anote=&loginForm%3Aj_idt40_activeIndex=0&javax.faces.ViewState="
					 + code;

	return ( curl_wrapper.HTTP_Post( url
							   , "https://web3.teledin.cz/WebEdi2/faces/login.xhtml"
							   , headers
							   , post_data
							   , true ) );
}

/*******************************************************************************
 *
 */
bool GetIndexPage( CurlWrapper & curl_wrapper )
{
	list<string> headers = {
		"Upgrade-Insecure-Requests: 1"
	  , "Accept: text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8"
	};

	return ( curl_wrapper.HTTP_Get( "https://web3.teledin.cz/WebEdi2/faces/secu/index.xhtml"
								  , "https://web3.teledin.cz/WebEdi2/faces/login.xhtml"
								  , headers ) );
};

/*******************************************************************************
 *
 */
bool PostIndexPage( CurlWrapper & curl_wrapper, const string & code )
{
	list<string> headers = {
			"Upgrade-Insecure-Requests: 1"
		  , "Accept: text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8"
	};

	string post_data = "menuForm=menuForm&javax.faces.ViewState="
					+ code
					+ "&menuForm%3Aj_idt29=menuForm%3Aj_idt29";

	return ( curl_wrapper.HTTP_Post( "https://web3.teledin.cz/WebEdi2/faces/secu/index.xhtml"
							   , "https://web3.teledin.cz/WebEdi2/faces/secu/index.xhtml"
							   , headers
							   , post_data ) );
}

/*******************************************************************************
 *
 */
string ReplaceSpaces( const string & src, const string & replace )
{
	string rslt = "";

	for ( auto & c : src )
	{
		if ( c == ' '  ) rslt += replace;
		else rslt  += c;
	}

	return rslt;
}

/*******************************************************************************
 *
 */
bool ChoosePart_FirstRequest( CurlWrapper & curl_wrapper, const string & code, const string & part_name )
{
	list<string> headers = {
		   "Accept: */*"
	};

	string part_number = ReplaceSpaces( part_name, "%20" ); // "%20BDB%20300%20001%20A";

	string post_data = "DlfrListForm=DlfrListForm&DlfrListForm%3Aindex_focus=&DlfrListForm%3Aindex_input=&Dl"
					   "frListForm%3Apart_focus=&DlfrListForm%3Apart_input="
					 + part_number
					 + "&DlfrListForm%3A"
					   "term_focus=&DlfrListForm%3Aterm_input=&DlfrListForm%3AreferenceDate_focus=&DlfrListForm%3A"
					   "referenceDate_input=&javax.faces.ViewState="
					 + code
					 + "&javax.faces.source=DlfrListForm%3Apart&javax.faces.partial.event=change&"
					   "javax.faces.partial.execute=DlfrListForm%3Apart%20DlfrListForm%3Apart&"
					   "javax.faces.partial.render=DlfrListForm&javax.faces.behavior.event=change&javax.faces.partial.ajax=true";

	return ( curl_wrapper.HTTP_Post( "https://web3.teledin.cz/WebEdi2/faces/secu/dlfr/List.xhtml"
							   	   , "https://web3.teledin.cz/WebEdi2/faces/secu/index.xhtml"
								   , headers
								   , post_data) );
}


/*******************************************************************************
 *
 */
bool ChoosePart_SecondRequest( CurlWrapper & curl_wrapper, const string & code, const string & part_name )
{
	list<string> headers = {
		   "application/xml, text/xml, */*; q=0.01"
	};

	string part_number = ReplaceSpaces( part_name, "+" ); // "+BDB+300+001+A";

	string post_data = "javax.faces.partial.ajax=true&javax.faces.source=DlfrListForm%3Adatalist&"
						"javax.faces.partial.execute=DlfrListForm%3Adatalist&javax.faces.partial.render=DlfrListForm%3A"
						"datalist&DlfrListForm%3Adatalist=DlfrListForm%3Adatalist&DlfrListForm%3Adatalist_filtering=true&Dl"
						"frListForm%3Adatalist_encodeFeature=true&DlfrListForm=DlfrListForm&DlfrListForm%3Aindex_focus=&Dl"
						"frListForm%3Aindex_input=&DlfrListForm%3Apart_focus=&DlfrListForm%3Apart_input="
					 + part_number
					 + 	"&DlfrListForm%3Aterm_focus=&DlfrListForm%3Aterm_input=&DlfrListForm%3AreferenceDate_focus=&DlfrListForm%3A"
						"referenceDate_input=&DlfrListForm%3Adatalist_rppDD=20&DlfrListForm%3Adatalist_rppDD=20&DlfrListForm%3A"
						"datalist_selection=&javax.faces.ViewState="
 					 + code;

	return ( curl_wrapper.HTTP_Post( "https://web3.teledin.cz/WebEdi2/faces/secu/dlfr/List.xhtml"
							   	   , "Referer: https://web3.teledin.cz/WebEdi2/faces/secu/index.xhtml"
								   , headers
								   , post_data ) );
}

/*******************************************************************************
 *
 */
bool GetReference_FirstRequest( CurlWrapper & curl_wrapper, const string & code, const string & part_name, const string & data_rk )
{
	list<string> headers = {
		   "application/xml, text/xml, */*; q=0.01"
	};

	string part_number = ReplaceSpaces( part_name, "+" ); // "+BDB+300+001+A";

	string post_data = "javax.faces.partial.ajax=true&javax.faces.source=DlfrListForm%3Adatalist&javax.faces.partial.execute=DlfrListForm%3A"
					   "datalist&javax.faces.partial.render=DlfrListForm%3Adatalist%3AviewButton&javax.faces.behavior.event=rowSelect&"
					   "javax.faces.partial.event=rowSelect&DlfrListForm%3Adatalist_instantSelectedRowKey="
					 + data_rk
					 + "&DlfrListForm=DlfrListForm&"
					   "DlfrListForm%3Aindex_focus=&DlfrListForm%3Aindex_input=&DlfrListForm%3Apart_focus=&DlfrListForm%3Apart_input="
					 + part_number
					 + "&DlfrListForm%3Aterm_focus=&DlfrListForm%3Aterm_input=&DlfrListForm%3AreferenceDate_focus=&DlfrListForm%3A"
					   "referenceDate_input=&DlfrListForm%3Adatalist_rppDD=20&DlfrListForm%3Adatalist_rppDD=20&DlfrListForm%3Adatalist_selection="
					 + data_rk
					 + "&javax.faces.ViewState="
 				     + code;

	return ( curl_wrapper.HTTP_Post( "https://web3.teledin.cz/WebEdi2/faces/secu/dlfr/List.xhtml"
							   	   , "Referer: https://web3.teledin.cz/WebEdi2/faces/secu/index.xhtml"
								   , headers
								   , post_data
								   , true ) );
}

/*******************************************************************************
 *
 */
bool GetReference_SecondRequest( CurlWrapper & curl_wrapper, const string & code, const string & part_name, const string & data_rk )
{
	list<string> headers = {
		   "application/xml, text/xml, */*; q=0.01"
	};

	string part_number = ReplaceSpaces( part_name, "+" ); // "+BDB+300+001+A";

	string post_data = "javax.faces.partial.ajax=true&javax.faces.source=DlfrListForm%3Adatalist%3AviewButton&"
					   "javax.faces.partial.execute=%40all&DlfrListForm%3Adatalist%3AviewButton=DlfrListForm%3A"
					   "datalist%3AviewButton&DlfrListForm=DlfrListForm&DlfrListForm%3Aindex_focus=&DlfrListForm%3A"
					   "index_input=&DlfrListForm%3Apart_focus=&DlfrListForm%3Apart_input="
					 + part_number
					 + "&DlfrListForm%3Aterm_focus=&DlfrListForm%3Aterm_input=&DlfrListForm%3AreferenceDate_focus=&Dl"
					   "frListForm%3AreferenceDate_input=&DlfrListForm%3Adatalist_rppDD=20&DlfrListForm%3Adatalist_rppDD=20"
					   "&DlfrListForm%3Adatalist_selection="
					 + data_rk
					 + "&javax.faces.ViewState="
 					 + code;

	return ( curl_wrapper.HTTP_Post( "https://web3.teledin.cz/WebEdi2/faces/secu/dlfr/List.xhtml"
							   	   , "https://web3.teledin.cz/WebEdi2/faces/secu/index.xhtml"
								   , headers
								   , post_data
								   , true ) );
}

/*******************************************************************************
 *
 */
bool ExportReference( CurlWrapper & curl_wrapper, const string & code )
{
	list<string> headers = {
		   "text/html,application/xhtml+xm…plication/xml;q=0.9,*/*;q=0.8"
	};

	string post_data = "DlfrForm=DlfrForm&DlfrForm%3AdatalistPrijato_scrollState=0%2C0&DlfrForm%3A"
					   "datalistKdykolikList_scrollState=0%2C0&DlfrForm%3AdownloadXml=&"
					   "javax.faces.ViewState="
					 + code;

	return ( curl_wrapper.HTTP_Post( "https://web3.teledin.cz/WebEdi2/faces/secu/dlfr/DlfrDetail.xhtml"
							   	   , "https://web3.teledin.cz/WebEdi2/faces/secu/index.xhtml"
								   , headers
								   , post_data ) );
}

/*******************************************************************************
 *
 */
vector<string> ParsePartNumbers( string src )
{
	vector<string> part_numbers;

	// skip to select start
	size_t pos = src.find( "id=\"DlfrListForm:part_input\"" );
	if ( pos == string::npos ) return part_numbers;
	src = src.substr ( pos );

	//skip first item ("Vyber...")
	pos = src.find( "/option" );
	if ( pos == string::npos ) return part_numbers;
	src = src.substr ( pos );

	//cut the part after select
	pos = src.find( "</select>" );
	src = src.substr( 0, pos );

	while ( ( ( pos = src.find( "<option value=\"" ) ) != string::npos ) )
	{
		pos += strlen( "<option value=\"" );
		src = src.substr ( pos );

		//substruct part name
		pos = src.find( "\"" );
		string part_name = src.substr( 0, pos );

		part_numbers.push_back( part_name );
	}

	return part_numbers;
}

/*******************************************************************************
 *
 */
bool ForEachReference( CurlWrapper & curl_wrapper, const string & code, const string & part_name, const vector<string> & data_rk_list, CFiles & output_file )
{
	for ( auto & i : data_rk_list )
	{
		/*
		 * Display details of 1 reference
		 */
		if ( GetReference_FirstRequest( curl_wrapper, code, part_name, i ) ) return -1;
		if ( GetReference_SecondRequest( curl_wrapper, code, part_name, i ) ) return -2;

		/*
		 * Export/download 1 reference
		 */
		if ( ExportReference( curl_wrapper, code ) ) return -3;
#if 0
		if ( store_to_file )
		{
			sprintf(store_name, "./Output/reference_%.03u.xml", reference_counter);
			store_file = fopen(store_name, "w");
			if (store_file != NULL)
			{
				//todo - copy buffer to the file
				fclose(store_file);
			}
		}
		else
#endif

		if ( ReadFile( curl_wrapper.GetResponse( ), output_file.Get( ) , output_file.GetAll( ) ) != 0 ) return -4;

		reference_counter++;

		char buf[100];
		sprintf(buf, "Odvolávka #%.03d zpracována", reference_counter);
		LogMsg(buf);
	}
	return false;
}

/*******************************************************************************
 *
 */
bool ForEachPartNumber( CurlWrapper & curl_wrapper, const string & code, const vector<string> & part_numbers, CFiles & output_files )
{
	for ( auto & i : part_numbers )
	{
		/*
		 * Select 1 item
		 */
		if ( ChoosePart_FirstRequest( curl_wrapper, code, i ) ) return -1;
//		if ( ChoosePart_SecondRequest( curl_wrapper, code, i ) ) return -2; the application seems to work without this code

		vector<string> data_rk_list = GetDataRkList( curl_wrapper.GetResponse( ) );

		if ( ForEachReference( curl_wrapper, code, i, data_rk_list, output_files ) ) return -3;

		part_number_counter++;

		if ( part_number_counter > 3 )
			break;

		cout << "\rOdvolávky pro díl č." << part_number_counter << " z " << part_numbers.size() <<" uloženy" << flush;
	}

	return false;
}

/*******************************************************************************
 *
 */
int ProcessCurl( CFiles & output_files )
{
	LogMsg("Download references (odvolavky)");

	list<string> headers = {};
	string session_id = "";
	string code = "";
	FILE * p_categories = NULL;
	bool debug = false;

	CurlWrapper curl_wrapper( configuration.proxy, debug );

	/*
	 *  Get login page
	 */
	if ( GetLoginPage( curl_wrapper ) ) return -1;

	code = GetSecreteCode( curl_wrapper.GetResponse( ) );
	session_id = curl_wrapper.GetSessionId( );

	/*
	 * Post Login information
	 */
	if ( PostLoginPage( curl_wrapper, session_id, code ) ) return -2;

	/*
	 * Get Index
	 */
	if ( GetIndexPage( curl_wrapper ) ) return -3;

	/*
	 * Get list of references
	 */
	code = GetSecreteCode( curl_wrapper.GetResponse( ) );

	if ( PostIndexPage( curl_wrapper, code ) ) return -4;

	code = GetSecreteCode( curl_wrapper.GetResponse( ) );

	vector<string> part_numbers = ParsePartNumbers( curl_wrapper.GetResponse( ) );

	/*
	 * Only create category file and quit, otherwise download all references
	 */
	if ( ( p_categories = output_files.GetCategories( ) ) != NULL )
	{
		for ( auto & i : part_numbers )	PrintCategory( p_categories, ( char * )( i.c_str( ) ) );

		cout << "Categories file created." << flush;
	}
	else
	{
		if ( ForEachPartNumber( curl_wrapper, code, part_numbers, output_files ) ) return -5;

		char buf[ 100 ];
		sprintf( buf, "%d parts, %d references downloaded", part_number_counter, reference_counter );
		LogMsg( buf );
	}

 	return 0;
}

/*******************************************************************************
 * Return 1 = end program, 0 = ok, else error
 */
int ParseArguments( int argc, char *argv[], bool & create_categories )
{
	for ( int i = 1; i < argc; i++ )
	{
		if ( !strcmp(argv[i], "-v" ) )
		{
			cout << "Version:" << VERSION;
			return 1;
		}
		else if ( !strcmp( argv[i], "-d" ) )
		{
			EnableDebugMode( 2 );
		}
		else if ( !strcmp( argv[i], "-h" ) )
		{
			printf( "Syntax: %s [-d] [-h | -v | -s]\n", argv[0] );
			puts( "\t-d\tEnable debug messages" );
			puts( "\t-h\tPrint help and exit" );
			puts( "\t-v\tPrint version and exit" );
			puts( "\t-s\tCreate category file" );

			return 1;
		}
		else if ( !strcmp(argv[i], "-s" ) )
		{
			create_categories = true;
		}
	}
	return 0;
}


/*******************************************************************************
 *
 */
int main(int argc, char *argv[])
{
	LogMsg("Application start");
	//	static DebugTime debug_time;
	bool create_categories = false;

	if ( ParseArguments( argc, argv, create_categories ) )
		return 0;

	InitConfiguration( );

	CFiles output_files( create_categories );

	if ( ProcessCurl( output_files ) ) Error("HTTP error");

	LogMsg("Application end");

//	cout << endl << debug_time.Update( ) << endl;

	return 0;
}

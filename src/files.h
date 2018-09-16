/*
 * files.h
 *
 *  Created on: Sep 15, 2018
 *      Author: E9848823
 */

#ifndef FILES_H_
#define FILES_H_

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "parsexml.h"
#include "utils.h"

using namespace std;

/*******************************************************************************
 *
 */
class CFiles {
	public:
		CFiles ( bool create_categories );
		~CFiles ();
		FILE * Get( void ) { return mp_output; }
		FILE * GetAll( void ) { return mp_output_all; }
		FILE * GetCategories( void ) { return mp_categories; }
	private:
		void openOutputFiles( void );
		static int copyOutputFile( string time_string, string prefix );

		string m_time_string = GetTimeString(tfFILE_NAME);
		FILE * mp_output = NULL;
		FILE * mp_output_all = NULL;
		FILE * mp_categories = NULL;
};


/*******************************************************************************
 *
 */
CFiles::CFiles( bool create_categories )
{
	LogMsg("Create output files.");

	mkdir("Output", 0777);

	if ( create_categories )
	{
		if ( ( mp_categories = fopen( "categories.xml", "wb" ) ) == NULL ) throw( "Error opening categories file" );
		fprintf( mp_categories, "<Categories>\r\n" );
	}
	else
	{
		openOutputFiles( );
		PrintHeader( mp_output );
		PrintHeader( mp_output_all );
	}
}

/*******************************************************************************
 *
 */
CFiles::~CFiles( )
{
	LogMsg( "Copy output file." );

	if ( mp_output )
	{
		fclose( mp_output );

		if ( copyOutputFile( m_time_string, "" ) )
			ErrorCode( "Error creating copy of the output file", 0 );
	}
	if ( mp_output )
		fclose( mp_output );

	if ( mp_output_all )
		fclose( mp_output_all );

	if ( mp_categories )
	{
		fprintf( mp_categories, "</Categories>\r\n" );
		fclose( mp_categories );
	}

}

/*******************************************************************************
 *
 */
void CFiles::openOutputFiles( )
{
	string output_file_name = "./output/" + m_time_string + ".csv";
	if ( ( mp_output = fopen(output_file_name.c_str(), "wb") ) == NULL ) throw( "Error opening output.csv file" );

	fputc(0xEF, mp_output);
	fputc(0xBB, mp_output);
	fputc(0xBF, mp_output);

	output_file_name = "./output/" + m_time_string + "_all.csv";
	if ( ( mp_output_all = fopen(output_file_name.c_str(), "wb") ) == NULL ) throw( "Error opening output_all.csv file" );

	fputc(0xEF, mp_output);
	fputc(0xBB, mp_output);
	fputc(0xBF, mp_output);
}

/*******************************************************************************
 *
 */
int CFiles::copyOutputFile( string time_string, string prefix )
{
	FILE *copy_file = NULL;
	FILE *source_file = NULL;
	char buf[100];
	int len;
	int rslt = -1;

	string source_file_name = "./output/" + prefix + time_string + ".csv";
	source_file = fopen(source_file_name.c_str(), "rb");

	string copy_file_name = "./output/" + prefix + "last.csv";
	copy_file = fopen(copy_file_name.c_str(), "wb");

	if ( copy_file )
	{
		rslt = -2;

		if ( source_file )
		{
			while ( ( len = fread(buf, 1, 100, source_file ) ) )
			{
				fwrite( buf, 1, len, copy_file );
			}

			rslt = 0;
		}
		fclose( copy_file );
	}

	return rslt;
}

#endif /* FILES_H_ */

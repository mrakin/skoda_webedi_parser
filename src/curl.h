/*
 * curl.h
 *
 *  Created on: Sep 14, 2018
 *      Author: E9848823
 */

#ifndef CURL_H_
#define CURL_H_

#include <array>
#include <curl/curl.h>
#include <curl/easy.h>
#include "curl_debug.h"
#include "utils.h"


using namespace std;

size_t ResponseToBuffer_Callback( void *ptr, size_t size, size_t nmemb, string * p_buffer );
size_t DropResponse_Callback( void *ptr, size_t size, size_t nmemb, void * numb );
size_t PostData_Callback(void *ptr, size_t size, size_t nmemb, string * p_buffer );

/******************************************************************************
 *
 */
class CurlWrapper {
	public:
		CurlWrapper( const string & proxy, bool debug );
		~CurlWrapper( );
		string GetSessionId( void );
		bool HTTP_Get( const string & url, const string & referer, const list<string> & headers );
		bool HTTP_Post( const string & url, const string & referer, const list<string> & headers, const string post_data, bool drop_response = false );
		const string & GetResponse( void ) { return m_r_data; }
	private:
		bool HTTP_Request( const string & url, const string & referer, const list<string> & headers, bool drop_response = false );
		CURLcode m_res;
		CURL * mp_curl;
		CurlDebug * mp_debug;
		string m_r_data;
		string m_s_data;
};

/******************************************************************************
 *
 */
CurlWrapper::CurlWrapper( const string & proxy, bool debug = false )
			: m_res( CURLE_OK ), mp_curl( NULL ), mp_debug ( NULL ), m_r_data( "" ), m_s_data( "" )
{
	if ( !( mp_curl = curl_easy_init( ) ) ) throw "Curl init error!";

	if ( debug )
	{
		mp_debug = new CurlDebug( );

		curl_easy_setopt( mp_curl, CURLOPT_VERBOSE, 1L );
		curl_easy_setopt( mp_curl, CURLOPT_DEBUGFUNCTION, DebugTrace );
		curl_easy_setopt( mp_curl, CURLOPT_DEBUGDATA, mp_debug );
	}

	if ( proxy != "" ) curl_easy_setopt( mp_curl, CURLOPT_PROXY, proxy.c_str( ) );

	curl_easy_setopt(mp_curl, CURLOPT_TCP_KEEPALIVE, 1L);
	curl_easy_setopt(mp_curl, CURLOPT_COOKIEFILE, "");
//	curl_easy_setopt(mp_curl, CURLOPT_COOKIE, 1L);
	curl_easy_setopt(mp_curl, CURLOPT_FOLLOWLOCATION, 1L);
	curl_easy_setopt(mp_curl, CURLOPT_SSL_VERIFYPEER , 0);
	curl_easy_setopt(mp_curl, CURLOPT_SSL_VERIFYHOST , 1);
	curl_easy_setopt(mp_curl, CURLOPT_CAINFO, "./CAcertificate.crt");
	curl_easy_setopt(mp_curl, CURLOPT_CAPATH, "./CAcertificate.crt");
	curl_easy_setopt(mp_curl, CURLOPT_SSL_SESSIONID_CACHE, 1L);
}

/******************************************************************************
 *
 */
CurlWrapper::~CurlWrapper( )
{
	delete mp_debug;

	if ( mp_curl ) curl_easy_cleanup(mp_curl);
}

/*******************************************************************************
 *
 */
bool CurlWrapper::HTTP_Request( const string & url, const string & referer, const list<string> & headers, bool drop_response )
{
	m_r_data = "";

	curl_easy_setopt( mp_curl, CURLOPT_URL, url.c_str() );
	curl_easy_setopt( mp_curl, CURLOPT_REFERER, referer.c_str() );

	if ( drop_response )
	{
		curl_easy_setopt( mp_curl, CURLOPT_WRITEFUNCTION, DropResponse_Callback );
		curl_easy_setopt(mp_curl, CURLOPT_WRITEDATA, NULL );
	}
	else
	{
		curl_easy_setopt( mp_curl, CURLOPT_WRITEFUNCTION, ResponseToBuffer_Callback );
		curl_easy_setopt(mp_curl, CURLOPT_WRITEDATA, &m_r_data );
	}

	struct curl_slist *list = NULL;
	for (const auto & h : headers) list = curl_slist_append( list, h.c_str( ) );
	curl_easy_setopt( mp_curl, CURLOPT_HTTPHEADER, list );

	if ( ( m_res = curl_easy_perform( mp_curl ) ) != CURLE_OK )
	{
		cout << curl_easy_strerror(m_res) << endl;
		ErrorCode("HTTP_Request", m_res);
		return true;
	}
	return false;
}

/*******************************************************************************
 *
 */
bool CurlWrapper::HTTP_Get( const string & url, const string & referer, const list<string> & headers )
{
	curl_easy_setopt( mp_curl, CURLOPT_POST, 0) ;
	curl_easy_setopt( mp_curl, CURLOPT_POSTFIELDSIZE, 0 );

	curl_easy_setopt( mp_curl, CURLOPT_READFUNCTION, NULL );
	curl_easy_setopt( mp_curl, CURLOPT_READDATA, NULL );

	return HTTP_Request( url, referer, headers );
}

/*******************************************************************************
 *
 */
bool CurlWrapper::HTTP_Post( const string & url, const string & referer, const list<string> & headers, const string post_data, bool drop_response )
{
	m_s_data = post_data;

	curl_easy_setopt( mp_curl, CURLOPT_POST, 1 );
	curl_easy_setopt( mp_curl, CURLOPT_POSTFIELDSIZE, post_data.length( ) );

	curl_easy_setopt( mp_curl, CURLOPT_READFUNCTION, PostData_Callback );
	curl_easy_setopt( mp_curl, CURLOPT_READDATA, &m_s_data );

	return HTTP_Request( url, referer, headers, drop_response );
}

/*******************************************************************************
 *
 */
string CurlWrapper::GetSessionId( void )
{
	CURLcode res;
	char * tmp = NULL;

    struct curl_slist *cookies = NULL;
    res = curl_easy_getinfo( mp_curl, CURLINFO_COOKIELIST, &cookies );
    if(!res && cookies) {
      /* a linked list of cookies in cookie file format */
      struct curl_slist *each = cookies;
      while(each) {
//        printf( "%s\n\r", each->data );
        tmp = strstr( each->data, "SESSIONID" );

        if ( tmp )
    	{
        	tmp += 1 + strlen( "SESSIONID" );

        	return string( tmp );
    	}

        each = each->next;
      }
    }

	return "";
}

/******************************************************************************
 *
 */size_t ResponseToBuffer_Callback( void *ptr, size_t size, size_t nmemb, string * p_buffer )

{
	int copy_size = size * nmemb;
	p_buffer->append( ( char * )ptr, copy_size );

	return copy_size;
}

/******************************************************************************
 *
 */
size_t DropResponse_Callback( void *ptr, size_t size, size_t nmemb, void * numb )
{
	return size * nmemb;
}

/******************************************************************************
 *
 */
size_t PostData_Callback(void *ptr, size_t size, size_t nmemb, string * p_buffer )
{
	size_t post_length = p_buffer->length( );
	size_t max_lenght = size * nmemb;

	if ( post_length > max_lenght )
		post_length = max_lenght;

	//copy max length and move inside the buffer
	memcpy( ptr, p_buffer->c_str( ), post_length );

	//todo - overit, ze tahle konstrukce s pointerem funguje!!
	*p_buffer = p_buffer->substr( post_length );

	return post_length;
}

/******************************************************************************
 *
 */


#endif /* CURL_H_ */

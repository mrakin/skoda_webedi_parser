/*
 * curl_debug.h
 *
 *  Created on: Sep 14, 2018
 *      Author: E9848823
 */

#ifndef CURL_DEBUG_H_
#define CURL_DEBUG_H_

#include "Windows.h"

/******************************************************************************
 *
 */
class DebugTime {
	public:
		DebugTime( ) { m_old = m_new = GetTickCount(); };
		const DebugTime & Update( void )
		{
			m_old = m_new;
			m_new = GetTickCount();
			return *this;
		};
		friend std::ostream & operator << ( std::ostream & os, const DebugTime & x );
	private:
		void PrintTime( std::ostream& os ) const
		{
			os << "" << ( double( m_new - m_old ) / 1000) << " seconds";
		}
		size_t m_old;
		size_t m_new;
};

std::ostream& operator << ( std::ostream& os, const DebugTime & x )
{
	x.PrintTime( os );
    return os;
}

/******************************************************************************
 *
 */
class CurlDebug {
	public:
		CurlDebug( );
		~CurlDebug( ) { if ( m_stream ) fclose( m_stream ); }
		int my_trace( curl_infotype type, char *data, size_t size );
	private:
		void dump( const char *text,
		          FILE *stream, unsigned char *ptr, size_t size );
		bool m_trace_ascii;
		FILE * m_stream;
};

/******************************************************************************
 *
 */
CurlDebug::CurlDebug( )
{
	m_trace_ascii = 1;
	m_stream = fopen("curl_debug.txt", "wb");
	if ( m_stream == NULL ) throw "Opening debug file failed!";
}

/******************************************************************************
 *
 */
void CurlDebug::dump(const char *text,
          FILE *stream, unsigned char *ptr, size_t size )
{
  size_t i;
  size_t c;

  unsigned int width = 0x10;
  if(m_trace_ascii)
    /* without the hex output, we can fit more on screen */
    width = 200;//0x40;
#if 0
  fprintf(stream, "%s, %10.10lu bytes (0x%8.8lx)\n",
          text, (unsigned long)size, (unsigned long)size);
#endif

  for(i = 0; i<size; i += width) {

//    fprintf(stream, "%4.4lx: ", (unsigned long)i);

    if(!m_trace_ascii) {
      /* hex not disabled, show it */
      for(c = 0; c < width; c++)
        if(i + c < size)
          fprintf(stream, "%02x ", ptr[i + c]);
        else
          fputs("   ", stream);
    }

    for(c = 0; (c < width) && (i + c < size); c++) {
      /* check for 0D0A; if found, skip past and start a new line of output */
      if(m_trace_ascii && (i + c + 1 < size) && ptr[i + c] == 0x0D &&
         ptr[i + c + 1] == 0x0A) {
        i += (c + 2 - width);
        break;
      }
      fprintf(stream, "%c",
              (ptr[i + c] >= 0x20) && (ptr[i + c]<0x80)?ptr[i + c]:'.');
      /* check again for 0D0A, to avoid an extra \n if it's at width */
      if(m_trace_ascii && (i + c + 2 < size) && ptr[i + c + 1] == 0x0D &&
         ptr[i + c + 2] == 0x0A) {
        i += (c + 3 - width);
        break;
      }
    }
    fputc('\n', stream); /* newline */
  }
  fflush(stream);
}

/******************************************************************************
 *
 */
int CurlDebug::my_trace( curl_infotype type, char *data, size_t size )
{
  const char *text;

  switch(type) {
  case CURLINFO_TEXT:
    //fprintf(stderr, "== Info: %s", data);
    /* no break */
  default: /* in case a new one is introduced to shock us */
    return 0;

  case CURLINFO_HEADER_OUT:
    text = "=> Send header";
    break;
  case CURLINFO_DATA_OUT:
    text = "=> Send data";
    break;
  case CURLINFO_SSL_DATA_OUT:
    text = "=> Send SSL data";
    break;
  case CURLINFO_HEADER_IN:
    text = "<= Recv header";
    break;
  case CURLINFO_DATA_IN:
    text = "<= Recv data";
    break;
  case CURLINFO_SSL_DATA_IN:
    text = "<= Recv SSL data";
    break;
  }

//  dump(text, stderr, (unsigned char *)data, size, m_trace_ascii);
  dump( text, m_stream, (unsigned char *)data, size );
  return 0;
}

/******************************************************************************
 *
 */
int DebugTrace( CURL *handle, curl_infotype type,
		             char *data, size_t size,
		             void *userp )
{
	(void)handle; /* prevent compiler warning */

	CurlDebug * curl_debug = ( CurlDebug * )userp;

	return curl_debug->my_trace( type, data, size );
}

#endif /* CURL_DEBUG_H_ */


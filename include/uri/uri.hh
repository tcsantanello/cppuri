/* -*- Mode: c++ -*- */
/*
 * Copyright (c) 2017-2019, Thomas Santanello
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#ifndef __URI_URI__
#define __URI_URI__

#include <functional>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

class Uri {
 public:
  static constexpr const char *SCHEME   = "scheme";
  static constexpr const char *HOST     = "host";
  static constexpr const char *PORT     = "port";
  static constexpr const char *USER     = "user";
  static constexpr const char *PASSWORD = "password";
  static constexpr const char *RESOURCE = "resource";
  static constexpr const char *FRAGMENT = "fragment";
  static constexpr const char *QUERY    = "query";
  static constexpr const char *URI      = "uri";

  static Uri *parse( const std::string &uri ) noexcept( false );

  virtual ~Uri( ) = default;

  virtual std::string getComponent( const std::string & ) const        = 0;
  virtual std::string setComponent( const std::string &, std::string ) = 0;

                      operator std::string( ) { return toString( ); }
  virtual std::string toString( ) = 0;

  virtual bool opaque( ) const = 0;
  virtual bool opaque( bool )  = 0;

  virtual std::unordered_map< std::string, std::string > getQuery( ) const                     = 0;
  virtual std::vector< std::string >                     getQuery( const std::string & ) const = 0;
  virtual bool                                           removeQuery( const std::string & )    = 0;
  virtual bool removeQuery( const std::string &, const std::string & )                         = 0;
  virtual bool addQuery( std::string, std::string )                                            = 0;

  std::string scheme( ) const { return getComponent( SCHEME ); }
  std::string scheme( std::string value ) { return setComponent( SCHEME, std::move( value ) ); }

  std::string host( ) const { return getComponent( HOST ); }
  std::string host( std::string value ) { return setComponent( HOST, std::move( value ) ); }

  int port( ) const {
    std::string val = getComponent( PORT );
    try {
      if ( !val.empty( ) ) {
        return std::stoi( val );
      }
    } catch ( ... ) {
    }
    return 0;
  }

  int port( int port ) {
    std::stringstream ss;
    ss << port;
    try {
      return std::stoi( setComponent( PORT, ss.str( ) ) );
    } catch ( ... ) {
      return 0;
    }
  }

  std::string user( ) const { return getComponent( USER ); }
  std::string user( std::string value ) { return setComponent( USER, std::move( value ) ); }

  std::string password( ) const { return getComponent( PASSWORD ); }
  std::string password( std::string value ) { return setComponent( PASSWORD, std::move( value ) ); }

  std::string resource( ) const { return getComponent( RESOURCE ); }
  std::string resource( std::string value ) { return setComponent( RESOURCE, std::move( value ) ); }

  std::string fragment( ) const { return getComponent( FRAGMENT ); }
  std::string fragment( std::string value ) { return setComponent( FRAGMENT, std::move( value ) ); }

  template < class I >
  bool addQuery( std::string name, I begin, I end ) {
    while ( begin != end ) {
      if ( !addQuery( name, *( begin++ ) ) ) {
        return false;
      }
    }
    return true;
  }

  template < class I >
  bool addQuery( I begin, I end ) {
    while ( begin != end ) {
      if ( !addQuery( begin->first, begin->second ) ) {
        return false;
      }
    }
    return true;
  }

  /**
   * @brief Un-escape a URI value; convert percent encodings to normal characters
   * @param value component value to un-escape
   * @return normalized value
   */
  static std::string unescape( const std::string &value ) {
    std::stringstream ss;

    auto call = []( char val ) -> uint8_t {
      if ( ( val >= 'A' ) && ( val <= 'F' ) ) {
        return ( val - 'A' ) + 10;
      } else if ( ( val >= 'a' ) && ( val <= 'f' ) ) {
        return ( val - 'a' ) + 10;
      } else if ( ( val >= '0' ) && ( val <= '9' ) ) {
        return ( val - '0' );
      }
      return 0;
    };

    for ( auto iterator = value.begin( ); iterator != value.end( ); ++iterator ) {
      if ( *iterator == '%' ) {
        if ( value.end( ) - iterator >= 2 ) {
          ss << ( char ) ( ( ( call( iterator[ 1 ] ) << 4 ) | call( iterator[ 2 ] ) ) );
          iterator += 2;
        }
      } else {
        ss << *iterator;
      }
    }

    return ss.str( );
  }

  /**
   * @breif Escape the URI value to eliminate the possibility of character conflicts that would
   * cause a parsing problem
   * @param value URI value
   * @return normalized / escaped value
   */
  static std::string escape( const std::string &value ) {
    std::stringstream ss;

    for ( auto ch : value ) {
      switch ( ch ) { // clang-format off
        case ':': case '/': case '?': case '#': case '[': case ']':
        case '@': case '%': case '!': case '$': case '&': case '\'':
        case '(': case ')': case '*': case '+': case ',': case ';':
        case ' ': case '=': {
          // clang-format on
          ss << '%'                                                     //
             << std::hex << std::uppercase << ( ( ( ch ) &0xF0 ) >> 4 ) //
             << std::hex << std::uppercase << ( ( ( ch ) &0x0F ) >> 0 );
          break;
        }
        default: {
          ss << ch;
          break;
        }
      }
    }

    return ss.str( );
  }

  /* - * - * - * - * - * - * - * - * - * - * - * - * - * - */

  typedef std::function< bool( Uri &, std::string ) > UriParser;
  typedef std::function< std::string( const Uri & ) > UriBuilder;

  static void registerScheme( const std::string &, UriParser, UriBuilder );
};

#endif

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

#include "uri/uri.hh"

#include <map>
#include <netdb.h>
#include <sstream>
#include <string>

#include <iostream>

static bool        default_parse( Uri &, std::string );
static std::string default_build( const Uri & );

class UriImpl;

struct UriFormat {
  Uri::UriParser  parse;
  Uri::UriBuilder build;
};
typedef std::map< std::string, UriFormat > UriFormatMap;

static UriFormatMap schemes;
UriFormat           DEFAULT = { default_parse, default_build };

/**
 * @brief Parse the authority section
 * @param uri Uri object
 * @param authority authority sub-string
 */
inline static void authority_parse( Uri &uri, const std::string &authority ) {
  std::string::size_type host{ };
  std::string::size_type port{ };
  std::string::size_type user = std::string::npos;
  std::string::size_type pass{ };

  if ( !authority.empty( ) ) {
    host = authority.rfind( '@' );
    port = authority.rfind( ':' );
    pass = authority.find( ':' );

    if ( host == std::string::npos ) {
      host = 0;
    } else {
      ++host;
      user = 0;
    }

    if ( pass > host ) {
      pass = std::string::npos;
    }

    if ( port < host ) {
      port = std::string::npos;
    }

    if ( user != std::string::npos ) {
      if ( pass != std::string::npos ) {
        uri.password( authority.substr( pass + 1, host - pass - 2 ) );
        uri.user( authority.substr( user, pass - user ) );
      } else {
        uri.user( authority.substr( user, host - user - 1 ) );
      }
    }

    if ( port != std::string::npos ) {
      uri.setComponent( Uri::PORT, authority.substr( port + 1 ) );
      uri.host( authority.substr( host, port - host ) );
    } else {
      uri.host( authority.substr( host ) );
    }
  }
}

/**
 * Get the format parameters for the identified scheme type
 * @param scheme format scheme/type
 * @return Uri formatting functions
 */
static UriFormat getSchemeFormat( const std::string &scheme ) {
  auto iterator = schemes.find( scheme );
  return ( iterator == schemes.end( ) ) ? DEFAULT : iterator->second;
}

/**
 * @brief Register a custom scheme builder and parser
 * @param scheme format's scheme name
 * @param parser parsing function
 * @param builder building function
 */
void Uri::registerScheme( const std::string &scheme,
                          Uri::UriParser     parser,
                          Uri::UriBuilder    builder ) {
  schemes.insert( { scheme, UriFormat{ std::move( parser ), std::move( builder ) } } );
}

/**
 * Uri Implementation
 */
class UriImpl : public Uri {
  friend std::string default_build( const Uri &uri );

 private:
  std::map< std::string, std::string >      components;
  std::multimap< std::string, std::string > queryFields;
  bool                                      isOpaque;
  bool                                      hasPort;

  void clear( ) {
    queryFields.clear( );
    components.clear( );
    isOpaque = true;
    hasPort  = false;
  }

  /**
   * @brief Parse a string representation of a URI
   * @param uri URI string
   * @return true on success, false on failure
   */
  bool parse( std::string &uri ) {
    std::string::size_type colon = uri.find( ':' );
    std::string            scheme;
    UriFormat              format;

    clear( );

    if ( colon != std::string::npos ) {
      scheme = uri.substr( 0, colon );
      this->scheme( scheme );
    }

    format = getSchemeFormat( scheme );

    if ( colon != std::string::npos ) {
      opaque( uri.substr( colon, 2 ) != ":/" );
    }

    return format.parse( *this, uri );
  }

 public:
  /**
   * @brief Parsing constructor
   * @note Will throw a runtime error if the URI is not parse-able
   * @param uri URI to parse
   */
  explicit UriImpl( std::string uri )
    : isOpaque( true )
    , hasPort( false ) {
    if ( !parse( uri ) ) {
      throw std::runtime_error( "[" + uri + "] is not a valid URI" );
    }
  }

  /**
   * @brief Get the component value
   * @param name field name
   * @return field value
   */
  std::string getComponent( const std::string &name ) const override {
    std::string value;

    if ( name != Uri::QUERY ) {
      auto component = components.find( name );
      if ( component != components.end( ) ) {
        value = component->second;
      }
    } else if ( !queryFields.empty( ) ) {
      std::stringstream ss;

      for ( auto &it : queryFields ) {
        ss << escape( ( it ).first ) << "=" << escape( ( it ).second ) << "&";
      }

      value = ss.str( );
      value.erase( value.length( ) - 1 );
    }
    return value;
  }

  /**
   * @brief Set the value of a component field
   * @param name field name
   * @param value field value
   */
  std::string setComponent( const std::string &name, std::string value ) override {
    std::string old;
    auto        component = components.find( name );

    if ( name == Uri::SCHEME ) {
      struct servent *service = getservbyname( value.c_str( ), nullptr );

      if ( service ) {
        port( ntohs( service->s_port ) );
        hasPort = false;
      }
    } else if ( name == Uri::PORT ) {
      hasPort = ( ( value.length( ) > 0 ) && ( value != "0" ) );
    }

    if ( component != components.end( ) ) {
      old               = component->second;
      component->second = unescape( value );
    } else {
      components[ name ] = unescape( value );
    }

    components[ URI ] = "";

    return old;
  }

  /**
   * @brief Convert the URI into string form
   * @return URI as a string
   */
  std::string toString( ) override {
    std::string uri = getComponent( URI );

    if ( uri.empty( ) ) {
      std::stringstream ss;
      std::string       fragment = getComponent( FRAGMENT );
      std::string       scheme   = getComponent( SCHEME );
      UriFormat         format   = getSchemeFormat( scheme );

      ss << scheme                             //
         << ( ( scheme.empty( ) ) ? "" : ":" ) //
         << ( opaque( ) ? "" : "//" )          //
         << format.build( *this );             //

      if ( !fragment.empty( ) ) {
        ss << "#" << fragment;
      }

      uri                     = ss.str( );
      this->components[ URI ] = uri;
    }

    return uri;
  }

  /**
   * Get the opacity of the URI
   * @return opacity (true/false)
   */
  bool opaque( ) const override { return isOpaque; }

  /**
   * Set the URI as opaque
   * @param opaque new opacity value
   * @return old opacity value
   */
  bool opaque( bool opaque ) override {
    bool tmp = isOpaque;
    isOpaque = opaque;
    return tmp;
  }

  /**
   * @brief Get the value(s) of a query field
   * @param key query field name
   * @return set of query values
   */
  std::vector< std::string > getQuery( const std::string &key ) const override {
    std::vector< std::string > query;
    auto                       pair = queryFields.equal_range( key );

    for ( auto iterator = pair.first; iterator != pair.second; ++iterator ) {
      query.push_back( iterator->second );
    }

    return query;
  }

  std::unordered_map< std::string, std::string > getQuery( ) const override {
    std::unordered_map< std::string, std::string > query;
    for ( auto &it : queryFields ) {
      if ( query.find( it.first ) == query.end( ) ) {
        query.emplace( std::make_pair( ( it ).first, ( it ).second ) );
      }
    }
    return query;
  }

  /**
   * @brief Remove a query field's values (name/value pairs)
   * @param key query field name
   * @return true on success, false on failure
   */
  bool removeQuery( const std::string &key ) override {
    queryFields.erase( key );
    return true;
  }

  /**
   * @brief Remove a query specific query field (and value)
   * @param key query field name
   * @param value query field value
   * @return true on success, or false on failure
   */
  bool removeQuery( const std::string &key, const std::string &value ) override {
    auto pair = queryFields.equal_range( key );

    for ( auto iterator = pair.first; iterator != pair.second; ++iterator ) {
      if ( iterator->second == value ) {
        queryFields.erase( iterator );
        return true;
      }
    }

    return false;
  }

  /**
   * @brief Add a query field
   * @param key query field name
   * @param value query field value
   * @return true on success, or false on failure
   */
  bool addQuery( std::string key, std::string value ) override {
    size_t size = queryFields.size( );

    queryFields.insert( { std::move( unescape( key ) ), std::move( unescape( value ) ) } );

    return queryFields.size( ) > size;
  }
};

/**
 * Parse a URI
 * @param uri URI to parse
 * @throw runtime error on parsing or memory allocation
 * @return URI object
 */
Uri *Uri::parse( const std::string &uri ) {
  return new UriImpl( uri );
}

/**
 * @brief Default Uri parser
 * @param uri Uri object
 * @param value uri string representation
 * @return true on success, false on failure
 */
static bool default_parse( Uri &uri, std::string value ) {
  std::string::size_type index = value.find( ':' );
  std::string::size_type query{ };
  std::string::size_type fragment = value.rfind( '#' );
  std::string::size_type resource{ };

  if ( index == std::string::npos ) {
    /*
     * The entirety of the value is the resource... usually a path name
     */
    uri.resource( value );

    return true;
  }

  ++index;

  query = value.find( '?' );

  if ( uri.opaque( ) ) {
    uri.resource( value.substr( index, query ) );
  } else {
    if ( uri.scheme( ) == "file" ) {
      if ( value[ index ] == '/' ) {
        if ( value[ index + 1 ] == '/' ) {
          index += 1;
        } else if ( value[ index + 2 ] == '/' ) {
          index += 2;
        }
      }
    } else {
      index += 1 + ( value[ index + 1 ] == '/' );
    }

    resource = value.find( '/', index );

    if ( resource != std::string::npos ) {
      authority_parse( uri, value.substr( index, resource - index ) );

      if ( query != std::string::npos ) {
        uri.resource( value.substr( resource, query - resource ) );
      } else {
        uri.resource( value.substr( resource ) );
      }
    } else {
      authority_parse( uri, value.substr( index ) );
    }
  }

  if ( fragment != std::string::npos ) {
    uri.fragment( value.substr( fragment ) );
  }

  if ( query != std::string::npos ) {
    std::vector< std::string > pairs;
    auto split = []( std::vector< std::string > &parts, std::string value, std::string delim ) {
      std::string::size_type last = 0;
      for ( std::string::size_type pos = 0; ( pos = value.find( delim, pos ) ) != std::string::npos;
            ++pos ) {
        parts.push_back( value.substr( last, pos - last ) );
        last = pos + delim.length( );
      }
      parts.push_back( value.substr( last ) );
    };

    value = value.substr( query + 1, fragment );

    split( pairs, value, "&" );

    for ( auto &each : pairs ) {
      std::vector< std::string > nvp;

      split( nvp, each, "=" );

      uri.addQuery( nvp[ 0 ], ( nvp.size( ) > 1 ) ? nvp[ 1 ] : "" );
    }
  }

  return true;
}

/**
 * @brief Generic default Uri string builder
 * @param uri Uri object
 * @return stringified Uri
 */
static std::string default_build( const Uri &uri ) {
  std::stringstream ss( uri.scheme( ) );
  std::string       resource = uri.resource( );
  std::string       fragment = uri.fragment( );
  std::string       query    = uri.getComponent( Uri::QUERY );

  if ( !uri.opaque( ) ) {
    std::string user = uri.user( );
    std::string pass = uri.password( );
    std::string host = uri.host( );
    std::string port = uri.getComponent( Uri::PORT );

    if ( !user.empty( ) ) {
      ss << user;

      if ( !pass.empty( ) ) {
        ss << ":" << pass;
      }

      ss << "@";
    }

    ss << host;

    if ( dynamic_cast< const UriImpl * >( &uri )->hasPort ) {
      if ( !port.empty( ) ) {
        ss << ":" << port;
      }
    }

    ss << resource;

    if ( !query.empty( ) ) {
      ss << "?" << query;
    }

    if ( !fragment.empty( ) ) {
      ss << "#" << fragment;
    }
  } else {
    ss << resource;
  }

  return ss.str( );
}

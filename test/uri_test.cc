
#undef NDEBUG
#include "uri/uri.hh"
#include <algorithm>
#include <array>
#include <assert.h>
#include <iomanip>
#include <iostream>

struct UriVerify {
  std::string                                          uri;
  std::string                                          result;
  std::string                                          scheme;
  std::string                                          user;
  std::string                                          pass;
  std::string                                          host;
  std::string                                          resource;
  std::vector< std::pair< std::string, std::string > > params;
  std::string                                          fragment;
  int                                                  port;

  UriVerify( std::string &&                                         _uri,
             std::string &&                                         _result,
             std::string &&                                         _scheme,
             std::string &&                                         _user,
             std::string &&                                         _pass,
             std::string &&                                         _host,
             std::string &&                                         _resource,
             std::vector< std::pair< std::string, std::string > > &&_params,
             std::string &&                                         _fragment,
             int                                                    _port )
    : uri( std::move( _uri ) )
    , result( std::move( _result ) )
    , scheme( std::move( _scheme ) )
    , user( std::move( _user ) )
    , pass( std::move( _pass ) )
    , host( std::move( _host ) )
    , resource( std::move( _resource ) )
    , params( std::move( _params ) )
    , fragment( std::move( _fragment ) )
    , port( _port ) {}

  void perform( ) {
    auto ptr = std::shared_ptr< Uri >( Uri::parse( uri ) );

    std::cout << std::setfill( '-' ) << std::setw( 80 ) << ""
              << "\n"
              << std::setfill( ' ' );
    std::cout << std::setw( 30 ) << std::setiosflags( std::ios::left ) //
              << "Uri: " << uri << "\n";
    std::cout << std::setw( 30 ) << std::setiosflags( std::ios::left ) //
              << "Scheme: " << ptr->scheme( ) << "\n";
    std::cout << std::setw( 30 ) << std::setiosflags( std::ios::left ) //
              << "User: " << ptr->user( ) << "\n";
    std::cout << std::setw( 30 ) << std::setiosflags( std::ios::left ) //
              << "Password: " << ptr->password( ) << "\n";
    std::cout << std::setw( 30 ) << std::setiosflags( std::ios::left ) //
              << "Host: " << ptr->host( ) << "\n";
    std::cout << std::setw( 30 ) << std::setiosflags( std::ios::left ) //
              << "Port: " << ptr->getComponent( Uri::PORT ) << "\n";
    std::cout << std::setw( 30 ) << std::setiosflags( std::ios::left ) //
              << "Resource: " << ptr->resource( ) << "\n";
    std::cout << std::setw( 30 ) << std::setiosflags( std::ios::left ) //
              << "Fragment: " << ptr->fragment( ) << "\n";

    for ( auto &&pair : params ) {
      std::vector< std::string > values = ptr->getQuery( pair.first );
      auto                       iter   = std::find( values.begin( ), values.end( ),
                                                     pair.second );
      if ( iter != values.end( ) ) {
        std::cout << std::setw( 30 ) << std::setiosflags( std::ios::left ) //
                  << ( pair.first + ": " ) << *iter << "\n";
      }
    }

    std::cout << std::setfill( '-' ) << std::setw( 80 ) << ""
              << "\n"
              << std::setfill( ' ' );

    assert( ptr->scheme( ) == scheme );
    assert( ptr->user( ) == user );
    assert( ptr->password( ) == pass );
    assert( ptr->host( ) == host );
    assert( ptr->resource( ) == resource );
    assert( ptr->fragment( ) == fragment );

    try {
      assert( ptr->port( ) == port );
    } catch ( std::invalid_argument &ex ) {
    }

    for ( auto &&pair : params ) {
      std::vector< std::string > values = ptr->getQuery( pair.first );
      auto                       iter   = std::find( values.begin( ), values.end( ),
                                                     pair.second );
      assert( iter != values.end( ) );
    }

    assert( ptr->toString( ) == result );
  }
};

int main( int argc, char *argv[] ) {
  std::vector< UriVerify > tests = {
    UriVerify( "/path/to/some/file",                                   // URI
               "/path/to/some/file",                                   // Expected
               "",                                                     // Scheme
               "",                                                     // User
               "",                                                     // Pass
               "",                                                     // Host
               "/path/to/some/file",                                   // Resource
               std::vector< std::pair< std::string, std::string > >{}, // Params
               "",                                                     // Fragment
               0 ),                                                    // Port
    UriVerify( "file://path/to/some/file",                             // URI
               "file:///path/to/some/file",                            // Expected
               "file",                                                 // Scheme
               "",                                                     // User
               "",                                                     // Pass
               "",                                                     // Host
               "/path/to/some/file",                                   // Resource
               std::vector< std::pair< std::string, std::string > >{}, // Params
               "",                                                     // Fragment
               0 ),                                                    // Port
    UriVerify( "file:/path/to/some/file",                              // URI
               "file:///path/to/some/file",                            // Expected
               "file",                                                 // Scheme
               "",                                                     // User
               "",                                                     // Pass
               "",                                                     // Host
               "/path/to/some/file",                                   // Resource
               std::vector< std::pair< std::string, std::string > >{}, // Params
               "",                                                     // Fragment
               0 ),                                                    // Port
    UriVerify( "http://www.google.com/",                               // URI
               "http://www.google.com/",                               // Expected
               "http",                                                 // Scheme
               "",                                                     // User
               "",                                                     // Pass
               "www.google.com",                                       // Host
               "/",                                                    // Resource
               std::vector< std::pair< std::string, std::string > >{}, // Params
               "",                                                     // Fragment
               80 ),                                                   // Port
    UriVerify( "http://user@www.google.com/",                          // URI
               "http://user@www.google.com/",                          // Expected
               "http",                                                 // Scheme
               "user",                                                 // User
               "",                                                     // Pass
               "www.google.com",                                       // Host
               "/",                                                    // Resource
               std::vector< std::pair< std::string, std::string > >{}, // Params
               "",                                                     // Fragment
               80 ),                                                   // Port
    UriVerify( "http://user:pass@www.google.com/",                     // URI
               "http://user:pass@www.google.com/",                     // Expected
               "http",                                                 // Scheme
               "user",                                                 // User
               "pass",                                                 // Pass
               "www.google.com",                                       // Host
               "/",                                                    // Resource
               std::vector< std::pair< std::string, std::string > >{}, // Params
               "",                                                     // Fragment
               80 ),                                                   // Port
    UriVerify( "http://user:pass@www.google.com:8181/",                // URI
               "http://user:pass@www.google.com:8181/",                // Expected
               "http",                                                 // Scheme
               "user",                                                 // User
               "pass",                                                 // Pass
               "www.google.com",                                       // Host
               "/",                                                    // Resource
               std::vector< std::pair< std::string, std::string > >{}, // Params
               "",                                                     // Fragment
               8181 ),                                                 // Port
    UriVerify( "http://www.google.com/?q=some+query+value",            // URI
               "http://www.google.com/?q=some%2Bquery%2Bvalue",        // Expected
               "http",                                                 // Scheme
               "",                                                     // User
               "",                                                     // Pass
               "www.google.com",                                       // Host
               "/",                                                    // Resource
               std::vector< std::pair< std::string, std::string > >{
                 // Params
                 std::make_pair< std::string, std::string >( "q", "some+query+value" ),
               },
               "",                                              // Fragment
               80 ),                                            // Port
    UriVerify( "http://www.google.com/?q=some%20query%20value", // URI
               "http://www.google.com/?q=some%20query%20value", // Expected
               "http",                                          // Scheme
               "",                                              // User
               "",                                              // Pass
               "www.google.com",                                // Host
               "/",                                             // Resource
               std::vector< std::pair< std::string, std::string > >{
                 // Params
                 std::make_pair< std::string, std::string >( "q", "some query value" ),
               },
               "",                                                     // Fragment
               80 ),                                                   // Port
    UriVerify( "mailto:jerk@wad.com",                                  // URI
               "mailto:jerk@wad.com",                                  // Expected
               "mailto",                                               // Scheme
               "",                                                     // User
               "",                                                     // Pass
               "",                                                     // Host
               "jerk@wad.com",                                         // Resource
               std::vector< std::pair< std::string, std::string > >{}, // Params
               "",                                                     // Fragment
               0 ),                                                    // Port
    UriVerify( "tel:+18008080085",                                     // URI
               "tel:+18008080085",                                     // Expected
               "tel",                                                  // Scheme
               "",                                                     // User
               "",                                                     // Pass
               "",                                                     // Host
               "+18008080085",                                         // Resource
               std::vector< std::pair< std::string, std::string > >{}, // Params
               "",                                                     // Fragment
               0 ),                                                    // Port
  };

  for ( auto &&test : tests ) {
    test.perform( );
  }

  return 0;
}

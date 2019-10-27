CMAKE_MINIMUM_REQUIRED( VERSION 3.8 )

## Set version via CI/CD if available... or default to the below
IF ( NOT VERSION )
  SET( VERSION "0.0.0" )
ENDIF( )

UNSET( __VERSION CACHE )
SET( __VERSION "${VERSION}" CACHE STRING "Project Version" )
PROJECT( uri LANGUAGES CXX VERSION ${__VERSION} )

INCLUDE( CMakePackageConfigHelpers )
IF ( EXISTS "${CMAKE_BINARY_DIR}/conanbuildinfo.cmake" )
  INCLUDE( "${CMAKE_BINARY_DIR}/conanbuildinfo.cmake" )
  CONAN_BASIC_SETUP( )
ENDIF( )

#################
###  Library

ADD_LIBRARY(
  uri SHARED
  src/uri.cc
)

STRING( REGEX MATCH "([0-9]+)" major_version "${PROJECT_VERSION}" )
SET_TARGET_PROPERTIES( uri PROPERTIES SOVERSION ${major_version} )
SET_TARGET_PROPERTIES( uri PROPERTIES VERSION ${PROJECT_VERSION} )
TARGET_COMPILE_FEATURES( uri PUBLIC cxx_std_11 )
TARGET_COMPILE_OPTIONS( uri PRIVATE
  $<IF:$<CONFIG:Debug>,-ggdb3 -O0 -Wall,-Wall>
)

TARGET_INCLUDE_DIRECTORIES(
  uri PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include>
)

#################
###  Unit tests

ENABLE_TESTING( )

ADD_EXECUTABLE( uri_test test/uri_test.cc )
TARGET_LINK_LIBRARIES( uri_test uri )
ADD_TEST( NAME URI COMMAND uri_test )

#################
###  Installation & Packaging

WRITE_BASIC_PACKAGE_VERSION_FILE(
  CppUriConfig.cmake
  COMPATIBILITY SameMajorVersion
)

INSTALL(
  TARGETS  uri
  EXPORT   CppUriTargets
  LIBRARY  DESTINATION lib
  ARCHIVE  DESTINATION lib
  RUNTIME  DESTINATION bin
  INCLUDES DESTINATION include
)

INSTALL(
  EXPORT      CppUriTargets
  DESTINATION lib/cmake/CppUri
  FILE        UriTargets.cmake
  NAMESPACE   TCS::
)

INSTALL( DIRECTORY include DESTINATION include )

INSTALL(
  FILES CppUriConfig.cmake
  ${CMAKE_CURRENT_BINARY_DIR}/CppUriConfig.cmake
  DESTINATION lib/cmake/CppUri
)

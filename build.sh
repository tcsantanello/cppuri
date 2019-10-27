#!/bin/bash

SELF=$0
SDIR=$( cd $( dirname "$SELF" ); pwd )

PROJECT_NAME=$( git remote -v | sed -ne '1 { s,.*/\([^.]\+\)\.git.*,\1,g; p  }' )
PROJECT_VERS=$(
        REV=$( git rev-parse HEAD );
        TAG=$( git tag -l --points-at $REV | tr '-' '.' );
        DIRTY=$( git diff --quiet || echo "-dirty" )
        case "$(git branch)" in
            *master*) echo ${TAG:-${REV:0:6}}${DIRTY:-${DIRTY}}
                      ;;
            *) echo ${REV:0:6}${DIRTY:-${DIRTY}}
               ;;
        esac
  )
PROJECT_CHAN=$(
    case "$( git branch )" in
        *master*) echo "stable"
                  ;;
        *) echo "testing"
           ;;
    esac )
REFERENCE=${PROJECT_NAME}/${PROJECT_VERS}@${CONAN_USERNAME:-${USER}}/${PROJECT_CHAN}
PROFILE_NAME=${CONAN_USERNAME:-$USER}-${PROJECT_CHAN}

# sudo pip install conan --upgrade
conan profile new --detect --force ${PROFILE_NAME} || exit $?
conan profile update settings.compiler.libcxx=libstdc++11 ${PROFILE_NAME} || exit $?
GCC_VER=$( gcc --version | awk '/^gcc/ { print $NF }' )
if test "${GCC_VER//.*}" == "10"; then
    conan profile update settings.compiler.version=${GCC_VER%.*} ${PROFILE_NAME} || exit $?
fi

CONAN_HOME=${CONAN_USER_HOME:-${HOME}}/.conan
CONAN_BASE_PATH=$( sed -n '/\[storage\]/,/^path/ { /^path/ { s,path[ \t]*=[ \t]*,,g; p }  }' \
                       ${CONAN_HOME}/conan.conf )

if test "${CONAN_BASE_PATH:0:1}" == "~"; then 
    CONAN_BASE_PATH=${CONAN_HOME}${CONAN_BASE_PATH:1}
fi

# Export the recipe and source
conan remove -f ${REFERENCE} &> /dev/null
conan export ${SDIR} ${REFERENCE} || exit $?

# Compile it and any missing dependencies
conan create --build missing ${SDIR} ${REFERENCE} || exit $?

# Get the build directory (has the built tests)
BUILD=${CONAN_BASE_PATH}/${REFERENCE//@/\/}/build/
BUILD=${BUILD}/$( ls -1 ${BUILD} )

test ! -e ./build && ln -sf ${BUILD} ./build

( cd "${BUILD}";
  export LD_LIBRARY_PATH=${BUILD}:${BUILD}/lib:${BUILD}/bin:${BUILD}/src
  make test
); RET=$?

exit $RET

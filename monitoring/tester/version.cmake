#basic definitions

set(HEADER_FILE "${DIR}/include/build.h")
set(CACHE_FILE "${DIR}/include/BuildNumberCache.txt")

#Reading data from file + incrementation
IF(EXISTS ${CACHE_FILE})
    file(READ ${CACHE_FILE} BUILD_NUMBER)
    math(EXPR BUILD_NUMBER "${BUILD_NUMBER}+1")
ELSE()
    set(BUILD_NUMBER "764")
ENDIF()

#Update the cache
file(WRITE ${CACHE_FILE} "${BUILD_NUMBER}")

#Create the header
file(WRITE ${HEADER_FILE} "#ifndef BUILD_H\n#define BUILD_H\n\n#define BUILD_NUMBER ${BUILD_NUMBER}\n\n#endif")

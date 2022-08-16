#basic definitions

set(HEADER_FILE "${DIR}/include/increment.h")
set(CACHE_FILE "${DIR}/include/BuildNumberCache.txt")

#Reading data from file + incrementation
IF(EXISTS ${CACHE_FILE})
    file(READ ${CACHE_FILE} INCREMENTED_VALUE)
    math(EXPR INCREMENTED_VALUE "${INCREMENTED_VALUE}+1")
ELSE()
    set(INCREMENTED_VALUE "1")
ENDIF()

#Update the cache
file(WRITE ${CACHE_FILE} "${INCREMENTED_VALUE}")

#Create the header
file(WRITE ${HEADER_FILE} "#ifndef INCREMENT_H\n#define INCREMENT_H\n\n#define INCREMENTED_VALUE ${INCREMENTED_VALUE}\n\n#endif")

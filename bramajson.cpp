﻿// bramajson.cpp : Defines the entry point for the application.
//

#include "bramajson.h"
#include <assert.h>

#define BRAMAJSON_VALIDATE_ASSERT(json_content, expected) { \
    int32_t result;\
    bramajson_parse((json_content), &result);\
    assert(result == expected); \
}

int main()
{
    BRAMAJSON_VALIDATE_ASSERT("[true,false,1,1.2]", BRAMAJSON_SUCCESS);
    BRAMAJSON_VALIDATE_ASSERT("[1,2,3,4]", BRAMAJSON_SUCCESS);

    BRAMAJSON_VALIDATE_ASSERT(NULL, BRAMAJSON_CONTENT_EMPTY);
    BRAMAJSON_VALIDATE_ASSERT("[]", BRAMAJSON_SUCCESS);
    BRAMAJSON_VALIDATE_ASSERT("[", BRAMAJSON_JSON_NOT_VALID);
    BRAMAJSON_VALIDATE_ASSERT("]", BRAMAJSON_JSON_NOT_VALID);
    BRAMAJSON_VALIDATE_ASSERT("{}", BRAMAJSON_SUCCESS);
    BRAMAJSON_VALIDATE_ASSERT("{", BRAMAJSON_JSON_NOT_VALID);
    BRAMAJSON_VALIDATE_ASSERT("}", BRAMAJSON_JSON_NOT_VALID);
    BRAMAJSON_VALIDATE_ASSERT("{]", BRAMAJSON_JSON_NOT_VALID);
    BRAMAJSON_VALIDATE_ASSERT("[}", BRAMAJSON_JSON_NOT_VALID);
    BRAMAJSON_VALIDATE_ASSERT("1024", BRAMAJSON_SUCCESS);
    BRAMAJSON_VALIDATE_ASSERT("1024.1", BRAMAJSON_SUCCESS);
    BRAMAJSON_VALIDATE_ASSERT("+1024", BRAMAJSON_SUCCESS);
    BRAMAJSON_VALIDATE_ASSERT("-1024", BRAMAJSON_SUCCESS);
    BRAMAJSON_VALIDATE_ASSERT("-1024.1", BRAMAJSON_SUCCESS);
    BRAMAJSON_VALIDATE_ASSERT("+1024.1", BRAMAJSON_SUCCESS);
    BRAMAJSON_VALIDATE_ASSERT("true", BRAMAJSON_SUCCESS);
    BRAMAJSON_VALIDATE_ASSERT("false", BRAMAJSON_SUCCESS);
    BRAMAJSON_VALIDATE_ASSERT("null", BRAMAJSON_SUCCESS);

	return 0;
}

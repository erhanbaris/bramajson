// bramajson.cpp : Defines the entry point for the application.
//

#include "bramajson.h"
#include <assert.h>

#define BRAMAJSON_VALIDATE_ASSERT(json_content, expected) { \
    int32_t result;\
    bramajson_parse((json_content), &result);\
    assert(result == expected); \
}

bramajson_object* BRAMAJSON_ARRAY_ASSERT(char const* json_content, int32_t expected, size_t array_size) {
    int32_t result;
    bramajson_object* output =  bramajson_parse((json_content), &result);

    assert(result                == expected);
    assert(output->type          == BRAMAJSON_ARRAY);
    assert(output->_array->count == array_size);
    return output;
}

int main()
{
    BRAMAJSON_VALIDATE_ASSERT("{'erhan': 123}", BRAMAJSON_SUCCESS);

    bramajson_object* output = NULL;
    output = BRAMAJSON_ARRAY_ASSERT("[\"erhan\", \"baris\"]", BRAMAJSON_SUCCESS, 2);
    assert(strcmp("erhan", output->_array->items[0]->_string) == 0);
    assert(strcmp("baris", output->_array->items[1]->_string) == 0);

    output = BRAMAJSON_ARRAY_ASSERT("[  true   ,false \r\n,\r\n1,1.2]", BRAMAJSON_SUCCESS, 4);
    assert(output->_array->items[0]->type == BRAMAJSON_TRUE);
    assert(output->_array->items[1]->type == BRAMAJSON_FALSE);
    assert(output->_array->items[2]->type == BRAMAJSON_INT);
    assert(output->_array->items[2]->_integer == 1);
    assert(output->_array->items[3]->_double ==  1.2);

    output = BRAMAJSON_ARRAY_ASSERT("[1,2,3,4]", BRAMAJSON_SUCCESS, 4);
    assert(output->_array->items[0]->type == BRAMAJSON_INT);
    assert(output->_array->items[1]->type == BRAMAJSON_INT);
    assert(output->_array->items[2]->type == BRAMAJSON_INT);
    assert(output->_array->items[3]->type == BRAMAJSON_INT);
    assert(output->_array->items[0]->_integer == 1);
    assert(output->_array->items[1]->_integer == 2);
    assert(output->_array->items[2]->_integer == 3);
    assert(output->_array->items[3]->_integer == 4);

    output = BRAMAJSON_ARRAY_ASSERT("[[1],1]", BRAMAJSON_SUCCESS, 2);
    assert(output->_array->items[0]->type == BRAMAJSON_ARRAY);
    assert(output->_array->items[1]->type == BRAMAJSON_INT);

    assert(output->_array->items[0]->_array->count              == 1);
    assert(output->_array->items[0]->_array->items[0]->type     == BRAMAJSON_INT);
    assert(output->_array->items[0]->_array->items[0]->_integer == 1);
    assert(output->_array->items[1]->type                       == BRAMAJSON_INT);
    assert(output->_array->items[1]->_integer                   == 1);


    BRAMAJSON_VALIDATE_ASSERT(NULL, BRAMAJSON_CONTENT_EMPTY);
    BRAMAJSON_VALIDATE_ASSERT("'erhan'", BRAMAJSON_SUCCESS);
    BRAMAJSON_VALIDATE_ASSERT("[]", BRAMAJSON_SUCCESS);
    BRAMAJSON_VALIDATE_ASSERT("[", BRAMAJSON_JSON_NOT_VALID);
    BRAMAJSON_VALIDATE_ASSERT("]", BRAMAJSON_JSON_NOT_VALID);
    BRAMAJSON_VALIDATE_ASSERT("{}", BRAMAJSON_SUCCESS);
    BRAMAJSON_VALIDATE_ASSERT("{'erhan': 1}", BRAMAJSON_SUCCESS);
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
    BRAMAJSON_ARRAY_ASSERT("[[[[[[[[[[[[1],1],1],1],1],1],1],1],1],1],1],1]", BRAMAJSON_SUCCESS, 2);
    BRAMAJSON_VALIDATE_ASSERT("{\"menu\": {\n"
                              "  \"id\": \"file\",\n"
                              "  \"value\": \"File\",\n"
                              "  \"popup\": {\n"
                              "    \"menuitem\": [\n"
                              "      {\"value\": \"New\", \"onclick\": \"CreateNewDoc()\"},\n"
                              "      {\"value\": \"Open\", \"onclick\": \"OpenDoc()\"},\n"
                              "      {\"value\": \"Close\", \"onclick\": \"CloseDoc()\"}\n"
                              "    ]\n"
                              "  }\n"
                              "}}", BRAMAJSON_SUCCESS);

	return 0;
}

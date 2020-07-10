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
    size_t counter = 0;
    bramajson_object* item = output->_array->head;
    while(NULL != item) {
        ++counter;
        item = item->next;
    }
    assert(counter == array_size);
    return output;
}

void BRAMAJSON_ARRAY_SIZE_ASSERT(bramajson_object* output, size_t array_size) {
    assert(output->type          == BRAMAJSON_ARRAY);
    size_t counter = 0;
    bramajson_object* item = output->_array->head;
    while(NULL != item) {
        ++counter;
        item = item->next;
    }
    assert(counter == array_size);
}

char* ReadFile(char *filename)
{
    char *buffer = NULL;
    int string_size, read_size;
    FILE *handler = fopen(filename, "r");

    if (handler)
    {
        // Seek the last byte of the file
        fseek(handler, 0, SEEK_END);
        // Offset from the first to the last byte, or in other words, filesize
        string_size = ftell(handler);
        // go back to the start of the file
        rewind(handler);

        // Allocate a string that can hold it all
        buffer = (char*) malloc(sizeof(char) * (string_size + 1) );

        // Read it all in one operation
        read_size = fread(buffer, sizeof(char), string_size, handler);

        // fread doesn't set it so put a \0 in the last position
        // and buffer is now officially a string
        buffer[string_size] = '\0';

        if (string_size != read_size)
        {
            // Something went wrong, throw away the memory and set
            // the buffer to NULL
            free(buffer);
            buffer = NULL;
        }

        // Always remember to close the file.
        fclose(handler);
    }

    return buffer;
}

int main()
{

    bramajson_object* output = NULL;
    BRAMAJSON_VALIDATE_ASSERT("{'erhan': 'test', 'baris': 'test'}", BRAMAJSON_SUCCESS);

    output = BRAMAJSON_ARRAY_ASSERT("[  true   ,false \r\n,\r\n1,1.2]", BRAMAJSON_SUCCESS, 4);
    assert(output->_array->head->type == BRAMAJSON_TRUE);
    assert(output->_array->head->next->type == BRAMAJSON_FALSE);
    assert(output->_array->head->next->next->type == BRAMAJSON_INT);
    assert(output->_array->head->next->next->_integer == 1);
    assert(output->_array->head->next->next->next->_double  ==  1.2);

    BRAMAJSON_VALIDATE_ASSERT("{}", BRAMAJSON_SUCCESS);
    BRAMAJSON_VALIDATE_ASSERT("{'erhan': 'test'}", BRAMAJSON_SUCCESS);
    BRAMAJSON_VALIDATE_ASSERT("[1024]", BRAMAJSON_SUCCESS);
    BRAMAJSON_ARRAY_ASSERT("['erhan', [], [true], ['']]", BRAMAJSON_SUCCESS, 4);
    BRAMAJSON_ARRAY_ASSERT("['erhan', \"baris\", false]", BRAMAJSON_SUCCESS, 3);
    BRAMAJSON_ARRAY_ASSERT("[]", BRAMAJSON_SUCCESS, 0);
    BRAMAJSON_ARRAY_ASSERT("[true]", BRAMAJSON_SUCCESS, 1);
    BRAMAJSON_ARRAY_ASSERT("[true, false]", BRAMAJSON_SUCCESS, 2);
    output = BRAMAJSON_ARRAY_ASSERT("[[true, false]]", BRAMAJSON_SUCCESS, 1);
    assert(output->_array->head->type == BRAMAJSON_ARRAY);
    BRAMAJSON_ARRAY_SIZE_ASSERT(output->_array->head, 2);

    output = BRAMAJSON_ARRAY_ASSERT("[[[true, false]]]", BRAMAJSON_SUCCESS, 1);
    assert(output->_array->head->type == BRAMAJSON_ARRAY);
    BRAMAJSON_ARRAY_SIZE_ASSERT(output->_array->head, 1);
    assert(output->_array->head->_array->head->type == BRAMAJSON_ARRAY);
    BRAMAJSON_ARRAY_SIZE_ASSERT(output->_array->head->_array->head, 2);

    BRAMAJSON_ARRAY_ASSERT("[true, false]", BRAMAJSON_SUCCESS, 2);

    BRAMAJSON_VALIDATE_ASSERT("[true, ,false]", BRAMAJSON_JSON_NOT_VALID);
    BRAMAJSON_VALIDATE_ASSERT("[,]", BRAMAJSON_JSON_NOT_VALID);
    BRAMAJSON_VALIDATE_ASSERT("[,'erhan']", BRAMAJSON_JSON_NOT_VALID);
    BRAMAJSON_VALIDATE_ASSERT("['erhan',]", BRAMAJSON_JSON_NOT_VALID);

    BRAMAJSON_VALIDATE_ASSERT("1024.1", BRAMAJSON_SUCCESS);
    BRAMAJSON_VALIDATE_ASSERT("1024.1", BRAMAJSON_SUCCESS);

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

    output = BRAMAJSON_ARRAY_ASSERT("[\"erhan\", \"baris\"]", BRAMAJSON_SUCCESS, 2);
    assert(strcmp("erhan", output->_array->head->_string) == 0);
    assert(strcmp("baris", output->_array->head->next->_string) == 0);


    output = BRAMAJSON_ARRAY_ASSERT("[1,2,3,4]", BRAMAJSON_SUCCESS, 4);
    assert(output->_array->head->type == BRAMAJSON_INT);
    assert(output->_array->head->next->type == BRAMAJSON_INT);
    assert(output->_array->head->next->next->type == BRAMAJSON_INT);
    assert(output->_array->head->next->next->next->type == BRAMAJSON_INT);
    assert(output->_array->head->_integer == 1);
    assert(output->_array->head->next->_integer == 2);
    assert(output->_array->head->next->next->_integer == 3);
    assert(output->_array->head->next->next->next->_integer == 4);


    output = BRAMAJSON_ARRAY_ASSERT("[true, false, null]", BRAMAJSON_SUCCESS, 3);
    assert(output->_array->head->type == BRAMAJSON_TRUE);
    assert(output->_array->head->next->type == BRAMAJSON_FALSE);
    assert(output->_array->head->next->next->type == BRAMAJSON_NULL);

    output = BRAMAJSON_ARRAY_ASSERT("[[1],1]", BRAMAJSON_SUCCESS, 2);
    assert(output->_array->head->type == BRAMAJSON_ARRAY);
    assert(output->_array->head->next->type == BRAMAJSON_INT);

    BRAMAJSON_ARRAY_SIZE_ASSERT(output->_array->head, 1);
    assert(output->_array->head->_array->head->type     == BRAMAJSON_INT);
    assert(output->_array->head->_array->head->_integer == 1);
    assert(output->_array->head->next->type                       == BRAMAJSON_INT);
    assert(output->_array->head->next->_integer                   == 1);


    BRAMAJSON_VALIDATE_ASSERT(NULL, BRAMAJSON_CONTENT_EMPTY);
    BRAMAJSON_VALIDATE_ASSERT("'erhan'", BRAMAJSON_SUCCESS);
    BRAMAJSON_VALIDATE_ASSERT("[]", BRAMAJSON_SUCCESS);
    BRAMAJSON_VALIDATE_ASSERT("[", BRAMAJSON_JSON_NOT_VALID);
    BRAMAJSON_VALIDATE_ASSERT("erhan", BRAMAJSON_JSON_NOT_VALID);
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
    return 0;
}

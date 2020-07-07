

#pragma once

#include <stdlib.h> 
#include <string.h>
#include <stdbool.h>
#include <stdint.h>
#include <math.h>

#define BRAMAJSON_INT      (1 << 0)
#define BRAMAJSON_STRING   (1 << 1) 
#define BRAMAJSON_FLOAT    (1 << 2)
#define BRAMAJSON_ARRAY    (1 << 3)
#define BRAMAJSON_DICT     (1 << 4)
#define BRAMAJSON_NULL     (1 << 5)
#define BRAMAJSON_TRUE     (1 << 6)
#define BRAMAJSON_FALSE    (1 << 7)

#define BRAMAJSON_SUCCESS         (1 << 0)
#define BRAMAJSON_CONTENT_EMPTY   (1 << 1)
#define BRAMAJSON_JSON_NOT_VALID  (1 << 2)
#define BRAMAJSON_OUT_OF_MEMORY   (1 << 3)

#define is_end()         (context->content_length <= context->cursor_index)
#define get_char()       (context->json_content[context->cursor_index])
#define get_next_char()  (context->content_length > (context->cursor_index + 1) ? context->json_content[context->cursor_index + 1] : 0)
#define increase_index() do {++context->cursor_index;} while(0)


typedef struct _bramajson_object  bramajson_object;
typedef struct _bramajson_pair    bramajson_pair;
typedef struct _bramajson_context bramajson_context;



typedef struct _bramajson_pair {
    char*             key;
    bramajson_object* object;
} bramajson_pair;

typedef struct _bramajson_array {
    size_t             size;
    size_t             count;
    bramajson_object** items;
} bramajson_array;

typedef struct _bramajson_object {
    uint16_t type;
    union
    {
        int32_t           _integer;
        double            _double;
        char*             _string;
        bramajson_object* _object;
        bramajson_pair*   _pair;
        bramajson_array*  _array;
    };
} bramajson_object;


typedef struct _bramajson_context {
    bool        validation;
    size_t      content_length;
    size_t      cursor_index;
    char const* json_content;
} bramajson_context;

/* Public apis */
bramajson_object* bramajson_parse   (char const* json_content,      int32_t* status);
void              bramajson_destroy (bramajson_object* json_object, int32_t* status);

/* Private */
bramajson_object* bramajson_parse_object(bramajson_context* context, int32_t* status);

bramajson_object* bramajson_parse_array(bramajson_context* context, int32_t* status) {
    increase_index();

    bramajson_array* array   =  (bramajson_array*)malloc(sizeof(bramajson_array));
    bramajson_object* result = (bramajson_object*)malloc(sizeof(bramajson_object));
    array->items             = (bramajson_object**)malloc(1 * sizeof(bramajson_object*));
    array->count             = 0;
    array->size              = 1;
    result->_array           = array;
    result->type             = BRAMAJSON_ARRAY;

    char32_t chr             = get_char();
    bool     end_found       = false;

    do {
        if (chr == ']') {
            end_found = true;
            break;
        }

        bramajson_object* item = bramajson_parse_object(context, status);
        chr = get_char();

        if (chr == ',') {
            if (array->count == array->size) {
                array->size *= 2;
                array->items = (bramajson_object**)realloc(array->items, array->size * sizeof(bramajson_object*));
            }

            array->items[array->count++] = item;
            increase_index();
            chr = get_char();
        }

        if (chr == ']') {
            end_found = true;
            break;
        }
    }
    while(BRAMAJSON_SUCCESS == *status && !is_end() && chr != ']');

    if (false == end_found) {
        result  = NULL;
        *status = BRAMAJSON_JSON_NOT_VALID;
    }

    return result;
}

bramajson_object* bramajson_parse_dictionary(bramajson_context* context, int32_t* status) {
    increase_index();

    char32_t chr       = get_char();
    bool     end_found = false;
    do {
        chr = get_char();
        
        if (chr == '}')
            end_found = true;

        increase_index();
    }
    while(!is_end() && chr != '}');

    if (false == end_found)
        *status = BRAMAJSON_JSON_NOT_VALID;

    return (bramajson_object*)malloc(sizeof(bramajson_object));
}

bramajson_object* bramajson_parse_atom(bramajson_context* context, int32_t* status) {
    bramajson_object* object = NULL;
    uint16_t type            = BRAMAJSON_NULL;

    if (0 == strncmp("false", context->cursor_index + context->json_content, 5)) {
        type = BRAMAJSON_FALSE;
        context->cursor_index +=5;
    }
    else if (0 == strncmp("true", context->cursor_index + context->json_content, 4)) {
        type = BRAMAJSON_TRUE;
        context->cursor_index +=4;
    }
    else if (0 == strncmp("null", context->cursor_index + context->json_content, 4)) {
        type = BRAMAJSON_NULL;
        context->cursor_index +=4;
    } else
        *status = BRAMAJSON_JSON_NOT_VALID;

    if (BRAMAJSON_SUCCESS == *status) {
        object       = (bramajson_object*)malloc(sizeof(bramajson_object));
        object->type = type;
    }

    return object;
}


bramajson_object* bramajson_parse_number(bramajson_context* context, int32_t* status) {
    size_t index             = 0;
    bool isMinus             = false;
    int dotPlace             = 0;
    double beforeTheComma    = 0;
    double afterTheComma     = 0;
    size_t start             = context->cursor_index;
    bool isDouble            = false;
    char ch                  = get_char();
    char chNext              = get_next_char();
    bool e_used              = false;
    int e_after              = 0;
    bool plus_used           = false;

    while (!is_end()) {
        if (ch == '-') {
            if ((isMinus || (beforeTheComma > 0 || afterTheComma > 0)) && !e_used)
                break;

            isMinus = true;
        }

        else if (ch == '+') {
            if ((plus_used || (beforeTheComma > 0 || afterTheComma > 0)) && !e_used)
                break;

            plus_used = true;
        }

        else if (index != 0 && (ch == 'e' || ch == 'E')) {
            e_used = true;
        }

        else if (ch == '.') {
            /*if (chNext == '.')
                break;*/

            if (isDouble) {
                return NULL;
            }

            isDouble = true;
        }

        else if (!e_used && (ch >= '0' && ch <= '9')) {
            if (isDouble) {
                ++dotPlace;

                afterTheComma *= (int)pow(10, 1);
                afterTheComma += ch - '0';
            }
            else {
                beforeTheComma *= (int)pow(10, 1);
                beforeTheComma += ch - '0';
            }
        }

        else if (e_used && (ch >= '0' && ch <= '9')) {
            e_after *= (int)pow(10, 1);
            e_after += ch - '0';
        }
        else
            break;

        increase_index();
        ch     = get_char();
        chNext = get_next_char();
        ++index;
    }

    bramajson_object* token = (bramajson_object*)malloc(sizeof (bramajson_object));
    if (NULL == token) {
        return NULL;
    }

    if (!isDouble) {
        token->type = BRAMAJSON_INT;
        token->_integer = beforeTheComma;
    } else {
        token->type    = BRAMAJSON_INT;
        token->_double = (beforeTheComma + (afterTheComma * pow(10, -1 * dotPlace)));
    }

    if (e_used) {
        if (isMinus) {
            token->_double = token->_double / (double)pow((double)10, (double)e_after);
        } else {
            token->_double = token->_double * (double)pow((double)10, (double)e_after);
        }
    }

    if (isMinus && !e_used)
        token->_double *= -1;

    return token;
}

bramajson_object* bramajson_parse_object(bramajson_context* context, int32_t* status) {
    char32_t chr             = context->json_content[context->cursor_index];
    bramajson_object* result = NULL;

    /* Parse array */
    if (chr == '[') {
        result = bramajson_parse_array(context, status);
    }

    /* Parse dictionary */
    else if (chr == '{') {
        result = bramajson_parse_dictionary(context, status);
    }

    /* Parse number */
    else if (chr == '-' || chr == '+' || (chr >= '0' && chr <= '9')) {
        result = bramajson_parse_number(context, status);
    }

    /* Parse number */
    else if (chr == 't' || chr == 'f' || chr == 'n') {
        result = bramajson_parse_atom(context, status);
    }
        
    /* Parse issue */
    else
        *status = BRAMAJSON_JSON_NOT_VALID;

    return result;
}

bramajson_object* bramajson_parse_inner(char const* json_content, int32_t* status) {
    if (NULL == json_content || 0 == json_content[0]) {
        *status = BRAMAJSON_CONTENT_EMPTY;
        return NULL;
    }

    bramajson_context* context = (bramajson_context*)malloc( sizeof(bramajson_context));
    context->validation        = true;
    context->json_content      = json_content;
    context->content_length    = strlen(json_content);
    context->cursor_index      = 0;
    *status                    = BRAMAJSON_SUCCESS;

    bramajson_object*  object  = bramajson_parse_object(context, status);

    return object;
}

bramajson_object* bramajson_parse(char const* json_content, int32_t* status) {
    return bramajson_parse_inner(json_content, status);
}
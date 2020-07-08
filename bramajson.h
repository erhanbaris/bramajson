

#pragma once

#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
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

#define is_end()               (context->content_length <= context->cursor_index)
#define get_char()             (context->json_content[context->cursor_index])
#define get_next_char()        (context->content_length > (context->cursor_index + 1) ? context->json_content[context->cursor_index + 1] : 0)
#define increase_index()       do {++context->cursor_index; ++context->column_index;} while(0)
#define increase_newline()     do {++context->newline_index; context->column_index = 0;} while(0)
#define BRAMAJSON_MALLOC(size) bramajson_malloc(context, size)
#define CLEANUP_WHITESPACES()  while (!is_end() && (chr == '\r' || chr == '\n' || chr == ' ' || chr == '\t')) {\
        if (chr == '\n') \
            increase_newline();\
        increase_index();\
        chr = get_char();\
    }


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

typedef struct _bramajson_dictionary {
    size_t            size;
    size_t            count;
    bramajson_pair ** items;
} bramajson_dictionary;

typedef struct _bramajson_object {
    uint16_t type;
    union
    {
        int32_t                _integer;
        double                 _double;
        char*                  _string;
        bramajson_object*      _object;
        bramajson_dictionary * _dictionary;
        bramajson_array*       _array;
    };
} bramajson_object;


typedef struct _bramajson_context {
    bool        validation;
    size_t      content_length;
    size_t      cursor_index;
    size_t      newline_index;
    size_t      column_index;
    char const* json_content;

    /* Memory */
    char*       memory_block;
    size_t      memory_size;
    size_t      memory_count;
} bramajson_context;

/* Public apis */
bramajson_object* bramajson_parse   (char const* json_content,      int32_t* status);
void              bramajson_destroy (bramajson_object* json_object, int32_t* status);

/* Private */
bramajson_object* bramajson_parse_object(bramajson_context* context, int32_t* status);
bool              bramajson_get_text_block_info(bramajson_context* context, char opt, size_t* start, size_t* text_length);

/* Memory */

void* bramajson_malloc(bramajson_context* context, size_t size) {
    if (size + context->memory_count > context->memory_count) {
        context->memory_block = (char*)realloc(context->memory_block, context->memory_size * 2);
        context->memory_size = context->memory_size * 2;
    }

    context->memory_count += size;
    return (context->memory_block + (context->memory_count - size));
}

bramajson_object* bramajson_parse_array(bramajson_context* context, int32_t* status) {
    increase_index();

    bramajson_array* array   = (bramajson_array*)malloc(sizeof(bramajson_array));
    bramajson_object* result = (bramajson_object*)malloc(sizeof(bramajson_object));
    array->items             = (bramajson_object**)malloc(1 * sizeof(bramajson_object*));
    array->count             = 0;
    array->size              = 1;
    result->_array           = array;
    result->type             = BRAMAJSON_ARRAY;

    char32_t chr             = get_char();
    bool     end_found       = false;

    CLEANUP_WHITESPACES();

    do {
        if (chr == ']') {
            increase_index();
            chr = get_char();
            CLEANUP_WHITESPACES();
            end_found = true;
            break;
        }

        bramajson_object* item = bramajson_parse_object(context, status);
        chr = get_char();

        if (array->count == array->size) {
            array->size *= 2;
            array->items = (bramajson_object**)realloc(array->items, array->size * sizeof(bramajson_object*));
        }

        array->items[array->count++] = item;

        if (chr == ']') {
            increase_index();
            chr = get_char();
            CLEANUP_WHITESPACES();
            end_found = true;
            break;
        }
        else if (chr == ',') {
            increase_index();
            chr = get_char();
            CLEANUP_WHITESPACES();
        } else
            *status = BRAMAJSON_JSON_NOT_VALID;

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

    bramajson_dictionary * dict = (bramajson_dictionary*)malloc(sizeof(bramajson_dictionary));
    bramajson_object* result    = (bramajson_object*)malloc(sizeof(bramajson_object));
    dict->items                 = (bramajson_pair**)malloc(1 * sizeof(bramajson_pair*));
    dict->count                 = 0;
    dict->size                  = 1;
    result->_dictionary         = dict;
    result->type                = BRAMAJSON_DICT;

    char32_t chr                = get_char();
    bool     end_found          = false;

    CLEANUP_WHITESPACES();

    do {
        if (chr == '}') {
            increase_index();
            chr = get_char();
            CLEANUP_WHITESPACES();
            end_found = true;
            break;
        }

        CLEANUP_WHITESPACES();
        char opt = get_char();

        if (opt != '"' && opt != '\''){
            *status = BRAMAJSON_JSON_NOT_VALID;
            return NULL;
        }

        size_t key_length = 0;
        size_t key_start  = 0;

        if (bramajson_get_text_block_info(context, opt,  &key_start, &key_length) == false) {
            *status = BRAMAJSON_JSON_NOT_VALID;
            return NULL;
        }

        char* key = (char*)malloc(sizeof(char) * key_length + 1);
        strncpy(key, context->json_content + key_start, key_length);
        key[key_length] = '\0';
        chr = get_char();
        CLEANUP_WHITESPACES();

        chr = get_char();
        if (':' != chr || *status != BRAMAJSON_SUCCESS) {
            *status = BRAMAJSON_JSON_NOT_VALID;
            return NULL;
        }

        increase_index();
        bramajson_object* value = bramajson_parse_object(context, status);
        chr = get_char();

        if (dict->count == dict->size) {
            dict->size *= 2;
            dict->items = (bramajson_pair**)realloc(dict->items, dict->size * sizeof(bramajson_pair*));
        }

        bramajson_pair* pair = (bramajson_pair*)malloc(sizeof(bramajson_pair));
        pair->key                  = key;
        pair->object               = value;
        dict->items[dict->count++] = pair;

        if (chr == '}') {
            increase_index();
            chr = get_char();
            CLEANUP_WHITESPACES();
            end_found = true;
            break;
        }
        else if (chr == ',') {
            increase_index();
            chr = get_char();
            CLEANUP_WHITESPACES();
        } else
            *status = BRAMAJSON_JSON_NOT_VALID;
    }
    while(BRAMAJSON_SUCCESS == *status && !is_end() && chr != '}');

    if (false == end_found) {
        result  = NULL;
        *status = BRAMAJSON_JSON_NOT_VALID;
    }

    return result;
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

bool bramajson_get_text_block_info(bramajson_context* context, char opt, size_t* start, size_t* text_length) {
    increase_index();

    char chr      = get_char();
    char chr_next = get_next_char();
    size_t length = 0;
    *start        = context->cursor_index;

    if (chr == opt)
        increase_index();
    else
        while (!is_end() && chr != opt) {
            chr      = get_char();
            chr_next = get_next_char();

            if (chr == '\\' && chr_next == opt) {
                ++length;
                increase_index();
            }
            else if (chr == opt) {
                increase_index();
                break;
            }
            else
                ++length;

            increase_index();
        }

    if (chr != opt)
        return false;

    *text_length = length;
    return true;
}

bramajson_object* bramajson_parse_text(bramajson_context* context, int32_t* status, char opt) {
    size_t length = 0;
    size_t start  = 0;

    if (bramajson_get_text_block_info(context, opt, &start, &length) == false) {
        *status = BRAMAJSON_JSON_NOT_VALID;
        return NULL;
    }

    bramajson_object* token = (bramajson_object*)malloc(sizeof (bramajson_object));
    token->type    = BRAMAJSON_STRING;
    token->_string = (char*)malloc(sizeof(char) * length + 1);
    strncpy(token->_string, context->json_content + start, length);
    token->_string[length] = '\0';

    return token;
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
        token->type    = BRAMAJSON_FLOAT;
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
    char32_t chr             = get_char();
    bramajson_object* result = NULL;

    CLEANUP_WHITESPACES();

    /* Parse array */
    if (chr == '[') {
        result = bramajson_parse_array(context, status);
    }

    else if (chr == '"') {
        result = bramajson_parse_text(context, status, '"');
    }

    else if (chr == '\'') {
        result = bramajson_parse_text(context, status, '\'');
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

    if (*status == BRAMAJSON_SUCCESS) {
        chr = get_char();
        CLEANUP_WHITESPACES();
    }

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
    context->newline_index     = 1;
    context->column_index      = 0;

    /* Prepare memory block */
    context->memory_block      = (char*)malloc(sizeof(char) * 1024);
    context->memory_size       = 1024;
    context->memory_count      = 0;

    *status                    = BRAMAJSON_SUCCESS;

    bramajson_object*  object  = bramajson_parse_object(context, status);

    return object;
}

bramajson_object* bramajson_parse(char const* json_content, int32_t* status) {
    return bramajson_parse_inner(json_content, status);
}

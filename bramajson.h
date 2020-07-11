

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

#define is_end()               (*context->chr == (int)NULL)
#define get_next_char()        (!is_end() ? *(context->chr+1) : (int)NULL)
#define increase_index()       do {++context->chr;} while(0)
#define increase_newline()     do {++context->newline_index;} while(0)
#define valid_digit(c)         ((c) >= '0' && (c) <= '9')
#define BRAMAJSON_MALLOC(size) bramajson_malloc(context, size)
#define CLEANUP_WHITESPACES()  while (!is_end() && (*context->chr == '\r' || *context->chr == '\n' || *context->chr == ' ' || *context->chr == '\t')) increase_index();
#define get_char()             (context->chr)


typedef struct _bramajson_object  bramajson_object;
typedef struct _bramajson_pair    bramajson_pair;
typedef struct _bramajson_context bramajson_context;



typedef struct _bramajson_pair {
    char*             key;
    bramajson_object* object;
    bramajson_pair*   next;
} bramajson_pair;

typedef struct _bramajson_array {
    bramajson_object*  head;
    bramajson_object*  last;
} bramajson_array;

typedef struct _bramajson_dictionary {
    bramajson_pair*   head;
    bramajson_pair*   last;
} bramajson_dictionary;

typedef struct _bramajson_object {
    uint16_t            type;
    bramajson_object*   next;
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
    char const* json_content;

    /* Memory */
    char**      memory_blocks;
    size_t      memory_blocks_count;
    char*       memory_block;
    size_t      memory_size;
    size_t      memory_count;
    char const* chr;
} bramajson_context;

/* Public apis */
bramajson_object* bramajson_parse   (char const* json_content,      int32_t* status);
void              bramajson_destroy (bramajson_object* json_object, int32_t* status);

/* Private */
bramajson_object* bramajson_parse_object(bramajson_context* context, int32_t* status);
bool              bramajson_get_text_block_info(bramajson_context* context, char opt, size_t* start, size_t* text_length);

/* Memory */

/* Memory */
void* bramajson_malloc(bramajson_context* context, size_t size) {
    if (size + context->memory_count > context->memory_size) {
        ++context->memory_blocks_count;
        context->memory_blocks       = (char**)realloc(context->memory_blocks, sizeof(char*) * context->memory_blocks_count);
        context->memory_block        = (char*)malloc(sizeof(char) * context->memory_size);
        context->memory_count        = 0;

        /* Save new memory block container */
        context->memory_blocks[context->memory_blocks_count - 1] = context->memory_block;
    }

    context->memory_count += size;
    return (context->memory_block + (context->memory_count - size));
}

bramajson_object* bramajson_parse_array(bramajson_context* context, int32_t* status) {
    increase_index();

    bramajson_array* array   = (bramajson_array*)BRAMAJSON_MALLOC(sizeof(bramajson_array));
    bramajson_object* result = (bramajson_object*)BRAMAJSON_MALLOC(sizeof(bramajson_object));
    array->head              = NULL;
    array->last              = NULL;
    result->_array           = array;
    result->type             = BRAMAJSON_ARRAY;

    char chr                 = *get_char();
    bool     end_found       = false;

    CLEANUP_WHITESPACES();

    do {
        if (chr == ']') {
            increase_index();
            chr = *get_char();
            CLEANUP_WHITESPACES();
            end_found = true;
            break;
        }

        bramajson_object* item = bramajson_parse_object(context, status);

        if (BRAMAJSON_SUCCESS != *status) return NULL;

        chr = *get_char();

        if (NULL == array->head) {
            array->head = item;
            array->last = item;
        } else {
            array->last->next = item;
            array->last = item;
        }

        if (chr == ']') {
            increase_index();
            chr = *get_char();
            CLEANUP_WHITESPACES();
            end_found = true;
            break;
        }
        else if (chr == ',') {
            increase_index();
            chr = *get_char();
            CLEANUP_WHITESPACES();
        } else {
            *status = BRAMAJSON_JSON_NOT_VALID;
            return NULL;
        }

    }
    while(!is_end());

    if (false == end_found) {
        result  = NULL;
        *status = BRAMAJSON_JSON_NOT_VALID;
    }

    return result;
}

bramajson_object* bramajson_parse_dictionary(bramajson_context* context, int32_t* status) {
    increase_index();

    bramajson_dictionary * dict = (bramajson_dictionary*)BRAMAJSON_MALLOC(sizeof(bramajson_dictionary));
    bramajson_object* result    = (bramajson_object*)BRAMAJSON_MALLOC(sizeof(bramajson_object));
    dict->head                  = NULL;
    dict->last                  = NULL;
    result->_dictionary         = dict;
    result->type                = BRAMAJSON_DICT;

    char chr                    = *get_char();
    bool     end_found          = false;

    CLEANUP_WHITESPACES();

    do {
        if (chr == '}') {
            increase_index();
            chr = *get_char();
            CLEANUP_WHITESPACES();
            end_found = true;
            break;
        }

        CLEANUP_WHITESPACES();
        char opt = *get_char();

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

        char* key = (char*)BRAMAJSON_MALLOC(sizeof(char) * key_length + 1);
        strncpy(key, context->json_content + key_start, key_length);
        key[key_length] = '\0';
        chr = *get_char();
        CLEANUP_WHITESPACES();

        chr = *get_char();
        if (':' != chr || *status != BRAMAJSON_SUCCESS) {
            *status = BRAMAJSON_JSON_NOT_VALID;
            return NULL;
        }

        increase_index();
        bramajson_object* value = bramajson_parse_object(context, status);

        if (BRAMAJSON_SUCCESS != *status) return NULL;

        chr = *get_char();

        bramajson_pair* pair = (bramajson_pair*)BRAMAJSON_MALLOC(sizeof(bramajson_pair));
        pair->key    = key;
        pair->object = value;

        if (NULL == dict->head) {
            dict->head = pair;
            dict->last = pair;
        } else {
            dict->last->next = pair;
            dict->last = pair;
        }

        if (chr == '}') {
            increase_index();
            chr = *get_char();
            CLEANUP_WHITESPACES();
            end_found = true;
            break;
        }

        if (chr == ',') {
            increase_index();
            chr = *get_char();
            CLEANUP_WHITESPACES();
        } else {
            *status = BRAMAJSON_JSON_NOT_VALID;
            return NULL;
        }
    }
    while(!is_end());

    if (false == end_found) {
        result  = NULL;
        *status = BRAMAJSON_JSON_NOT_VALID;
    }

    return result;
}

bramajson_object* bramajson_parse_atom(bramajson_context* context, int32_t* status) {
    bramajson_object* object = NULL;
    uint16_t type            = BRAMAJSON_NULL;

    if (0 == strncmp("false", context->chr, 5)) {
        type = BRAMAJSON_FALSE;
        context->chr +=5;
    }
    else if (0 == strncmp("true", context->chr, 4)) {
        type = BRAMAJSON_TRUE;
        context->chr +=4;
    }
    else if (0 == strncmp("null", context->chr, 4)) {
        type = BRAMAJSON_NULL;
        context->chr +=4;
    } else
        *status = BRAMAJSON_JSON_NOT_VALID;

    if (BRAMAJSON_SUCCESS == *status) {
        object       = (bramajson_object*)BRAMAJSON_MALLOC(sizeof(bramajson_object));
        object->type = type;
    }

    return object;
}

bool bramajson_get_text_block_info(bramajson_context* context, char opt, size_t* start, size_t* text_length) {
    increase_index();
    *start  = get_char() - context->json_content;

    if (*get_char() != opt)
        while (!is_end() && (*get_char() != opt || (*get_char() == '\\' &&  get_next_char() == opt)))
            increase_index();

    if (*get_char() != opt)
        return false;

    *text_length = (get_char() - context->json_content) - *start;
    increase_index();
    return true;
}

bramajson_object* bramajson_parse_text(bramajson_context* context, int32_t* status, char opt) {
    size_t length = 0;
    size_t start  = 0;

    if (bramajson_get_text_block_info(context, opt, &start, &length) == false) {
        *status = BRAMAJSON_JSON_NOT_VALID;
        return NULL;
    }

    bramajson_object* token = (bramajson_object*)BRAMAJSON_MALLOC(sizeof (bramajson_object));
    token->type    = BRAMAJSON_STRING;
    token->_string = (char*)BRAMAJSON_MALLOC(sizeof(char) * length + 1);
    strncpy(token->_string, context->json_content + start, length);
    token->_string[length] = '\0';

    return token;
}

bramajson_object* bramajson_parse_number(bramajson_context* context, int32_t* status) {
    bool is_double = false;

    int frac;
    double sign, value, scale;

    // Get sign, if any.

    sign = 1.0;
    if (*get_char() == '-') {
        sign = -1.0;
        increase_index();

    } else if (*get_char() == '+') {
        increase_index();
    }

    // Get digits before decimal point or exponent, if any.

    for (value = 0.0; valid_digit(*get_char()); get_char() += 1) {
        value = value * 10.0 + (*get_char() - '0');
    }

    // Get digits after decimal point, if any.

    if (*get_char() == '.') {
        is_double = true;
        double pow10 = 10.0;
        increase_index();
        while (valid_digit(*get_char())) {
            value += (*get_char() - '0') / pow10;
            pow10 *= 10.0;
            increase_index();
        }
    }

    frac = 0;
    scale = 1.0;
    if ((*get_char() == 'e') || (*get_char() == 'E')) {
        unsigned int expon;

        // Get sign of exponent, if any.

        increase_index();
        if (*get_char() == '-') {
            frac = 1;
            increase_index();

        } else if (*get_char() == '+') {
            increase_index();
        }

        // Get digits of exponent, if any.

        for (expon = 0; valid_digit(*get_char()); get_char() += 1) {
            expon = expon * 10 + (*get_char() - '0');
        }
        if (expon > 308) expon = 308;

        // Calculate scaling factor.

        while (expon >= 50) { scale *= 1E50; expon -= 50; }
        while (expon >=  8) { scale *= 1E8;  expon -=  8; }
        while (expon >   0) { scale *= 10.0; expon -=  1; }
    }

    // Return signed and scaled floating point result.

    bramajson_object* object = (bramajson_object*)BRAMAJSON_MALLOC(sizeof (bramajson_object));
    object->next = NULL;

    if (!is_double) {
        object->type     = BRAMAJSON_INT;
        object->_integer = sign * (frac ? (value / scale) : (value * scale));
    } else {
        object->type    = BRAMAJSON_FLOAT;
        object->_double = sign * (frac ? (value / scale) : (value * scale));
    }

    return object;
}

bramajson_object* bramajson_parse_object(bramajson_context* context, int32_t* status) {
    char chr             = *get_char();
    bramajson_object* result = NULL;

    CLEANUP_WHITESPACES();
    chr             = *get_char();

    switch (chr) {
        case '[':
            result = bramajson_parse_array(context, status);
            break;

        case '\'':
        case '"':
            result = bramajson_parse_text(context, status, chr);
            break;

        case '{':
            result = bramajson_parse_dictionary(context, status);
            break;

        case '-':
        case '+':
        case '0':
        case '1':
        case '2':
        case '3':
        case '4':
        case '5':
        case '6':
        case '7':
        case '8':
        case '9':
            result = bramajson_parse_number(context, status);
            break;

        case 't':
        case 'f':
        case 'n':
            result = bramajson_parse_atom(context, status);
            break;

        default:
            *status = BRAMAJSON_JSON_NOT_VALID;
    }

    chr = *get_char();
    CLEANUP_WHITESPACES();

    return result;
}

bramajson_object* bramajson_parse_inner(char const* json_content, int32_t* status) {
    if (NULL == json_content || 0 == json_content[0]) {
        *status = BRAMAJSON_CONTENT_EMPTY;
        return NULL;
    }

    bramajson_context* context = (bramajson_context*)malloc( sizeof(bramajson_context));
    context->json_content      = json_content;
    context->chr               = json_content;
    context->content_length    = strlen(json_content);

    /* Prepare memory block */
    context->memory_blocks       = (char**)malloc(sizeof(char*));
    context->memory_blocks_count = 1;

    context->memory_size         = context->content_length > 256 ? context->content_length : 256;
    context->memory_block        = (char*)malloc(sizeof(char) * context->memory_size);
    context->memory_count        = 0;

    context->memory_blocks[0]    = context->memory_block;

    *status                     = BRAMAJSON_SUCCESS;
    bramajson_object*  object   = bramajson_parse_object(context, status);

    return object;
}

bramajson_object* bramajson_parse(char const* json_content, int32_t* status) {
    return bramajson_parse_inner(json_content, status);
}

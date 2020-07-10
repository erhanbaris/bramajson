

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


#define BRAMAJSON_POST_ACTION_ADD    (1 << 0)
#define BRAMAJSON_POST_ACTION_REMOVE (1 << 1)
#define BRAMAJSON_POST_ACTION_IDLE   (1 << 2)

#define is_end()               (*chr != (int)NULL)
#define get_next_char()        (is_end() ? *(chr+1) : 0)
#define increase_index()       do {++chr;} while(0)
#define increase_newline()     do {++context->newline_index;} while(0)
#define valid_digit(c)         ((c) >= '0' && (c) <= '9')
#define BRAMAJSON_MALLOC(size) bramajson_malloc(context, size)
#define CLEANUP_WHITESPACES()  while (!is_end() && (*chr == '\r' || *chr == '\n' || *chr == ' ' || *chr == '\t')) increase_index();


typedef struct _bramajson_object  bramajson_object;
typedef struct _bramajson_pair    bramajson_pair;
typedef struct _bramajson_context bramajson_context;


typedef struct _bramajson_pair {
    char*             key;
    bramajson_object* object;
    bramajson_pair*   next;
} bramajson_pair;

typedef struct _bramajson_array {
    bool               closed;
    bramajson_object*  head;
    bramajson_object*  last;
} bramajson_array;

typedef struct _bramajson_dictionary {
    bool              closed;
    bramajson_pair*   head;
    bramajson_pair*   last;
    bramajson_pair*   last_pair;
} bramajson_dictionary;

typedef struct _bramajson_object {
    uint16_t          type;
    bramajson_object* parent;
    bramajson_object* next;
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
    size_t      content_length;
    size_t      newline_index;
    char const* json_content;

    /* Memory */
    char**      memory_blocks;
    size_t      memory_blocks_count;
    char*       memory_block;
    size_t      memory_size;
    size_t      memory_count;
} bramajson_context;

/* Public apis */
bramajson_object* bramajson_parse   (char const* json_content,      int32_t* status);
void              bramajson_destroy (bramajson_object* json_object, int32_t* status);

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

double naive (const char *p)
{
    int frac;
    double sign, value, scale;

 
    // Get sign, if any.

    sign = 1.0;
    if (*p == '-') {
        sign = -1.0;
        p += 1;

    } else if (*p == '+') {
        p += 1;
    }

    // Get digits before decimal point or exponent, if any.

    for (value = 0.0; valid_digit(*p); p += 1) {
        value = value * 10.0 + (*p - '0');
    }

    // Get digits after decimal point, if any.

    if (*p == '.') {
        double pow10 = 10.0;
        p += 1;
        while (valid_digit(*p)) {
            value += (*p - '0') / pow10;
            pow10 *= 10.0;
            p += 1;
        }
    }

    // Handle exponent, if any.

    frac = 0;
    scale = 1.0;
    if ((*p == 'e') || (*p == 'E')) {
        unsigned int expon;

        // Get sign of exponent, if any.

        p += 1;
        if (*p == '-') {
            frac = 1;
            p += 1;

        } else if (*p == '+') {
            p += 1;
        }

        // Get digits of exponent, if any.

        for (expon = 0; valid_digit(*p); p += 1) {
            expon = expon * 10 + (*p - '0');
        }
        if (expon > 308) expon = 308;

        // Calculate scaling factor.

        while (expon >= 50) { scale *= 1E50; expon -= 50; }
        while (expon >=  8) { scale *= 1E8;  expon -=  8; }
        while (expon >   0) { scale *= 10.0; expon -=  1; }
    }

    // Return signed and scaled floating point result.

    return sign * (frac ? (value / scale) : (value * scale));
}

bramajson_object* bramajson_parse_inner(char const* json_content, int32_t* status) {
    if (NULL == json_content || 0 == json_content[0]) {
        *status = BRAMAJSON_CONTENT_EMPTY;
        return NULL;
    }

    bramajson_context* context = (bramajson_context*)malloc( sizeof(bramajson_context));
    context->json_content      = json_content;
    context->content_length    = strlen(json_content);
    context->newline_index     = 1;

    /* Prepare memory block */
    context->memory_blocks       = (char**)malloc(sizeof(char*));
    context->memory_blocks_count = 1;

    context->memory_size         = context->content_length > 256 ? context->content_length : 256;
    context->memory_block        = (char*)malloc(sizeof(char) * context->memory_size);
    context->memory_count        = 0;

    context->memory_blocks[0]    = context->memory_block;

    *status                    = BRAMAJSON_SUCCESS;

    bramajson_object** stack   = (bramajson_object**)malloc(1 * sizeof(bramajson_object*));
    size_t stack_index         = 0;
    size_t stack_size          = 1;

    char const * chr           = json_content;
    bramajson_object* last_object = NULL;

    /* String variables */
    size_t string_length  = 0;
    size_t string_start   = 0;
    bool string_in_buffer = false;
    
    bool deliminator_used    = false;
    bramajson_object* object = NULL;

    while(!is_end()) {
        int16_t post_action      = BRAMAJSON_POST_ACTION_IDLE;
        object                   = NULL;
        CLEANUP_WHITESPACES();

        switch (*chr)
        {
            /* Start array */
            case '[': {
                increase_index();

                object                   = (bramajson_object*)BRAMAJSON_MALLOC(sizeof(bramajson_object));
                bramajson_array* array   = (bramajson_array*) BRAMAJSON_MALLOC(sizeof(bramajson_array));
                array->head              = NULL;
                array->last              = NULL;
                array->closed            = false;
                object->next             = NULL;
                object->_array           = array;
                object->type             = BRAMAJSON_ARRAY;
                break;
            }

            /* Finish array */
            case ']': {
                if (NULL            == last_object || 
                    BRAMAJSON_ARRAY != last_object->type ||
                    true            == deliminator_used) {
                    goto brama_error;
                }

                last_object->_array->closed = true;
                increase_index();
                post_action = BRAMAJSON_POST_ACTION_REMOVE;
                break;
            }

            /* Deliminator */
            case ',': {
                if (true == deliminator_used ||
                    NULL == last_object ||
                    (BRAMAJSON_ARRAY != last_object->type && BRAMAJSON_DICT != last_object->type) ||
                    (BRAMAJSON_ARRAY == last_object->type && NULL == last_object->_array->head) ||
                    (BRAMAJSON_DICT  == last_object->type && NULL == last_object->_dictionary->head) ||
                    (BRAMAJSON_DICT  == last_object->type && NULL != last_object->_dictionary->last_pair)) {
                    goto brama_error;
                }

                deliminator_used = true;
                increase_index();
                break;
            }

            /* Deliminator */
            case ':': {
                if (true == deliminator_used ||
                    NULL == last_object ||
                    BRAMAJSON_DICT != last_object->type) {
                    goto brama_error;
                }

                deliminator_used = true;
                increase_index();
                break;
            }

            /* Parse string */
            case '"':
            case'\'': {
                increase_index();

                char32_t opt  = *chr;
                char chr_next = get_next_char();
                string_start  = chr - context->json_content;

                if (*chr == opt)
                    increase_index();
                else
                    while (!is_end()) {
                        chr_next = get_next_char();

                        if (*chr == '\\' && chr_next == opt) {
                            increase_index();
                        }
                        else if (*chr == opt) {
                            increase_index();
                            break;
                        }

                        increase_index();
                    }

                if (*chr != opt) {
                    goto brama_error;
                }

                string_length = (chr - context->json_content) - string_start;
                string_in_buffer = true;
                break;
            }

            /* Parse dictionary */
            case '{': {
                increase_index();

                bramajson_dictionary * dict = (bramajson_dictionary*)BRAMAJSON_MALLOC(sizeof(bramajson_dictionary));
                dict->head                  = NULL;
                dict->last                  = NULL;
                dict->last_pair             = NULL;
                dict->closed                = false;


                object                      = (bramajson_object*)BRAMAJSON_MALLOC(sizeof(bramajson_object));
                object->next                = NULL;
                object->_dictionary         = dict;
                object->type                = BRAMAJSON_DICT;
                break;
            }

            case '}': {
                if (NULL            == last_object ||
                    BRAMAJSON_DICT  != last_object->type ||
                    true            == deliminator_used ||
                    (NULL != last_object->_dictionary->last_pair && NULL == last_object->_dictionary->last_pair->object)) {
                    goto brama_error;
                }

                increase_index();
                last_object->_dictionary->closed = true;
                post_action = BRAMAJSON_POST_ACTION_REMOVE;
                break;
            }

            /* Parse number */
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
            case '9': {
                size_t index             = 0;
                bool isMinus             = false;
                int dotPlace             = 0;
                size_t start             = chr - context->json_content;
                bool isDouble            = false;
                char32_t chNext          = get_next_char();
                bool e_used              = false;
                size_t e_after           = 0;
                bool plus_used           = false;

                while (!is_end()) {
                    if (*chr == '-') {
                        if (isMinus && !e_used) {
                            goto brama_error;
                        }

                        isMinus = true;
                    }

                    else if (*chr == '+') {
                        if (plus_used && !e_used){
                            goto brama_error;
                        }

                        plus_used = true;
                    }

                    else if (index != 0 && (*chr == 'e' || *chr == 'E')) {
                        e_used = true;
                    }

                    else if (*chr == '.') {
                        if (isDouble) {
                            goto brama_error;
                        }

                        isDouble = true;
                    }

                    else if (!e_used && (*chr >= '0' && *chr <= '9')) {
                        if (isDouble)
                            ++dotPlace;
                    }

                    else if (e_used && (*chr >= '0' && *chr <= '9')) {

                        }
                    else {
                        break;
                    }
                    
                    increase_index();
                    chNext = get_next_char();
                    ++index;
                }

                char* tmp = (char*)BRAMAJSON_MALLOC(sizeof (char) * index + 1);
                strncpy(tmp, context->json_content + start, index);
                tmp[index] = '\0';
                
                object = (bramajson_object*)BRAMAJSON_MALLOC(sizeof (bramajson_object));
                object->next = NULL;
 
                if (!isDouble) {
                    object->type     = BRAMAJSON_INT;
                    object->_integer = naive(tmp);
                } else {
                    object->type    = BRAMAJSON_FLOAT;
                    object->_double = naive(tmp);
                }
                break;
            }

            /* Parse atom */
            case 't':
            case 'f':
            case 'n': {
                uint16_t type = BRAMAJSON_NULL;

                if (0 == strncmp("false", (chr - context->json_content) + context->json_content, 5)) {
                    type = BRAMAJSON_FALSE;
                    chr +=5;
                }
                else if (0 == strncmp("true", (chr - context->json_content) + context->json_content, 4)) {
                    type = BRAMAJSON_TRUE;
                    chr +=4;
                }
                else if (0 == strncmp("null", (chr - context->json_content) + context->json_content, 4)) {
                    type = BRAMAJSON_NULL;
                    chr +=4;
                } else {
                    goto brama_error;
                }

                object         = (bramajson_object*)BRAMAJSON_MALLOC(sizeof(bramajson_object));
                object->next   = NULL;
                object->type   = type;
                break;
            }

            case '\0': {
                break;
            }

            /* Parse issue */
            default: {
                goto brama_error;
            }
        }

        /* Check upper object is container */
        if (NULL != last_object) {

            /* String in fly */
            if (true == string_in_buffer) {
                if(BRAMAJSON_ARRAY == last_object->type) {
                    object          = (bramajson_object*)BRAMAJSON_MALLOC(sizeof (bramajson_object));
                    object->next    = NULL;
                    object->type    = BRAMAJSON_STRING;
                    object->_string = (char*)BRAMAJSON_MALLOC(sizeof(char) * string_length + 1);
                    strncpy(object->_string, context->json_content + string_start, string_length);
                    object->_string[string_length] = '\0';

                    string_start     = 0;
                    string_length    = 0;
                    string_in_buffer = false;
                }
                else if (BRAMAJSON_DICT == last_object->type) {
                    if (NULL == last_object->_dictionary->last_pair) {
                        last_object->_dictionary->last_pair         = (bramajson_pair*)BRAMAJSON_MALLOC(sizeof(bramajson_pair));
                        last_object->_dictionary->last_pair->next   = NULL;
                        last_object->_dictionary->last_pair->key    = (char*)BRAMAJSON_MALLOC(sizeof(char) * string_length + 1);
                        last_object->_dictionary->last_pair->object = NULL;
                        strncpy(last_object->_dictionary->last_pair->key, context->json_content + string_start, string_length);
                        last_object->_dictionary->last_pair->key[string_length] = '\0';

                        string_start     = 0;
                        string_length    = 0;
                        string_in_buffer = false;
                    } else {
                        object          = (bramajson_object*)BRAMAJSON_MALLOC(sizeof (bramajson_object));
                        object->next    = NULL;
                        object->type    = BRAMAJSON_STRING;
                        object->_string = (char*)BRAMAJSON_MALLOC(sizeof(char) * string_length + 1);
                        strncpy(object->_string, context->json_content + string_start, string_length);
                        object->_string[string_length] = '\0';

                        string_start     = 0;
                        string_length    = 0;
                        string_in_buffer = false;
                    }
                } else {
                    goto brama_error;
                }

                deliminator_used = false;
            }

            /* Object created and can be added to container */
            if (NULL != object) {
                switch (last_object->type) {
                
                    /* Upper object is array*/
                    case BRAMAJSON_ARRAY: {
                        if (NULL == last_object->_array->head) {
                            last_object->_array->head = object;
                            last_object->_array->last = object;
                        } else {
                            last_object->_array->last->next = object;
                            last_object->_array->last       = object;
                        }

                        object->parent   = last_object;
                        deliminator_used = false;
                        break;
                    }

                    case BRAMAJSON_DICT: {
                        if (NULL == last_object->_dictionary->head) {
                            last_object->_dictionary->head = last_object->_dictionary->last_pair;
                            last_object->_dictionary->last = last_object->_dictionary->last_pair;
                        } else {
                            last_object->_dictionary->last->next = last_object->_dictionary->last_pair;
                            last_object->_dictionary->last       = last_object->_dictionary->last_pair;
                        }

                        last_object->_dictionary->last_pair->object = object;

                        object->parent                                           = last_object;
                        last_object->_dictionary->last_pair                      = NULL;
                        deliminator_used                                         = false;
                        break;
                    }

                    default:
                        break;
                }
            }
        }

        /* Post actions after parsin code */
        switch (post_action) {
            case BRAMAJSON_POST_ACTION_ADD: {
                if (stack_index == stack_size) {
                    stack_size *= 2;
                    stack = (bramajson_object**)realloc(stack, stack_size * sizeof(bramajson_object*));
                }

                stack[stack_index++] = object;
                last_object          = object;
                break;
            }

            case BRAMAJSON_POST_ACTION_REMOVE: {

                if ((BRAMAJSON_ARRAY == last_object->type && false == last_object->_array->closed) ||
                    (BRAMAJSON_DICT  == last_object->type && false == last_object->_dictionary->closed)) {
                    goto brama_error;
                }

                --stack_index;
                last_object = stack[stack_index - 1];
                break;
            }
        }
    }

    if (NULL != object && ((BRAMAJSON_ARRAY == object->type && false == object->_array->closed) ||
        (BRAMAJSON_DICT  == object->type && false == object->_dictionary->closed))) {
        goto brama_error;
    }

    if (NULL == stack[0] && NULL != object)
        return object;
    else
        return stack[0];

brama_error:
    *status = BRAMAJSON_JSON_NOT_VALID;
    return NULL;
}

bramajson_object* bramajson_parse(char const* json_content, int32_t* status) {
    return bramajson_parse_inner(json_content, status);
}

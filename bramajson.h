#ifndef bramajson_h
#define bramajson_h

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

#define is_end()               (*chr == (int)NULL)
#define get_next_char()        (!is_end() ? *(chr+1) : (int)NULL)
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
    bramajson_object*  head;
    bramajson_object*  last;
    bool               closed;
} bramajson_array;

typedef struct _bramajson_dictionary {
    bramajson_pair*   head;
    bramajson_pair*   last;
    bramajson_pair*   last_pair;
    bool              closed;
} bramajson_dictionary;

typedef struct _bramajson_object {
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
    uint16_t          type;
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
                post_action              = BRAMAJSON_POST_ACTION_ADD;
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
                char32_t opt  = *chr;
                increase_index();
                string_start  = chr - context->json_content;

                if (*chr != opt)
                    while (!is_end() && (*chr != opt || (*chr == '\\' &&  get_next_char() == opt)))
                        increase_index();

                if (*chr != opt) {
                    goto brama_error;
                }

                string_length = (chr - context->json_content) - string_start;
                increase_index();

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
                post_action                 = BRAMAJSON_POST_ACTION_ADD;
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
                bool is_double = false;

                int frac;
                double sign, value, scale;

                // Get sign, if any.

                sign = 1.0;
                if (*chr == '-') {
                    sign = -1.0;
                    increase_index();

                } else if (*chr == '+') {
                    increase_index();
                }

                // Get digits before decimal point or exponent, if any.

                for (value = 0.0; valid_digit(*chr); chr += 1) {
                    value = value * 10.0 + (*chr - '0');
                }

                // Get digits after decimal point, if any.

                if (*chr == '.') {
                    is_double = true;
                    double pow10 = 10.0;
                    increase_index();
                    while (valid_digit(*chr)) {
                        value += (*chr - '0') / pow10;
                        pow10 *= 10.0;
                        increase_index();
                    }
                }

                frac = 0;
                scale = 1.0;
                if ((*chr == 'e') || (*chr == 'E')) {
                    unsigned int expon;

                    // Get sign of exponent, if any.

                    increase_index();
                    if (*chr == '-') {
                        frac = 1;
                        increase_index();

                    } else if (*chr == '+') {
                        increase_index();
                    }

                    // Get digits of exponent, if any.

                    for (expon = 0; valid_digit(*chr); chr += 1) {
                        expon = expon * 10 + (*chr - '0');
                    }
                    if (expon > 308) expon = 308;

                    // Calculate scaling factor.

                    while (expon >= 50) { scale *= 1E50; expon -= 50; }
                    while (expon >=  8) { scale *= 1E8;  expon -=  8; }
                    while (expon >   0) { scale *= 10.0; expon -=  1; }
                }

                // Return signed and scaled floating point result.

                object = (bramajson_object*)BRAMAJSON_MALLOC(sizeof (bramajson_object));
                object->next = NULL;

                if (!is_double) {
                    object->type     = BRAMAJSON_INT;
                    object->_integer = sign * (frac ? (value / scale) : (value * scale));
                } else {
                    object->type    = BRAMAJSON_FLOAT;
                    object->_double = sign * (frac ? (value / scale) : (value * scale));
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

#endif

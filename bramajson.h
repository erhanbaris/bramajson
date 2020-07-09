

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
    bool               closed;
    size_t             size;
    size_t             count;
    bramajson_object** items;
} bramajson_array;

typedef struct _bramajson_dictionary {
    bool              closed;
    size_t            size;
    size_t            count;
    bramajson_pair**  items;
    bramajson_pair*   last_pair;
} bramajson_dictionary;

typedef struct _bramajson_object {
    uint16_t          type;
    bramajson_object* parent;
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

    bramajson_object** stack   = (bramajson_object**)malloc(1 * sizeof(bramajson_object*));
    size_t stack_index         = 0;
    size_t stack_size          = 1;

    char32_t chr                  = '\0';
    bramajson_object* last_object = NULL;

    /* String variables */
    size_t string_length  = 0;
    size_t string_start   = 0;
    bool string_in_buffer = false;
    
    bool deliminator_used    = false;
    bramajson_object* object = NULL;

    while(!is_end() && BRAMAJSON_SUCCESS == *status) {
        int16_t post_action      = BRAMAJSON_POST_ACTION_IDLE;
        chr                      = get_char();
        object                   = NULL;
        CLEANUP_WHITESPACES();

        switch (chr)
        {
            /* Start array */
            case '[': {
                increase_index();

                object                   = (bramajson_object*)malloc(sizeof(bramajson_object));
                bramajson_array* array   = (bramajson_array*)malloc(sizeof(bramajson_array));
                array->items             = (bramajson_object**)malloc(1 * sizeof(bramajson_object*));
                array->count             = 0;
                array->size              = 1;
                array->closed            = false;
                object->_array           = array;
                object->type             = BRAMAJSON_ARRAY;
                bool     end_found       = false;

                /* Array parse not finished yet */
                if (chr != ']')
                    post_action = BRAMAJSON_POST_ACTION_ADD;
                else {
                    array->closed = true;
                    increase_index();
                }

                break;
            }

            /* Finish array */
            case ']': {
                if (NULL            == last_object || 
                    BRAMAJSON_ARRAY != last_object->type ||
                    true            == deliminator_used) {
                    *status = BRAMAJSON_JSON_NOT_VALID;
                    break;
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
                    (BRAMAJSON_ARRAY == last_object->type && 0    == last_object->_array->count) ||
                    (BRAMAJSON_DICT  == last_object->type && 0    == last_object->_dictionary->count) ||
                    (BRAMAJSON_DICT  == last_object->type && NULL != last_object->_dictionary->last_pair)) {
                    *status = BRAMAJSON_JSON_NOT_VALID;
                    break;
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
                    *status = BRAMAJSON_JSON_NOT_VALID;
                    break;
                }

                deliminator_used = true;
                increase_index();
                break;
            }

            /* Parse string */
            case '"':
            case'\'': {
                increase_index();
                CLEANUP_WHITESPACES();

                char opt      = chr;
                chr           = get_char();
                char chr_next = get_next_char();
                string_start  = context->cursor_index;
                string_length = 0;

                if (chr == opt)
                    increase_index();
                else
                    while (!is_end() && chr != opt) {
                        chr      = get_char();
                        chr_next = get_next_char();

                        if (chr == '\\' && chr_next == opt) {
                            ++string_length;
                            increase_index();
                        }
                        else if (chr == opt) {
                            increase_index();
                            break;
                        }
                        else
                            ++string_length;

                        increase_index();
                    }

                if (chr != opt) {
                    *status = BRAMAJSON_JSON_NOT_VALID;
                    break;
                }

                string_in_buffer = true;
                break;
            }

            /* Parse dictionary */
            case '{': {
                increase_index();

                bramajson_dictionary * dict = (bramajson_dictionary*)malloc(sizeof(bramajson_dictionary));
                dict->items                 = (bramajson_pair**)malloc(1 * sizeof(bramajson_pair*));
                dict->last_pair             = NULL;
                dict->count                 = 0;
                dict->size                  = 1;
                dict->closed                = false;


                object                      = (bramajson_object*)malloc(sizeof(bramajson_object));
                object->_dictionary         = dict;
                object->type                = BRAMAJSON_DICT;
                chr                         = get_char();

                CLEANUP_WHITESPACES();

                /* Dictionary parse not finished yet */
                if (chr != '}')
                    post_action = BRAMAJSON_POST_ACTION_ADD;
                else {
                    dict->closed = true;
                    increase_index();
                }
                break;
            }

            case '}': {
                if (NULL            == last_object ||
                    BRAMAJSON_DICT  != last_object->type ||
                    true            == deliminator_used ||
                    (NULL != last_object->_dictionary->last_pair && NULL == last_object->_dictionary->last_pair->object)) {
                    *status = BRAMAJSON_JSON_NOT_VALID;
                    break;
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
                double beforeTheComma    = 0;
                double afterTheComma     = 0;
                size_t start             = context->cursor_index;
                bool isDouble            = false;
                char32_t chNext          = get_next_char();
                bool e_used              = false;
                size_t e_after           = 0;
                bool plus_used           = false;

                while (!is_end()) {
                    if (chr == '-') {
                        if ((isMinus || (beforeTheComma > 0 || afterTheComma > 0)) && !e_used){
                            *status = BRAMAJSON_JSON_NOT_VALID;
                            break;
                        }

                        isMinus = true;
                    }

                    else if (chr == '+') {
                        if ((plus_used || (beforeTheComma > 0 || afterTheComma > 0)) && !e_used){
                            *status = BRAMAJSON_JSON_NOT_VALID;
                            break;
                        }

                        plus_used = true;
                    }

                    else if (index != 0 && (chr == 'e' || chr == 'E')) {
                        e_used = true;
                    }

                    else if (chr == '.') {
                        if (isDouble) {
                            *status = BRAMAJSON_JSON_NOT_VALID;
                            break;
                        }

                        isDouble = true;
                    }

                    else if (!e_used && (chr >= '0' && chr <= '9')) {
                        if (isDouble) {
                            ++dotPlace;

                            afterTheComma *= (int)pow(10, 1);
                            afterTheComma += chr - '0';
                        }
                        else {
                            beforeTheComma *= (int)pow(10, 1);
                            beforeTheComma += chr - '0';
                        }
                    }

                    else if (e_used && (chr >= '0' && chr <= '9')) {
                        e_after *= (int)pow(10, 1);
                        e_after += chr - '0';
                    }
                    else {
                        break;
                    }
                    
                    increase_index();
                    chr    = get_char();
                    chNext = get_next_char();
                    ++index;
                }
                
                object = (bramajson_object*)malloc(sizeof (bramajson_object));
 
                if (!isDouble) {
                    object->type     = BRAMAJSON_INT;
                    object->_integer = beforeTheComma;
                } else {
                    object->type    = BRAMAJSON_FLOAT;
                    object->_double = (beforeTheComma + (afterTheComma * pow(10, -1 * dotPlace)));
                }

                if (e_used) {
                    if (isMinus) {
                        object->_double = object->_double / (double)pow((double)10, (double)e_after);
                    } else {
                        object->_double = object->_double * (double)pow((double)10, (double)e_after);
                    }
                }

                if (isMinus && !e_used)
                    object->_double *= -1;
                break;
            }

            /* Parse atom */
            case 't':
            case 'f':
            case 'n': {
                uint16_t type = BRAMAJSON_NULL;

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
                    object         = (bramajson_object*)malloc(sizeof(bramajson_object));
                    object->type   = type;
                }

                break;
            }

            case '\0': {
                break;
            }

            /* Parse issue */
            default: {
                *status = BRAMAJSON_JSON_NOT_VALID;
                break;
            }
        }

        /* Sometings went wrong, stop parser */
        if (BRAMAJSON_SUCCESS != *status) break;

        /* Check upper object is container */
        if (NULL != last_object) {

            /* String in fly */
            if (true == string_in_buffer) {
                if(BRAMAJSON_ARRAY == last_object->type) {
                    object          = (bramajson_object*)malloc(sizeof (bramajson_object));
                    object->type    = BRAMAJSON_STRING;
                    object->_string = (char*)malloc(sizeof(char) * string_length + 1);
                    strncpy(object->_string, context->json_content + string_start, string_length);
                    object->_string[string_length] = '\0';

                    string_start     = 0;
                    string_length    = 0;
                    string_in_buffer = false;
                }
                else if (BRAMAJSON_DICT == last_object->type) {
                    if (NULL == last_object->_dictionary->last_pair) {
                        last_object->_dictionary->last_pair         = (bramajson_pair*)malloc(sizeof(bramajson_pair));
                        last_object->_dictionary->last_pair->key    = (char*)malloc(sizeof(char) * string_length + 1);
                        last_object->_dictionary->last_pair->object = NULL;
                        strncpy(last_object->_dictionary->last_pair->key, context->json_content + string_start, string_length);
                        last_object->_dictionary->last_pair->key[string_length] = '\0';

                        string_start     = 0;
                        string_length    = 0;
                        string_in_buffer = false;
                    } else {
                        object          = (bramajson_object*)malloc(sizeof (bramajson_object));
                        object->type    = BRAMAJSON_STRING;
                        object->_string = (char*)malloc(sizeof(char) * string_length + 1);
                        strncpy(object->_string, context->json_content + string_start, string_length);
                        object->_string[string_length] = '\0';

                        string_start     = 0;
                        string_length    = 0;
                        string_in_buffer = false;
                    }
                } else {
                    *status = BRAMAJSON_JSON_NOT_VALID;
                    break;
                }

                deliminator_used = false;
            }

            /* Object created and can be added to container */
            if (NULL != object) {
                switch (last_object->type) {
                
                    /* Upper object is array*/
                    case BRAMAJSON_ARRAY: {
                        if (last_object->_array->count == last_object->_array->size) {
                            last_object->_array->size *= 2;
                            last_object->_array->items = (bramajson_object**)realloc(last_object->_array->items, last_object->_array->size * sizeof(bramajson_object*));
                        }

                        last_object->_array->items[last_object->_array->count++] = object;
                        object->parent                                           = last_object;
                        deliminator_used                                         = false;
                        break;
                    }

                    case BRAMAJSON_DICT: {
                        if (last_object->_dictionary->count == last_object->_dictionary->size) {
                            last_object->_dictionary->size *= 2;
                            last_object->_dictionary->items = (bramajson_pair**)realloc(last_object->_dictionary->items, last_object->_dictionary->size * sizeof(bramajson_pair*));
                        }

                        last_object->_dictionary->last_pair->object = object;
                        last_object->_dictionary->items[last_object->_dictionary->count++] = last_object->_dictionary->last_pair;
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
                    *status = BRAMAJSON_JSON_NOT_VALID;
                    break;
                }

                --stack_index;
                last_object = stack[stack_index - 1];
                break;
            }
        }
    }

    if (BRAMAJSON_SUCCESS == *status) {
        if (NULL != object && ((BRAMAJSON_ARRAY == object->type && false == object->_array->closed) ||
            (BRAMAJSON_DICT  == object->type && false == object->_dictionary->closed))) {
            *status = BRAMAJSON_JSON_NOT_VALID;
            return NULL;
        }

        if (NULL == stack[0] && NULL != object)
            return object;
        else
            return stack[0];
    }
    return NULL;
}

bramajson_object* bramajson_parse(char const* json_content, int32_t* status) {
    return bramajson_parse_inner(json_content, status);
}

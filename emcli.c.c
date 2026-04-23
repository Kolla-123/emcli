#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>
#include <stdint.h>

/*
 * Choose the registration method here:
 * 1 = array based registry
 * 0 = linked list based registry
 */
#define ARRAY_BASED_COMMAND_REGISTER 1

/* Basic types */
typedef int base_type;
#define FALSE ((base_type)0)
#define PASS  ((base_type)1)
#define CUSTOM_CLI_MAX_COMMANDS 10
#define CLI_WRITE_BUFFER_SIZE 512

/* ========================= JSMN PARSER BEGIN ========================= */
#ifdef __cplusplus
extern "C" {
#endif

#ifdef JSMN_STATIC
#define JSMN_API static
#else
#define JSMN_API extern
#endif

typedef enum {
  JSMN_UNDEFINED = 0,
  JSMN_OBJECT = 1 << 0,
  JSMN_ARRAY = 1 << 1,
  JSMN_STRING = 1 << 2,
  JSMN_PRIMITIVE = 1 << 3
} jsmntype_t;

enum jsmnerr {
  JSMN_ERROR_NOMEM = -1,
  JSMN_ERROR_INVAL = -2,
  JSMN_ERROR_PART = -3
};

typedef struct jsmntok {
  jsmntype_t type;
  int start;
  int end;
  int size;
#ifdef JSMN_PARENT_LINKS
  int parent;
#endif
} jsmntok_t;

typedef struct jsmn_parser {
  unsigned int pos;
  unsigned int toknext;
  int toksuper;
} jsmn_parser;

JSMN_API void jsmn_init(jsmn_parser *parser);
JSMN_API int jsmn_parse(jsmn_parser *parser, const char *js, const size_t len,
                        jsmntok_t *tokens, const unsigned int num_tokens);

static jsmntok_t *jsmn_alloc_token(jsmn_parser *parser, jsmntok_t *tokens,
                                   const size_t num_tokens) {
  jsmntok_t *tok;
  if (parser->toknext >= num_tokens) {
    return NULL;
  }
  tok = &tokens[parser->toknext++];
  tok->start = tok->end = -1;
  tok->size = 0;
#ifdef JSMN_PARENT_LINKS
  tok->parent = -1;
#endif
  return tok;
}

static void jsmn_fill_token(jsmntok_t *token, const jsmntype_t type,
                            const int start, const int end) {
  token->type = type;
  token->start = start;
  token->end = end;
  token->size = 0;
}

static int jsmn_parse_primitive(jsmn_parser *parser, const char *js,
                                const size_t len, jsmntok_t *tokens,
                                const size_t num_tokens) {
  jsmntok_t *token;
  int start = (int)parser->pos;

  for (; parser->pos < len && js[parser->pos] != '\0'; parser->pos++) {
    switch (js[parser->pos]) {
#ifndef JSMN_STRICT
      case ':':
#endif
      case '\t':
      case '\r':
      case '\n':
      case ' ':
      case ',':
      case ']':
      case '}':
        goto found;
      default:
        break;
    }

    if (js[parser->pos] < 32 || js[parser->pos] >= 127) {
      parser->pos = (unsigned int)start;
      return JSMN_ERROR_INVAL;
    }
  }

#ifdef JSMN_STRICT
  parser->pos = (unsigned int)start;
  return JSMN_ERROR_PART;
#endif

found:
  if (tokens == NULL) {
    parser->pos--;
    return 0;
  }

  token = jsmn_alloc_token(parser, tokens, num_tokens);
  if (token == NULL) {
    parser->pos = (unsigned int)start;
    return JSMN_ERROR_NOMEM;
  }

  jsmn_fill_token(token, JSMN_PRIMITIVE, start, (int)parser->pos);
#ifdef JSMN_PARENT_LINKS
  token->parent = parser->toksuper;
#endif
  parser->pos--;
  return 0;
}

static int jsmn_parse_string(jsmn_parser *parser, const char *js,
                             const size_t len, jsmntok_t *tokens,
                             const size_t num_tokens) {
  jsmntok_t *token;
  int start = (int)parser->pos;

  parser->pos++;

  for (; parser->pos < len && js[parser->pos] != '\0'; parser->pos++) {
    char c = js[parser->pos];

    if (c == '\"') {
      if (tokens == NULL) {
        return 0;
      }

      token = jsmn_alloc_token(parser, tokens, num_tokens);
      if (token == NULL) {
        parser->pos = (unsigned int)start;
        return JSMN_ERROR_NOMEM;
      }

      jsmn_fill_token(token, JSMN_STRING, start + 1, (int)parser->pos);
#ifdef JSMN_PARENT_LINKS
      token->parent = parser->toksuper;
#endif
      return 0;
    }

    if (c == '\\' && parser->pos + 1 < len) {
      int i;
      parser->pos++;

      switch (js[parser->pos]) {
        case '\"':
        case '/':
        case '\\':
        case 'b':
        case 'f':
        case 'r':
        case 'n':
        case 't':
          break;

        case 'u':
          parser->pos++;
          for (i = 0; i < 4 && parser->pos < len && js[parser->pos] != '\0'; i++) {
            if (!((js[parser->pos] >= 48 && js[parser->pos] <= 57) ||
                  (js[parser->pos] >= 65 && js[parser->pos] <= 70) ||
                  (js[parser->pos] >= 97 && js[parser->pos] <= 102))) {
              parser->pos = (unsigned int)start;
              return JSMN_ERROR_INVAL;
            }
            parser->pos++;
          }
          parser->pos--;
          break;

        default:
          parser->pos = (unsigned int)start;
          return JSMN_ERROR_INVAL;
      }
    }
  }

  parser->pos = (unsigned int)start;
  return JSMN_ERROR_PART;
}

JSMN_API int jsmn_parse(jsmn_parser *parser, const char *js, const size_t len,
                        jsmntok_t *tokens, const unsigned int num_tokens) {
  int r;
  int i;
  jsmntok_t *token;
  int count = (int)parser->toknext;

  for (; parser->pos < len && js[parser->pos] != '\0'; parser->pos++) {
    char c = js[parser->pos];
    jsmntype_t type;

    switch (c) {
      case '{':
      case '[':
        count++;
        if (tokens == NULL) {
          break;
        }

        token = jsmn_alloc_token(parser, tokens, num_tokens);
        if (token == NULL) {
          return JSMN_ERROR_NOMEM;
        }

        if (parser->toksuper != -1) {
          jsmntok_t *t = &tokens[parser->toksuper];
#ifdef JSMN_STRICT
          if (t->type == JSMN_OBJECT) {
            return JSMN_ERROR_INVAL;
          }
#endif
          t->size++;
#ifdef JSMN_PARENT_LINKS
          token->parent = parser->toksuper;
#endif
        }

        token->type = (c == '{' ? JSMN_OBJECT : JSMN_ARRAY);
        token->start = (int)parser->pos;
        parser->toksuper = (int)parser->toknext - 1;
        break;

      case '}':
      case ']':
        if (tokens == NULL) {
          break;
        }

        type = (c == '}' ? JSMN_OBJECT : JSMN_ARRAY);
#ifdef JSMN_PARENT_LINKS
        if (parser->toknext < 1) {
          return JSMN_ERROR_INVAL;
        }

        token = &tokens[parser->toknext - 1];
        for (;;) {
          if (token->start != -1 && token->end == -1) {
            if (token->type != type) {
              return JSMN_ERROR_INVAL;
            }
            token->end = (int)parser->pos + 1;
            parser->toksuper = token->parent;
            break;
          }
          if (token->parent == -1) {
            if (token->type != type || parser->toksuper == -1) {
              return JSMN_ERROR_INVAL;
            }
            break;
          }
          token = &tokens[token->parent];
        }
#else
        for (i = (int)parser->toknext - 1; i >= 0; i--) {
          token = &tokens[i];
          if (token->start != -1 && token->end == -1) {
            if (token->type != type) {
              return JSMN_ERROR_INVAL;
            }
            parser->toksuper = -1;
            token->end = (int)parser->pos + 1;
            break;
          }
        }

        if (i == -1) {
          return JSMN_ERROR_INVAL;
        }

        for (; i >= 0; i--) {
          token = &tokens[i];
          if (token->start != -1 && token->end == -1) {
            parser->toksuper = i;
            break;
          }
        }
#endif
        break;

      case '\"':
        r = jsmn_parse_string(parser, js, len, tokens, num_tokens);
        if (r < 0) {
          return r;
        }
        count++;
        if (parser->toksuper != -1 && tokens != NULL) {
          tokens[parser->toksuper].size++;
        }
        break;

      case '\t':
      case '\r':
      case '\n':
      case ' ':
        break;

      case ':':
        parser->toksuper = (int)parser->toknext - 1;
        break;

      case ',':
        if (tokens != NULL && parser->toksuper != -1 &&
            tokens[parser->toksuper].type != JSMN_ARRAY &&
            tokens[parser->toksuper].type != JSMN_OBJECT) {
#ifdef JSMN_PARENT_LINKS
          parser->toksuper = tokens[parser->toksuper].parent;
#else
          for (i = (int)parser->toknext - 1; i >= 0; i--) {
            if ((tokens[i].type == JSMN_ARRAY || tokens[i].type == JSMN_OBJECT) &&
                tokens[i].start != -1 && tokens[i].end == -1) {
              parser->toksuper = i;
              break;
            }
          }
#endif
        }
        break;

#ifdef JSMN_STRICT
      case '-':
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
      case 't':
      case 'f':
      case 'n':
        if (tokens != NULL && parser->toksuper != -1) {
          const jsmntok_t *t = &tokens[parser->toksuper];
          if (t->type == JSMN_OBJECT ||
              (t->type == JSMN_STRING && t->size != 0)) {
            return JSMN_ERROR_INVAL;
          }
        }
#else
      default:
#endif
        r = jsmn_parse_primitive(parser, js, len, tokens, num_tokens);
        if (r < 0) {
          return r;
        }
        count++;
        if (parser->toksuper != -1 && tokens != NULL) {
          tokens[parser->toksuper].size++;
        }
        break;

#ifdef JSMN_STRICT
      default:
        return JSMN_ERROR_INVAL;
#endif
    }
  }

  if (tokens != NULL) {
    for (i = (int)parser->toknext - 1; i >= 0; i--) {
      if (tokens[i].start != -1 && tokens[i].end == -1) {
        return JSMN_ERROR_PART;
      }
    }
  }

  return count;
}

JSMN_API void jsmn_init(jsmn_parser *parser) {
  parser->pos = 0;
  parser->toknext = 0;
  parser->toksuper = -1;
}

#ifdef __cplusplus
}
#endif
/* ========================== JSMN PARSER END ========================== */

/* ============================ CLI BEGIN ============================== */
typedef struct command_line_input {
  const char *command;
  const char *help_string;
  base_type (*command_interpreter)(char *write_buffer, size_t write_buffer_len,
                                   const char *command_string);
  int8_t expected_number_of_parameters;
  base_type is_special_command;
} CLI_Command_Definition;

#if !ARRAY_BASED_COMMAND_REGISTER
typedef struct command_input_list {
  const CLI_Command_Definition *command_line_definition;
  struct command_input_list *next;
} CLI_Definition_List_Item;
#endif

static int command_count = 0;
static char write_buffer[CLI_WRITE_BUFFER_SIZE];

#if ARRAY_BASED_COMMAND_REGISTER
static CLI_Command_Definition commands_array[CUSTOM_CLI_MAX_COMMANDS];
#else
static CLI_Definition_List_Item *registered_commands_head = NULL;
static CLI_Definition_List_Item *registered_commands_tail = NULL;
#endif

static base_type register_command(const CLI_Command_Definition *command_to_register);
static base_type process_command(const char *command_input, char *write_buffer,
                                 size_t write_buffer_len);
static base_type get_number_of_parameters(const char *command_string);
static const char *cli_get_parameter(const char *command_string,
                                     base_type wanted_parameter,
                                     base_type *parameter_string_length);
static void append_text(char *write_buffer, size_t write_buffer_len, const char *text);
static const CLI_Command_Definition *find_command_definition(const char *command_input);

static base_type help_command_interpreter(char *write_buffer, size_t write_buffer_len,
                                          const char *command_string);
static base_type set_command_interpreter(char *write_buffer, size_t write_buffer_len,
                                         const char *command_string);
static base_type get_command_interpreter(char *write_buffer, size_t write_buffer_len,
                                         const char *command_string);
static base_type list_command_interpreter(char *write_buffer, size_t write_buffer_len,
                                          const char *command_string);

static int jsmn_list_output(char *write_buffer, size_t write_buffer_len);
static int jsoneq(const char *json, jsmntok_t *tok, const char *s);

static const CLI_Command_Definition help_command = {
  "help",
  "help: Lists all registered commands",
  help_command_interpreter,
  0,
  PASS
};

static const CLI_Command_Definition set_command = {
  "set",
  "set <parameter> <value>: Sets a value in the system",
  set_command_interpreter,
  2,
  FALSE
};

static const CLI_Command_Definition get_command = {
  "get",
  "get <parameter>: Gets a value from the system",
  get_command_interpreter,
  1,
  FALSE
};

static const CLI_Command_Definition list_command = {
  "list",
  "list: Shows user/admin/uid/groups values",
  list_command_interpreter,
  0,
  FALSE
};

static base_type register_command(const CLI_Command_Definition *command_to_register) {
  if (command_to_register == NULL || command_count >= CUSTOM_CLI_MAX_COMMANDS) {
    return FALSE;
  }

#if ARRAY_BASED_COMMAND_REGISTER
  commands_array[command_count] = *command_to_register;
#else
  CLI_Definition_List_Item *new_item =
      (CLI_Definition_List_Item *)malloc(sizeof(CLI_Definition_List_Item));
  if (new_item == NULL) {
    return FALSE;
  }

  new_item->command_line_definition = command_to_register;
  new_item->next = NULL;

  if (registered_commands_head == NULL) {
    registered_commands_head = new_item;
    registered_commands_tail = new_item;
  } else {
    registered_commands_tail->next = new_item;
    registered_commands_tail = new_item;
  }
#endif

  command_count++;
  return PASS;
}

static const CLI_Command_Definition *find_command_definition(const char *command_input) {
  const char *registered_command_string;
  base_type command_string_length;

#if ARRAY_BASED_COMMAND_REGISTER
  int command_index;
  for (command_index = 0; command_index < command_count; command_index++) {
    registered_command_string = commands_array[command_index].command;
    command_string_length = (base_type)strlen(registered_command_string);

    if ((command_input[command_string_length] == ' ') ||
        (command_input[command_string_length] == 0x00)) {
      if (strncmp(command_input, registered_command_string,
                  (size_t)command_string_length) == 0) {
        return &commands_array[command_index];
      }
    }
  }
#else
  CLI_Definition_List_Item *current = registered_commands_head;
  while (current != NULL) {
    registered_command_string = current->command_line_definition->command;
    command_string_length = (base_type)strlen(registered_command_string);

    if ((command_input[command_string_length] == ' ') ||
        (command_input[command_string_length] == 0x00)) {
      if (strncmp(command_input, registered_command_string,
                  (size_t)command_string_length) == 0) {
        return current->command_line_definition;
      }
    }
    current = current->next;
  }
#endif

  return NULL;
}

static base_type process_command(const char *command_input, char *write_buffer,
                                 size_t write_buffer_len) {
  const CLI_Command_Definition *command_def = find_command_definition(command_input);

  memset(write_buffer, 0, write_buffer_len);

  if (command_def == NULL) {
    strncpy(write_buffer,
            "Command not recognised. Enter 'help' to view the list of commands.\r\n",
            write_buffer_len - 1U);
    write_buffer[write_buffer_len - 1U] = '\0';
    return FALSE;
  }

  if (command_def->expected_number_of_parameters >= 0) {
    if (get_number_of_parameters(command_input) !=
        command_def->expected_number_of_parameters) {
      strncpy(write_buffer,
              "Incorrect command parameter(s). Enter 'help' to view usage.\r\n",
              write_buffer_len - 1U);
      write_buffer[write_buffer_len - 1U] = '\0';
      return FALSE;
    }
  }

  return command_def->command_interpreter(write_buffer, write_buffer_len,
                                          command_input);
}

static base_type help_command_interpreter(char *write_buffer, size_t write_buffer_len,
                                          const char *command_string) {
  (void)command_string;
  memset(write_buffer, 0, write_buffer_len);

#if ARRAY_BASED_COMMAND_REGISTER
  int i;
  for (i = 0; i < command_count; i++) {
    append_text(write_buffer, write_buffer_len, commands_array[i].help_string);
    append_text(write_buffer, write_buffer_len, "\r\n");
  }
#else
  CLI_Definition_List_Item *current = registered_commands_head;
  while (current != NULL) {
    append_text(write_buffer, write_buffer_len,
                current->command_line_definition->help_string);
    append_text(write_buffer, write_buffer_len, "\r\n");
    current = current->next;
  }
#endif

  return FALSE;
}

static base_type set_command_interpreter(char *write_buffer, size_t write_buffer_len,
                                         const char *command_string) {
  const char *parameter;
  base_type string_length = 0;
  char name[64] = {0};
  char value[64] = {0};

  parameter = cli_get_parameter(command_string, 1, &string_length);
  if (parameter != NULL && string_length < (base_type)sizeof(name)) {
    strncpy(name, parameter, (size_t)string_length);
    name[string_length] = '\0';
  }

  parameter = cli_get_parameter(command_string, 2, &string_length);
  if (parameter != NULL && string_length < (base_type)sizeof(value)) {
    strncpy(value, parameter, (size_t)string_length);
    value[string_length] = '\0';
  }

  snprintf(write_buffer, write_buffer_len,
           "SET command executed\r\nParameter: %s\r\nValue: %s\r\n",
           name, value);
  return FALSE;
}

static base_type get_command_interpreter(char *write_buffer, size_t write_buffer_len,
                                         const char *command_string) {
  const char *parameter;
  base_type string_length = 0;
  char name[64] = {0};

  parameter = cli_get_parameter(command_string, 1, &string_length);
  if (parameter != NULL && string_length < (base_type)sizeof(name)) {
    strncpy(name, parameter, (size_t)string_length);
    name[string_length] = '\0';
  }

  snprintf(write_buffer, write_buffer_len,
           "GET command executed\r\nRequested parameter: %s\r\n",
           name);
  return FALSE;
}

static base_type list_command_interpreter(char *write_buffer, size_t write_buffer_len,
                                          const char *command_string) {
  (void)command_string;
  if (jsmn_list_output(write_buffer, write_buffer_len) < 0) {
    strncpy(write_buffer, "LIST command failed\r\n", write_buffer_len - 1U);
    write_buffer[write_buffer_len - 1U] = '\0';
  }
  return FALSE;
}

static base_type get_number_of_parameters(const char *command_string) {
  base_type parameter_count = 0;
  base_type is_last_character_space = FALSE;

  while (*command_string != 0x00) {
    if (*command_string == ' ') {
      if (is_last_character_space != PASS) {
        parameter_count++;
        is_last_character_space = PASS;
      }
    } else {
      is_last_character_space = FALSE;
    }
    command_string++;
  }

  if (is_last_character_space == PASS) {
    parameter_count--;
  }

  return parameter_count;
}

static const char *cli_get_parameter(const char *command_string,
                                     base_type wanted_parameter,
                                     base_type *parameter_string_length) {
  base_type parameters_found = 0;
  const char *return_param = NULL;

  *parameter_string_length = 0;

  while (parameters_found < wanted_parameter) {
    while ((*command_string != 0x00) && (*command_string != ' ')) {
      command_string++;
    }

    while ((*command_string != 0x00) && (*command_string == ' ')) {
      command_string++;
    }

    if (*command_string != 0x00) {
      parameters_found++;

      if (parameters_found == wanted_parameter) {
        return_param = command_string;
        while ((*command_string != 0x00) && (*command_string != ' ')) {
          (*parameter_string_length)++;
          command_string++;
        }

        if (*parameter_string_length == 0) {
          return_param = NULL;
        }
        break;
      }
    } else {
      break;
    }
  }

  return return_param;
}

static void append_text(char *write_buffer, size_t write_buffer_len, const char *text) {
  size_t current_len = strlen(write_buffer);
  size_t remaining = (current_len < write_buffer_len)
                       ? (write_buffer_len - current_len - 1U)
                       : 0U;

  if (remaining > 0U) {
    strncat(write_buffer, text, remaining);
  }
}
/* ============================= CLI END =============================== */

/* ======================== DATA PARSE HELPER ========================== */
static const char *JSON_STRING =
    "{\"user\": \"Robin\", \"admin\": false, \"uid\": 1000, "
    "\"groups\": [\"users\", \"wheel\", \"text\", \"video\"]}";

static int jsoneq(const char *json, jsmntok_t *tok, const char *s) {
  if (tok->type == JSMN_STRING &&
      (int)strlen(s) == tok->end - tok->start &&
      strncmp(json + tok->start, s, (size_t)(tok->end - tok->start)) == 0) {
    return 0;
  }
  return -1;
}

static int jsmn_list_output(char *write_buffer, size_t write_buffer_len) {
  int i;
  int r;
  jsmn_parser p;
  jsmntok_t t[128];
  char temp[128];

  memset(write_buffer, 0, write_buffer_len);

  jsmn_init(&p);
  r = jsmn_parse(&p, JSON_STRING, strlen(JSON_STRING), t,
                 (unsigned int)(sizeof(t) / sizeof(t[0])));
  if (r < 0) {
    return r;
  }

  if (r < 1 || t[0].type != JSMN_OBJECT) {
    return -100;
  }

  append_text(write_buffer, write_buffer_len, "LIST command executed\r\n");

  for (i = 1; i < r; i++) {
    if (jsoneq(JSON_STRING, &t[i], "user") == 0) {
      snprintf(temp, sizeof(temp), "User: %.*s\r\n",
               t[i + 1].end - t[i + 1].start,
               JSON_STRING + t[i + 1].start);
      append_text(write_buffer, write_buffer_len, temp);
      i++;
    } else if (jsoneq(JSON_STRING, &t[i], "admin") == 0) {
      snprintf(temp, sizeof(temp), "Admin: %.*s\r\n",
               t[i + 1].end - t[i + 1].start,
               JSON_STRING + t[i + 1].start);
      append_text(write_buffer, write_buffer_len, temp);
      i++;
    } else if (jsoneq(JSON_STRING, &t[i], "uid") == 0) {
      snprintf(temp, sizeof(temp), "UID: %.*s\r\n",
               t[i + 1].end - t[i + 1].start,
               JSON_STRING + t[i + 1].start);
      append_text(write_buffer, write_buffer_len, temp);
      i++;
    } else if (jsoneq(JSON_STRING, &t[i], "groups") == 0) {
      int j;
      append_text(write_buffer, write_buffer_len, "Groups:\r\n");

      if (t[i + 1].type != JSMN_ARRAY) {
        continue;
      }

      for (j = 0; j < t[i + 1].size; j++) {
        jsmntok_t *g = &t[i + j + 2];
        snprintf(temp, sizeof(temp), "  %.*s\r\n",
                 g->end - g->start, JSON_STRING + g->start);
        append_text(write_buffer, write_buffer_len, temp);
      }

      i += t[i + 1].size + 1;
    }
  }

  return 0;
}
/* ====================== END DATA PARSE HELPER ======================== */

int main(void) {
  register_command(&help_command);
  register_command(&set_command);
  register_command(&get_command);
  register_command(&list_command);

#if ARRAY_BASED_COMMAND_REGISTER
  printf("Registry type: ARRAY BASED\n\n");
#else
  printf("Registry type: LINKED LIST BASED\n\n");
#endif

  process_command("help", write_buffer, sizeof(write_buffer));
  printf("%s\n", write_buffer);

  process_command("set temp 25", write_buffer, sizeof(write_buffer));
  printf("%s\n", write_buffer);

  process_command("get temp", write_buffer, sizeof(write_buffer));
  printf("%s\n", write_buffer);

  process_command("list", write_buffer, sizeof(write_buffer));
  printf("%s\n", write_buffer);

  process_command("wrongcmd", write_buffer, sizeof(write_buffer));
  printf("%s\n", write_buffer);

  return 0;
}

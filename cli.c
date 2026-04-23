#include "cli.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

/* ======================== DATA PARSE HELPER ========================== */

static const char *JSON_STRING =
    "{\"user\": \"Robin\", \"admin\": false, \"uid\": 1000, "
    "\"groups\": [\"users\", \"wheel\", \"text\", \"video\"]}";

static int8_t command_count = 0;
static char write_buffer[CLI_WRITE_BUFFER_SIZE];

#if ARRAY_BASED_COMMAND_REGISTER
static CLI_Command_Definition commands_array[CUSTOM_CLI_MAX_COMMANDS];
#else
static CLI_Definition_List_Item *registered_commands_head = NULL;
static CLI_Definition_List_Item *registered_commands_tail = NULL;
#endif

/* ======================== Static Helper Functions ======================== */

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

/* ======================== Command Interpreters ======================== */

base_type cli_help_command(char *write_buffer, size_t write_buffer_len,
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

base_type cli_set_command(char *write_buffer, size_t write_buffer_len,
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

base_type cli_get_command(char *write_buffer, size_t write_buffer_len,
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

base_type cli_list_command(char *write_buffer, size_t write_buffer_len,
                           const char *command_string) {
  (void)command_string;
  if (jsmn_list_output(write_buffer, write_buffer_len) < 0) {
    strncpy(write_buffer, "LIST command failed\r\n", write_buffer_len - 1U);
    write_buffer[write_buffer_len - 1U] = '\0';
  }
  return FALSE;
}

/* ======================== Public CLI Functions ======================== */

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

base_type cli_register_command(const CLI_Command_Definition *command_to_register) {
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

base_type cli_process_command(const char *command_input, char *write_buffer,
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

/* ====================== END DATA PARSE HELPER ======================== */

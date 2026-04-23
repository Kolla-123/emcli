#ifndef CLI_H
#define CLI_H

#include "config.h"
#include "jsmn.h"
#include <stddef.h>

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

/* Public API */
base_type cli_register_command(const CLI_Command_Definition *command_to_register);
base_type cli_process_command(const char *command_input, char *write_buffer,
                              size_t write_buffer_len);

/* Built-in command interpreters */
base_type cli_help_command(char *write_buffer, size_t write_buffer_len,
                           const char *command_string);
base_type cli_set_command(char *write_buffer, size_t write_buffer_len,
                          const char *command_string);
base_type cli_get_command(char *write_buffer, size_t write_buffer_len,
                          const char *command_string);
base_type cli_list_command(char *write_buffer, size_t write_buffer_len,
                           const char *command_string);

/* ============================= CLI END =============================== */

#endif /* CLI_H */

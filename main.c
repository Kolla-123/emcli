#include <stdio.h>
#include "cli.h"
#include "config.h"

/* Command Definitions */
static const CLI_Command_Definition help_command = {
  "help",
  "help: Lists all registered commands",
  cli_help_command,
  0,
  PASS
};

static const CLI_Command_Definition set_command = {
  "set",
  "set <parameter> <value>: Sets a value in the system",
  cli_set_command,
  2,
  FALSE
};

static const CLI_Command_Definition get_command = {
  "get",
  "get <parameter>: Gets a value from the system",
  cli_get_command,
  1,
  FALSE
};

static const CLI_Command_Definition list_command = {
  "list",
  "list: Shows user/admin/uid/groups values",
  cli_list_command,
  0,
  FALSE
};

int main(void) {
  static char write_buffer[CLI_WRITE_BUFFER_SIZE];

  /* Register commands */
  cli_register_command(&help_command);
  cli_register_command(&set_command);
  cli_register_command(&get_command);
  cli_register_command(&list_command);

#if ARRAY_BASED_COMMAND_REGISTER
  printf("Registry type: ARRAY BASED\n\n");
#else
  printf("Registry type: LINKED LIST BASED\n\n");
#endif

  /* Test: help command */
  cli_process_command("help", write_buffer, sizeof(write_buffer));
  printf("%s\n", write_buffer);

  /* Test: set command */
  cli_process_command("set temp 25", write_buffer, sizeof(write_buffer));
  printf("%s\n", write_buffer);

  /* Test: get command */
  cli_process_command("get temp", write_buffer, sizeof(write_buffer));
  printf("%s\n", write_buffer);

  /* Test: list command */
  cli_process_command("list", write_buffer, sizeof(write_buffer));
  printf("%s\n", write_buffer);

  /* Test: invalid command */
  cli_process_command("wrongcmd", write_buffer, sizeof(write_buffer));
  printf("%s\n", write_buffer);

  return 0;
}

#include <stdio.h>
#include <string.h>
#include "cli.h"
#include "config.h"
#include "getset_protocol.h"

#define APP_STORE_VALUE_SIZE        (16U)
#define APP_RESPONSE_VALUE_SIZE     (64U)
#define APP_KEY_LENGTH              (4U)
#define APP_SET_PREFIX_LENGTH       (5U)
#define APP_TEMP_KEY                "temp"
#define APP_MODE_KEY                "mode"
#define APP_TEMP_SET_PREFIX         "temp="
#define APP_MODE_SET_PREFIX         "mode="
#define APP_UNKNOWN_KEY_RESPONSE    "ERR=UNKNOWN_KEY"
#define APP_SUCCESS_RESPONSE        "OK"
#define APP_NO_COMMAND_PARAMETERS   ((int8_t)0)
#define APP_ONE_COMMAND_PARAMETER   ((int8_t)1)
#define APP_TWO_COMMAND_PARAMETERS  ((int8_t)2)

typedef struct app_store {
  char temp[APP_STORE_VALUE_SIZE];
  char mode[APP_STORE_VALUE_SIZE];
} App_Store;

static GetSet_Status app_get_handler(const GetSet_Packet *packet,
                                     GetSet_Packet *response,
                                     void *user_context) {
  App_Store *store = (App_Store *)user_context;
  char response_value[APP_RESPONSE_VALUE_SIZE];
  const char *key;

  if (packet == NULL || response == NULL || store == NULL) {
    return GETSET_STATUS_NULL_ARG;
  }

  if (packet->type != GETSET_TYPE_STR) {
    return GETSET_STATUS_BAD_LENGTH;
  }

  key = (const char *)packet->value;
  if (packet->length == APP_KEY_LENGTH &&
      strncmp(key, APP_TEMP_KEY, APP_KEY_LENGTH) == 0) {
    snprintf(response_value, sizeof(response_value), "temp=%s", store->temp);
  } else if (packet->length == APP_KEY_LENGTH &&
             strncmp(key, APP_MODE_KEY, APP_KEY_LENGTH) == 0) {
    snprintf(response_value, sizeof(response_value), "mode=%s", store->mode);
  } else {
    snprintf(response_value, sizeof(response_value), "%s",
             APP_UNKNOWN_KEY_RESPONSE);
  }

  return getset_packet_init(response, GETSET_OPCODE_ACK_DATA, GETSET_TYPE_STR,
                            (const uint8_t *)response_value,
                            (uint16_t)strlen(response_value));
}

static GetSet_Status app_set_handler(const GetSet_Packet *packet,
                                     GetSet_Packet *response,
                                     void *user_context) {
  App_Store *store = (App_Store *)user_context;
  const char *payload;
  const char *ack_value = APP_SUCCESS_RESPONSE;

  if (packet == NULL || response == NULL || store == NULL) {
    return GETSET_STATUS_NULL_ARG;
  }

  if (packet->type != GETSET_TYPE_STR) {
    return GETSET_STATUS_BAD_LENGTH;
  }

  payload = (const char *)packet->value;
  if (packet->length > APP_SET_PREFIX_LENGTH &&
      strncmp(payload, APP_TEMP_SET_PREFIX, APP_SET_PREFIX_LENGTH) == 0) {
    snprintf(store->temp, sizeof(store->temp), "%.*s",
             (int)(packet->length - APP_SET_PREFIX_LENGTH),
             payload + APP_SET_PREFIX_LENGTH);
  } else if (packet->length > APP_SET_PREFIX_LENGTH &&
             strncmp(payload, APP_MODE_SET_PREFIX, APP_SET_PREFIX_LENGTH) == 0) {
    snprintf(store->mode, sizeof(store->mode), "%.*s",
             (int)(packet->length - APP_SET_PREFIX_LENGTH),
             payload + APP_SET_PREFIX_LENGTH);
  } else {
    ack_value = APP_UNKNOWN_KEY_RESPONSE;
  }

  return getset_packet_init(response, GETSET_OPCODE_ACK_DATA, GETSET_TYPE_STR,
                            (const uint8_t *)ack_value,
                            (uint16_t)strlen(ack_value));
}

/* Command Definitions */
static const CLI_Command_Definition help_command = {
  "help",
  "help: Lists all registered commands",
  cli_help_command,
  APP_NO_COMMAND_PARAMETERS,
  PASS
};

static const CLI_Command_Definition set_command = {
  "set",
  "set <parameter> <value>: Sets a value in the system",
  cli_set_command,
  APP_TWO_COMMAND_PARAMETERS,
  FALSE
};

static const CLI_Command_Definition get_command = {
  "get",
  "get <parameter>: Gets a value from the system",
  cli_get_command,
  APP_ONE_COMMAND_PARAMETER,
  FALSE
};

static const CLI_Command_Definition list_command = {
  "list",
  "list: Shows user/admin/uid/groups values",
  cli_list_command,
  APP_NO_COMMAND_PARAMETERS,
  FALSE
};

int main(void) {
  static char write_buffer[CLI_WRITE_BUFFER_SIZE];
  App_Store store = {"25", "auto"};
  GetSet_Context getset_context;

  getset_context_init(&getset_context, &store);
  getset_register_handler(&getset_context, GETSET_OPCODE_GET, app_get_handler);
  getset_register_handler(&getset_context, GETSET_OPCODE_SET, app_set_handler);
  cli_set_getset_context(&getset_context);

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

  /* Test: get command */
  cli_process_command("get temp", write_buffer, sizeof(write_buffer));
  printf("%s\n", write_buffer);

  /* Test: set command */
  cli_process_command("set temp 30", write_buffer, sizeof(write_buffer));
  printf("%s\n", write_buffer);

  /* Test: get command after SET updated the application store */
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

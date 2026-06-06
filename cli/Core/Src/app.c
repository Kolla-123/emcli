#include "app.h"
#include "cli.h"
#include "getset_protocol.h"
#include <stdio.h>
#include <string.h>

#define APP_STORE_VALUE_SIZE      (16U)
#define APP_RESPONSE_VALUE_SIZE   (64U)
#define APP_KEY_LENGTH            (4U)
#define APP_SET_PREFIX_LENGTH     (5U)
#define APP_TEMP_KEY              "temp"
#define APP_MODE_KEY              "mode"
#define APP_TEMP_SET_PREFIX       "temp="
#define APP_MODE_SET_PREFIX       "mode="
#define APP_UNKNOWN_KEY_RESPONSE  "ERR=UNKNOWN_KEY"
#define APP_SUCCESS_RESPONSE      "OK"
#define APP_NO_COMMAND_PARAMETERS ((int8_t)0)
#define APP_ONE_COMMAND_PARAMETER ((int8_t)1)
#define APP_TWO_COMMAND_PARAMETERS ((int8_t)2)

typedef struct app_store {
  char temp[APP_STORE_VALUE_SIZE];
  char mode[APP_STORE_VALUE_SIZE];
} App_Store;

static App_Store app_store = {"25", "auto"};
static GetSet_Context getset_context;

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

void app_init(void) {
  getset_context_init(&getset_context, &app_store);
  (void)getset_register_handler(&getset_context, GETSET_OPCODE_GET,
                                app_get_handler);
  (void)getset_register_handler(&getset_context, GETSET_OPCODE_SET,
                                app_set_handler);
  cli_set_getset_context(&getset_context);

  (void)cli_register_command(&help_command);
  (void)cli_register_command(&set_command);
  (void)cli_register_command(&get_command);
  (void)cli_register_command(&list_command);
}

#include "cli.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

/* ======================== DATA PARSE HELPER ========================== */

#define CLI_PARAMETER_BUFFER_SIZE       (64U)
#define CLI_PROTOCOL_VALUE_BUFFER_SIZE  (128U)
#define CLI_TEXT_BUFFER_SIZE            (96U)
#define CLI_JSON_TOKEN_COUNT            (128U)
#define CLI_FIRST_PARAMETER             ((base_type)1)
#define CLI_SECOND_PARAMETER            ((base_type)2)
#define CLI_PARSE_ERROR_INVALID_OBJECT  (-100)

static const char json_string[] =
    "{\"user\": \"Robin\", \"admin\": false, \"uid\": 1000, "
    "\"groups\": [\"users\", \"wheel\", \"text\", \"video\"]}";

static size_t command_count = 0U;
static GetSet_Context *protocol_context = NULL;

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

  if (command_string == NULL) {
    return 0;
  }

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

  if (command_string == NULL || parameter_string_length == NULL) {
    return NULL;
  }

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
  size_t current_len;
  size_t remaining;

  if (write_buffer == NULL || text == NULL || write_buffer_len == 0U) {
    return;
  }

  current_len = strlen(write_buffer);
  remaining = (current_len < write_buffer_len)
                ? (write_buffer_len - current_len - 1U)
                : 0U;

  if (remaining > 0U) {
    strncat(write_buffer, text, remaining);
  }
}

static base_type copy_parameter(char *destination, size_t destination_len,
                                const char *command_string,
                                base_type wanted_parameter) {
  const char *parameter;
  base_type string_length = 0;

  if (destination == NULL || command_string == NULL || destination_len == 0U) {
    return CLI_FAILURE;
  }

  destination[0] = '\0';
  parameter = cli_get_parameter(command_string, wanted_parameter, &string_length);
  if (parameter == NULL || string_length <= 0) {
    return CLI_FAILURE;
  }

  if ((size_t)string_length >= destination_len) {
    return CLI_FAILURE;
  }

  memcpy(destination, parameter, (size_t)string_length);
  destination[string_length] = '\0';
  return CLI_SUCCESS;
}

static void append_hex_frame(char *write_buffer, size_t write_buffer_len,
                             const uint8_t *frame, size_t frame_len) {
  size_t index;
  char temp[4];

  if (frame == NULL && frame_len > 0U) {
    return;
  }

  for (index = 0U; index < frame_len; index++) {
    snprintf(temp, sizeof(temp), "%02X", frame[index]);
    append_text(write_buffer, write_buffer_len, temp);
    if ((index + 1U) < frame_len) {
      append_text(write_buffer, write_buffer_len, " ");
    }
  }
}

static void append_packet_summary(char *write_buffer, size_t write_buffer_len,
                                  const GetSet_Packet *packet) {
  char temp[CLI_TEXT_BUFFER_SIZE];

  if (packet == NULL) {
    append_text(write_buffer, write_buffer_len, "Packet unavailable\r\n");
    return;
  }

  snprintf(temp, sizeof(temp), "SOF: 0x%02X\r\n", packet->sof);
  append_text(write_buffer, write_buffer_len, temp);

  snprintf(temp, sizeof(temp), "OPCODE: 0x%02X (%s)\r\n", packet->opcode,
           getset_opcode_to_string(packet->opcode));
  append_text(write_buffer, write_buffer_len, temp);

  snprintf(temp, sizeof(temp), "TYPE: 0x%02X (%s)\r\n", packet->type,
           getset_type_to_string(packet->type));
  append_text(write_buffer, write_buffer_len, temp);

  snprintf(temp, sizeof(temp), "LENGTH: 0x%04X (%u bytes)\r\n", packet->length,
           (unsigned int)packet->length);
  append_text(write_buffer, write_buffer_len, temp);

  append_text(write_buffer, write_buffer_len, "VALUE: ");
  if (packet->type == GETSET_TYPE_STR) {
    snprintf(temp, sizeof(temp), "%.*s\r\n", (int)packet->length,
             (const char *)packet->value);
    append_text(write_buffer, write_buffer_len, temp);
  } else {
    append_hex_frame(write_buffer, write_buffer_len, packet->value,
                     packet->length);
    append_text(write_buffer, write_buffer_len, "\r\n");
  }

  snprintf(temp, sizeof(temp), "CSUM: 0x%02X\r\n", packet->checksum);
  append_text(write_buffer, write_buffer_len, temp);
}

static void run_protocol_demo(char *write_buffer, size_t write_buffer_len,
                              const GetSet_Packet *tx_packet) {
  uint8_t frame[GETSET_MAX_FRAME_SIZE];
  uint8_t response_frame[GETSET_MAX_FRAME_SIZE];
  size_t frame_len = 0U;
  size_t response_frame_len = 0U;
  GetSet_Packet rx_packet;
  GetSet_Packet response_packet;
  GetSet_Status status;
  char temp[CLI_TEXT_BUFFER_SIZE];

  if (write_buffer == NULL || tx_packet == NULL || write_buffer_len == 0U) {
    return;
  }

  status = getset_encode_packet(tx_packet, frame, sizeof(frame), &frame_len);
  if (status != GETSET_STATUS_OK) {
    snprintf(write_buffer, write_buffer_len, "Encode failed: %s\r\n",
             getset_status_to_string(status));
    return;
  }

  append_text(write_buffer, write_buffer_len, "TX frame: ");
  append_hex_frame(write_buffer, write_buffer_len, frame, frame_len);
  append_text(write_buffer, write_buffer_len, "\r\n\r\nDecoded packet blocks:\r\n");

  status = getset_decode_frame(frame, frame_len, &rx_packet);
  if (status != GETSET_STATUS_OK) {
    snprintf(temp, sizeof(temp), "Decode failed: %s\r\n",
             getset_status_to_string(status));
    append_text(write_buffer, write_buffer_len, temp);
    return;
  }

  append_packet_summary(write_buffer, write_buffer_len, &rx_packet);

  if (protocol_context == NULL) {
    append_text(write_buffer, write_buffer_len,
                "Dispatch skipped: no GET-SET context registered\r\n");
    return;
  }

  status = getset_dispatch_packet(protocol_context, &rx_packet,
                                  &response_packet);
  snprintf(temp, sizeof(temp), "\r\nDispatch status: %s\r\n",
           getset_status_to_string(status));
  append_text(write_buffer, write_buffer_len, temp);
  if (status != GETSET_STATUS_OK) {
    return;
  }

  status = getset_encode_packet(&response_packet, response_frame,
                                sizeof(response_frame), &response_frame_len);
  if (status != GETSET_STATUS_OK) {
    snprintf(temp, sizeof(temp), "Response encode failed: %s\r\n",
             getset_status_to_string(status));
    append_text(write_buffer, write_buffer_len, temp);
    return;
  }

  append_text(write_buffer, write_buffer_len, "Response frame: ");
  append_hex_frame(write_buffer, write_buffer_len, response_frame,
                   response_frame_len);
  append_text(write_buffer, write_buffer_len, "\r\n");
  append_packet_summary(write_buffer, write_buffer_len, &response_packet);
}

static int jsoneq(const char *json, const jsmntok_t *tok, const char *s) {
  if (json == NULL || tok == NULL || s == NULL) {
    return -1;
  }

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
  jsmntok_t t[CLI_JSON_TOKEN_COUNT];
  char temp[CLI_PROTOCOL_VALUE_BUFFER_SIZE];

  if (write_buffer == NULL || write_buffer_len == 0U) {
    return CLI_PARSE_ERROR_INVALID_OBJECT;
  }

  memset(write_buffer, 0, write_buffer_len);

  jsmn_init(&p);
  r = jsmn_parse(&p, json_string, strlen(json_string), t,
                 (unsigned int)(sizeof(t) / sizeof(t[0])));
  if (r < 0) {
    return r;
  }

  if (r < 1 || t[0].type != JSMN_OBJECT) {
    return CLI_PARSE_ERROR_INVALID_OBJECT;
  }

  append_text(write_buffer, write_buffer_len, "LIST command executed\r\n");

  for (i = 1; i < r; i++) {
    if ((i + 1) >= r) {
      return CLI_PARSE_ERROR_INVALID_OBJECT;
    }

    if (jsoneq(json_string, &t[i], "user") == 0) {
      snprintf(temp, sizeof(temp), "User: %.*s\r\n",
               t[i + 1].end - t[i + 1].start,
               json_string + t[i + 1].start);
      append_text(write_buffer, write_buffer_len, temp);
      i++;
    } else if (jsoneq(json_string, &t[i], "admin") == 0) {
      snprintf(temp, sizeof(temp), "Admin: %.*s\r\n",
               t[i + 1].end - t[i + 1].start,
               json_string + t[i + 1].start);
      append_text(write_buffer, write_buffer_len, temp);
      i++;
    } else if (jsoneq(json_string, &t[i], "uid") == 0) {
      snprintf(temp, sizeof(temp), "UID: %.*s\r\n",
               t[i + 1].end - t[i + 1].start,
               json_string + t[i + 1].start);
      append_text(write_buffer, write_buffer_len, temp);
      i++;
    } else if (jsoneq(json_string, &t[i], "groups") == 0) {
      int j;
      append_text(write_buffer, write_buffer_len, "Groups:\r\n");

      if (t[i + 1].type == JSMN_ARRAY) {
        if ((i + t[i + 1].size + 1) >= r) {
          return CLI_PARSE_ERROR_INVALID_OBJECT;
        }

        for (j = 0; j < t[i + 1].size; j++) {
          jsmntok_t *g = &t[i + j + 2];
          snprintf(temp, sizeof(temp), "  %.*s\r\n",
                   g->end - g->start, json_string + g->start);
          append_text(write_buffer, write_buffer_len, temp);
        }
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

  if (write_buffer == NULL || write_buffer_len == 0U) {
    return CLI_FAILURE;
  }

  memset(write_buffer, 0, write_buffer_len);

#if ARRAY_BASED_COMMAND_REGISTER
  size_t command_index;
  for (command_index = 0U; command_index < command_count; command_index++) {
    append_text(write_buffer, write_buffer_len,
                commands_array[command_index].help_string);
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

  return CLI_SUCCESS;
}

base_type cli_set_command(char *write_buffer, size_t write_buffer_len,
                          const char *command_string) {
  char name[CLI_PARAMETER_BUFFER_SIZE] = {0};
  char value[CLI_PARAMETER_BUFFER_SIZE] = {0};
  char protocol_value[CLI_PROTOCOL_VALUE_BUFFER_SIZE];
  GetSet_Packet packet;
  GetSet_Status status;

  if (write_buffer == NULL || write_buffer_len == 0U) {
    return CLI_FAILURE;
  }

  memset(write_buffer, 0, write_buffer_len);

  if (copy_parameter(name, sizeof(name), command_string, CLI_FIRST_PARAMETER) !=
      CLI_SUCCESS ||
      copy_parameter(value, sizeof(value), command_string, CLI_SECOND_PARAMETER) !=
      CLI_SUCCESS) {
    snprintf(write_buffer, write_buffer_len, "SET parameter is invalid\r\n");
    return CLI_FAILURE;
  }

  snprintf(protocol_value, sizeof(protocol_value), "%s=%s", name, value);

  status = getset_packet_init(&packet, GETSET_OPCODE_SET, GETSET_TYPE_STR,
                              (const uint8_t *)protocol_value,
                              (uint16_t)strlen(protocol_value));
  if (status != GETSET_STATUS_OK) {
    snprintf(write_buffer, write_buffer_len, "SET packet failed: %s\r\n",
             getset_status_to_string(status));
    return CLI_FAILURE;
  }

  append_text(write_buffer, write_buffer_len,
              "SET command mapped to GET-SET protocol\r\n");
  run_protocol_demo(write_buffer, write_buffer_len, &packet);
  return CLI_SUCCESS;
}

base_type cli_get_command(char *write_buffer, size_t write_buffer_len,
                          const char *command_string) {
  char name[CLI_PARAMETER_BUFFER_SIZE] = {0};
  GetSet_Packet packet;
  GetSet_Status status;

  if (write_buffer == NULL || write_buffer_len == 0U) {
    return CLI_FAILURE;
  }

  memset(write_buffer, 0, write_buffer_len);
  if (copy_parameter(name, sizeof(name), command_string, CLI_FIRST_PARAMETER) !=
      CLI_SUCCESS) {
    snprintf(write_buffer, write_buffer_len, "GET parameter is invalid\r\n");
    return CLI_FAILURE;
  }

  status = getset_packet_init(&packet, GETSET_OPCODE_GET, GETSET_TYPE_STR,
                              (const uint8_t *)name,
                              (uint16_t)strlen(name));
  if (status != GETSET_STATUS_OK) {
    snprintf(write_buffer, write_buffer_len, "GET packet failed: %s\r\n",
             getset_status_to_string(status));
    return CLI_FAILURE;
  }

  append_text(write_buffer, write_buffer_len,
              "GET command mapped to GET-SET protocol\r\n");
  run_protocol_demo(write_buffer, write_buffer_len, &packet);
  return CLI_SUCCESS;
}

base_type cli_list_command(char *write_buffer, size_t write_buffer_len,
                           const char *command_string) {
  (void)command_string;
  if (jsmn_list_output(write_buffer, write_buffer_len) < 0) {
    if (write_buffer != NULL && write_buffer_len > 0U) {
      snprintf(write_buffer, write_buffer_len, "LIST command failed\r\n");
    }
  }
  return CLI_SUCCESS;
}

/* ======================== Public CLI Functions ======================== */

static const CLI_Command_Definition *find_command_definition(const char *command_input) {
  const char *registered_command_string;
  base_type command_string_length;

  if (command_input == NULL) {
    return NULL;
  }

#if ARRAY_BASED_COMMAND_REGISTER
  size_t command_index;
  for (command_index = 0U; command_index < command_count; command_index++) {
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
    return CLI_FAILURE;
  }

#if ARRAY_BASED_COMMAND_REGISTER
  commands_array[command_count] = *command_to_register;
#else
  CLI_Definition_List_Item *new_item =
      (CLI_Definition_List_Item *)malloc(sizeof(CLI_Definition_List_Item));
  if (new_item == NULL) {
    return CLI_FAILURE;
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

  if (write_buffer == NULL || write_buffer_len == 0U || command_input == NULL) {
    return CLI_FAILURE;
  }

  memset(write_buffer, 0, write_buffer_len);

  if (command_def == NULL) {
    snprintf(write_buffer, write_buffer_len,
             "Command not recognised. Enter 'help' to view the list of commands.\r\n");
    return CLI_FAILURE;
  }

  if (command_def->expected_number_of_parameters >= 0) {
    if (get_number_of_parameters(command_input) !=
        command_def->expected_number_of_parameters) {
      snprintf(write_buffer, write_buffer_len,
               "Incorrect command parameter(s). Enter 'help' to view usage.\r\n");
      return CLI_FAILURE;
    }
  }

  return command_def->command_interpreter(write_buffer, write_buffer_len,
                                          command_input);
}

void cli_set_getset_context(GetSet_Context *context) {
  protocol_context = context;
}

/* ====================== END DATA PARSE HELPER ======================== */

#include "getset_protocol.h"
#include <string.h>

#define GETSET_ESCAPED_BYTE_SIZE (2U)
#define GETSET_SINGLE_BYTE_SIZE  (1U)

static GetSet_Status append_escaped_byte(uint8_t byte, uint8_t *frame_buffer,
                                         size_t frame_buffer_len,
                                         size_t *index) {
  if (frame_buffer == NULL || index == NULL) {
    return GETSET_STATUS_NULL_ARG;
  }

  if (byte == GETSET_SOF_BYTE || byte == GETSET_ESCAPE_BYTE) {
    if ((*index + GETSET_ESCAPED_BYTE_SIZE) > frame_buffer_len) {
      return GETSET_STATUS_BUFFER_TOO_SMALL;
    }

    frame_buffer[(*index)++] = GETSET_ESCAPE_BYTE;
    frame_buffer[(*index)++] = byte;
  } else {
    if ((*index + GETSET_SINGLE_BYTE_SIZE) > frame_buffer_len) {
      return GETSET_STATUS_BUFFER_TOO_SMALL;
    }

    frame_buffer[(*index)++] = byte;
  }

  return GETSET_STATUS_OK;
}

static GetSet_Status read_unescaped_byte(const uint8_t *frame_buffer,
                                         size_t frame_buffer_len,
                                         size_t *index, uint8_t *byte) {
  if (frame_buffer == NULL || index == NULL || byte == NULL) {
    return GETSET_STATUS_NULL_ARG;
  }

  if (*index >= frame_buffer_len) {
    return GETSET_STATUS_FRAME_TOO_SMALL;
  }

  *byte = frame_buffer[(*index)++];
  if (*byte == GETSET_ESCAPE_BYTE) {
    if (*index >= frame_buffer_len) {
      return GETSET_STATUS_ESCAPE_ERROR;
    }

    *byte = frame_buffer[(*index)++];
    if (*byte != GETSET_SOF_BYTE && *byte != GETSET_ESCAPE_BYTE) {
      return GETSET_STATUS_ESCAPE_ERROR;
    }
  }

  return GETSET_STATUS_OK;
}

void getset_context_init(GetSet_Context *context, void *user_context) {
  if (context != NULL) {
    memset(context, 0, sizeof(*context));
    context->user_context = user_context;
  }
}

GetSet_Status getset_register_handler(GetSet_Context *context, uint8_t opcode,
                                      GetSet_Handler handler) {
  if (context == NULL || handler == NULL) {
    return GETSET_STATUS_NULL_ARG;
  }

  if (context->handler_count >= GETSET_MAX_HANDLERS) {
    return GETSET_STATUS_BUFFER_TOO_SMALL;
  }

  context->handlers[context->handler_count].opcode = opcode;
  context->handlers[context->handler_count].handler = handler;
  context->handler_count++;

  return GETSET_STATUS_OK;
}

uint8_t getset_calculate_checksum(uint8_t opcode, uint8_t type,
                                  uint16_t length, const uint8_t *value) {
  uint8_t sum = 0U;
  uint16_t index;

  sum = (uint8_t)(sum + opcode);
  sum = (uint8_t)(sum + type);
  sum = (uint8_t)(sum + (uint8_t)(length >> 8));
  sum = (uint8_t)(sum + (uint8_t)(length & 0xFFU));

  if (value != NULL) {
    for (index = 0U; index < length; index++) {
      sum = (uint8_t)(sum + value[index]);
    }
  }

  return (uint8_t)(~sum);
}

GetSet_Status getset_packet_init(GetSet_Packet *packet, uint8_t opcode,
                                 uint8_t type, const uint8_t *value,
                                 uint16_t length) {
  if (packet == NULL || (value == NULL && length > 0U)) {
    return GETSET_STATUS_NULL_ARG;
  }

  if (length > GETSET_MAX_VALUE_SIZE) {
    return GETSET_STATUS_BAD_LENGTH;
  }

  memset(packet, 0, sizeof(*packet));
  packet->sof = GETSET_SOF_BYTE;
  packet->opcode = opcode;
  packet->type = type;
  packet->length = length;

  if (length > 0U) {
    memcpy(packet->value, value, length);
  }

  packet->checksum =
      getset_calculate_checksum(packet->opcode, packet->type, packet->length,
                                packet->value);

  return GETSET_STATUS_OK;
}

GetSet_Status getset_encode_packet(const GetSet_Packet *packet,
                                   uint8_t *frame_buffer,
                                   size_t frame_buffer_len,
                                   size_t *encoded_len) {
  size_t index = 0U;
  uint16_t value_index;
  GetSet_Status status;

  if (packet == NULL || frame_buffer == NULL || encoded_len == NULL) {
    return GETSET_STATUS_NULL_ARG;
  }

  if (packet->length > GETSET_MAX_VALUE_SIZE) {
    return GETSET_STATUS_BAD_LENGTH;
  }

  if (frame_buffer_len < GETSET_FRAME_OVERHEAD) {
    return GETSET_STATUS_BUFFER_TOO_SMALL;
  }

  frame_buffer[index++] = GETSET_SOF_BYTE;

  status = append_escaped_byte(packet->opcode, frame_buffer, frame_buffer_len,
                               &index);
  if (status != GETSET_STATUS_OK) {
    return status;
  }

  status = append_escaped_byte(packet->type, frame_buffer, frame_buffer_len,
                               &index);
  if (status != GETSET_STATUS_OK) {
    return status;
  }

  status = append_escaped_byte((uint8_t)(packet->length >> 8), frame_buffer,
                               frame_buffer_len, &index);
  if (status != GETSET_STATUS_OK) {
    return status;
  }

  status = append_escaped_byte((uint8_t)(packet->length & 0xFFU), frame_buffer,
                               frame_buffer_len, &index);
  if (status != GETSET_STATUS_OK) {
    return status;
  }

  for (value_index = 0U; value_index < packet->length; value_index++) {
    status = append_escaped_byte(packet->value[value_index], frame_buffer,
                                 frame_buffer_len, &index);
    if (status != GETSET_STATUS_OK) {
      return status;
    }
  }

  status = append_escaped_byte(packet->checksum, frame_buffer, frame_buffer_len,
                               &index);
  if (status != GETSET_STATUS_OK) {
    return status;
  }

  *encoded_len = index;
  return GETSET_STATUS_OK;
}

GetSet_Status getset_decode_frame(const uint8_t *frame_buffer,
                                  size_t frame_buffer_len,
                                  GetSet_Packet *packet) {
  size_t index = 0U;
  uint16_t value_index;
  uint8_t length_high;
  uint8_t length_low;
  uint8_t received_checksum;
  uint8_t calculated_checksum;
  GetSet_Status status;

  if (frame_buffer == NULL || packet == NULL) {
    return GETSET_STATUS_NULL_ARG;
  }

  if (frame_buffer_len < GETSET_FRAME_OVERHEAD) {
    return GETSET_STATUS_FRAME_TOO_SMALL;
  }

  memset(packet, 0, sizeof(*packet));

  if (frame_buffer[index++] != GETSET_SOF_BYTE) {
    return GETSET_STATUS_BAD_SOF;
  }

  packet->sof = GETSET_SOF_BYTE;

  status = read_unescaped_byte(frame_buffer, frame_buffer_len, &index,
                               &packet->opcode);
  if (status != GETSET_STATUS_OK) {
    return status;
  }

  status = read_unescaped_byte(frame_buffer, frame_buffer_len, &index,
                               &packet->type);
  if (status != GETSET_STATUS_OK) {
    return status;
  }

  status = read_unescaped_byte(frame_buffer, frame_buffer_len, &index,
                               &length_high);
  if (status != GETSET_STATUS_OK) {
    return status;
  }

  status = read_unescaped_byte(frame_buffer, frame_buffer_len, &index,
                               &length_low);
  if (status != GETSET_STATUS_OK) {
    return status;
  }

  packet->length = (uint16_t)(((uint16_t)length_high << 8) | length_low);
  if (packet->length > GETSET_MAX_VALUE_SIZE) {
    return GETSET_STATUS_BAD_LENGTH;
  }

  for (value_index = 0U; value_index < packet->length; value_index++) {
    status = read_unescaped_byte(frame_buffer, frame_buffer_len, &index,
                                 &packet->value[value_index]);
    if (status != GETSET_STATUS_OK) {
      return status;
    }
  }

  status = read_unescaped_byte(frame_buffer, frame_buffer_len, &index,
                               &received_checksum);
  if (status != GETSET_STATUS_OK) {
    return status;
  }

  calculated_checksum =
      getset_calculate_checksum(packet->opcode, packet->type, packet->length,
                                packet->value);
  if (calculated_checksum != received_checksum) {
    return GETSET_STATUS_CHECKSUM_ERROR;
  }

  packet->checksum = received_checksum;
  return GETSET_STATUS_OK;
}

GetSet_Status getset_dispatch_packet(GetSet_Context *context,
                                     const GetSet_Packet *packet,
                                     GetSet_Packet *response) {
  size_t index;

  if (context == NULL || packet == NULL || response == NULL) {
    return GETSET_STATUS_NULL_ARG;
  }

  for (index = 0U; index < context->handler_count; index++) {
    if (context->handlers[index].opcode == packet->opcode) {
      return context->handlers[index].handler(packet, response,
                                             context->user_context);
    }
  }

  return GETSET_STATUS_NO_HANDLER;
}

const char *getset_status_to_string(GetSet_Status status) {
  switch (status) {
  case GETSET_STATUS_OK:
    return "OK";
  case GETSET_STATUS_NULL_ARG:
    return "NULL_ARG";
  case GETSET_STATUS_BAD_SOF:
    return "BAD_SOF";
  case GETSET_STATUS_BAD_LENGTH:
    return "BAD_LENGTH";
  case GETSET_STATUS_FRAME_TOO_SMALL:
    return "FRAME_TOO_SMALL";
  case GETSET_STATUS_BUFFER_TOO_SMALL:
    return "BUFFER_TOO_SMALL";
  case GETSET_STATUS_CHECKSUM_ERROR:
    return "CHECKSUM_ERROR";
  case GETSET_STATUS_ESCAPE_ERROR:
    return "ESCAPE_ERROR";
  case GETSET_STATUS_NO_HANDLER:
    return "NO_HANDLER";
  default:
    return "UNKNOWN";
  }
}

const char *getset_opcode_to_string(uint8_t opcode) {
  switch (opcode) {
  case GETSET_OPCODE_GET:
    return "GET";
  case GETSET_OPCODE_SET:
    return "SET";
  case GETSET_OPCODE_ACK_DATA:
    return "ACK/DATA";
  default:
    return "UNKNOWN";
  }
}

const char *getset_type_to_string(uint8_t type) {
  switch (type) {
  case GETSET_TYPE_INT:
    return "INT";
  case GETSET_TYPE_CHAR:
    return "CHAR";
  case GETSET_TYPE_STR:
    return "STR";
  default:
    return "UNKNOWN";
  }
}

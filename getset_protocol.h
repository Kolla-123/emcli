#ifndef EMCLI_GETSET_PROTOCOL_H
#define EMCLI_GETSET_PROTOCOL_H

#include "config.h"
#include <stddef.h>
#include <stdint.h>

/* ====================== GET-SET PROTOCOL BEGIN ======================= */

/* Packet block values from the protocol specification. */
#define GETSET_SOF_BYTE    ((uint8_t)0x24) /* '$' */
#define GETSET_ESCAPE_BYTE ((uint8_t)0x2F) /* '/' */

typedef enum getset_opcode {
  GETSET_OPCODE_GET = 0x01,
  GETSET_OPCODE_SET = 0x02,
  GETSET_OPCODE_ACK_DATA = 0x03
} GetSet_Opcode;

typedef enum getset_type {
  GETSET_TYPE_INT = 0x01,
  GETSET_TYPE_CHAR = 0x02,
  GETSET_TYPE_STR = 0x03
} GetSet_Type;

typedef enum getset_status {
  GETSET_STATUS_OK = 0,
  GETSET_STATUS_NULL_ARG,
  GETSET_STATUS_BAD_SOF,
  GETSET_STATUS_BAD_LENGTH,
  GETSET_STATUS_FRAME_TOO_SMALL,
  GETSET_STATUS_BUFFER_TOO_SMALL,
  GETSET_STATUS_CHECKSUM_ERROR,
  GETSET_STATUS_ESCAPE_ERROR,
  GETSET_STATUS_NO_HANDLER
} GetSet_Status;

typedef struct getset_packet {
  uint8_t sof;       /* Block 0: Start of Frame, always 0x24 ('$'). */
  uint8_t opcode;    /* Block 1: GET, SET, or ACK/DATA. */
  uint8_t type;      /* Block 2: INT, CHAR, or STR. */
  uint16_t length;   /* Block 3: unescaped VALUE length, big-endian on wire. */
  uint8_t value[GETSET_MAX_VALUE_SIZE]; /* Block 4: Unescaped application data. */
  uint8_t checksum;  /* Block 5: 1's complement checksum. */
} GetSet_Packet;

typedef GetSet_Status (*GetSet_Handler)(const GetSet_Packet *packet,
                                        GetSet_Packet *response,
                                        void *user_context);

typedef struct getset_handler_definition {
  uint8_t opcode;
  GetSet_Handler handler;
} GetSet_Handler_Definition;

typedef struct getset_context {
  GetSet_Handler_Definition handlers[GETSET_MAX_HANDLERS];
  size_t handler_count;
  void *user_context;
} GetSet_Context;

void getset_context_init(GetSet_Context *context, void *user_context);
GetSet_Status getset_register_handler(GetSet_Context *context, uint8_t opcode,
                                       GetSet_Handler handler);

uint8_t getset_calculate_checksum(uint8_t opcode, uint8_t type,
                                  uint16_t length, const uint8_t *value);

GetSet_Status getset_packet_init(GetSet_Packet *packet, uint8_t opcode,
                                 uint8_t type, const uint8_t *value,
                                 uint16_t length);

GetSet_Status getset_encode_packet(const GetSet_Packet *packet,
                                   uint8_t *frame_buffer,
                                   size_t frame_buffer_len,
                                   size_t *encoded_len);

GetSet_Status getset_decode_frame(const uint8_t *frame_buffer,
                                  size_t frame_buffer_len,
                                  GetSet_Packet *packet);

GetSet_Status getset_dispatch_packet(GetSet_Context *context,
                                     const GetSet_Packet *packet,
                                     GetSet_Packet *response);

const char *getset_status_to_string(GetSet_Status status);
const char *getset_opcode_to_string(uint8_t opcode);
const char *getset_type_to_string(uint8_t type);

/* ======================= GET-SET PROTOCOL END ======================== */

#endif /* EMCLI_GETSET_PROTOCOL_H */

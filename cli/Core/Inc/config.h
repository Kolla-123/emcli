#ifndef EMCLI_CONFIG_H
#define EMCLI_CONFIG_H

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
#define TRUE  ((base_type)1)
#define PASS  ((base_type)1)
#define CLI_SUCCESS ((base_type)0)
#define CLI_FAILURE ((base_type)-1)

/* CLI Configuration */
#define CUSTOM_CLI_MAX_COMMANDS (10U)
#define CLI_WRITE_BUFFER_SIZE   (512U)

/* GET-SET Protocol Configuration */
#define GETSET_MAX_VALUE_SIZE (128U)
#define GETSET_FRAME_OVERHEAD (6U)
#define GETSET_MAX_FRAME_SIZE ((GETSET_MAX_VALUE_SIZE * 2U) + GETSET_FRAME_OVERHEAD)
#define GETSET_MAX_HANDLERS   (8U)

#endif /* EMCLI_CONFIG_H */

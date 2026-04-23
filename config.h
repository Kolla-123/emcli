#ifndef CONFIG_H
#define CONFIG_H

#include <stdint.h>

/* Choose the registration method here:
 * 1 = array based registry
 * 0 = linked list based registry
 */
#define ARRAY_BASED_COMMAND_REGISTER 1

/* Basic types */
typedef int base_type;
#define FALSE ((base_type)0)
#define PASS  ((base_type)1)

/* CLI Configuration */
#define CUSTOM_CLI_MAX_COMMANDS 10
#define CLI_WRITE_BUFFER_SIZE 512

#endif /* CONFIG_H */

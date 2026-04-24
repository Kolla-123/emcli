# EMCLI - Embedded Command Line Interface

A **lightweight, production-ready CLI system** designed for embedded systems and resource-constrained environments. Features integrated JSON parsing, modular command registration, and zero external dependencies.

##  Overview

EMCLI (Embedded CLI) is a compact command-line interface framework that allows you to build extensible command-based applications with minimal memory footprint. It's perfect for:

- **Embedded Systems**: Firmware with limited resources
- **IoT Devices**: Connected devices requiring remote management
- **Microcontroller Projects**: Interfacing via serial/USB
- **System Management Tools**: Configuration and monitoring utilities
- **Educational Projects**: Learning systems programming and C

### Key Capabilities

 **Command Registration System** - Dynamically register commands at runtime  
 **JSON Data Parsing** - Built-in JSMN JSON parser for structured data  
 **Configurable Storage** - Choose between array or linked-list based registries  
 **Parameter Validation** - Automatic command parameter checking  
 **Safe String Operations** - Bounds-checking on all string operations  
 **Zero Dependencies** - Pure C99, no external libraries required  

##  Project Structure

```
emcli/
├── config.h           - Shared configuration and type definitions
├── jsmn.h             - JSMN JSON parser header (public API)
├── jsmn.c             - JSMN JSON parser implementation
├── cli.h              - CLI system header (public API)
├── cli.c              - CLI system implementation & built-in commands
├── main.c             - Application entry point with demo
├── Makefile           - Build configuration
├── README.md          - This file
└── emcli.c.c          - Original single-file version (reference)
```

## 🔧 What Does This Code Do?

### Core Architecture

**EMCLI** separates concerns into three layers:

#### 1. **Configuration Layer** (`config.h`)
Centralized settings for the entire application:
- Type definitions for consistent codebase
- Memory constraints (max commands, buffer sizes)
- Registry method selection (array vs. linked list)

#### 2. **JSON Parser Layer** (`jsmn.h / jsmn.c`)
A self-contained, lightweight JSON tokenizer:
- Parses JSON strings into token arrays
- Supports nested objects and arrays
- Handles escape sequences and Unicode escapes
- Returns meaningful error codes
- **Why embedded?** IoT/embedded devices often need to parse configuration or sensor data in JSON format without heavy dependencies

#### 3. **CLI Framework** (`cli.h / cli.c`)
The command execution engine:
- **Command Registry**: Maintains a list of available commands
- **Command Parser**: Extracts command name and parameters
- **Parameter Validator**: Ensures correct argument count
- **Command Dispatcher**: Routes input to appropriate handler
- **Built-in Commands**:
  - `help` - Lists all available commands
  - `set <parameter> <value>` - Stores configuration
  - `get <parameter>` - Retrieves configuration
  - `list` - Displays JSON-formatted system data

#### 4. **Application Layer** (`main.c`)
Demonstrates the framework in use:
- Registers 4 built-in commands
- Executes test commands
- Shows output formatting

### Example: How a User Command Flows Through the System

```
User Input: "set temperature 25"
    ↓
cli_process_command() analyzes input
    ↓
find_command_definition() locates "set" command
    ↓
get_number_of_parameters() validates 2 parameters provided
    ↓
cli_set_command() handler executes
    ↓
Output: "SET command executed\r\nParameter: temperature\r\nValue: 25\r\n"
```

## 📊 Technical Details

### Registry Methods

**Array-Based (Default, `ARRAY_BASED_COMMAND_REGISTER = 1`):**
-  Fast O(n) lookup
-  Fixed memory allocation
-  Cache-friendly
-  Limited to `CUSTOM_CLI_MAX_COMMANDS`

**Linked-List Based (`ARRAY_BASED_COMMAND_REGISTER = 0`):**
-  Dynamic command addition
-  No hard limit on command count
-  Slower O(n) lookup
-  Additional memory per command

### Memory Layout Example

```
Default Configuration:
- CLI Buffer: 512 bytes
- Max Commands: 10
- Per-Command Overhead: ~48 bytes (array) or 56+ bytes (linked list)
- Total Memory: ~1-2 KB for a typical configuration
```

### JSON Parsing Example

```c
Input JSON: {"user": "Robin", "uid": 1000, "groups": ["users", "wheel"]}

Tokens Generated:
[0] OBJECT   {start: 0, end: 78}           - Root object
[1] STRING   {start: 2, end: 6}     "user" - Key
[2] STRING   {start: 10, end: 15}   "Robin" - Value
[3] STRING   {start: 20, end: 23}   "uid"  - Key
[4] PRIMITIVE {start: 26, end: 30}  "1000" - Value
[5] STRING   {start: 35, end: 41}   "groups" - Key
[6] ARRAY    {start: 43, end: 77}         - Array value
[7] STRING   {start: 45, end: 50}   "users" - Array element
[8] STRING   {start: 53, end: 58}   "wheel" - Array element
```

##  Building & Running

### Using Makefile (Recommended)

```bash
# Build the project
make

# Build and run
make run

# Clean build artifacts
make clean
```

### Using GCC Directly

```bash
# Compile
gcc -Wall -Wextra -std=c99 -o emcli main.c cli.c jsmn.c

# Run
./emcli
```

### Expected Output

```
Registry type: ARRAY BASED

help: Lists all registered commands
set <parameter> <value>: Sets a value in the system
get <parameter>: Gets a value from the system
list: Shows user/admin/uid/groups values

SET command executed
Parameter: temp
Value: 25

GET command executed
Requested parameter: temp

LIST command executed
User: Robin
Admin: false
UID: 1000
Groups:
  users
  wheel
  text
  video

Command not recognised. Enter 'help' to view the list of commands.
```

##  Usage Examples

### Adding a Custom Command

```c
#include "cli.h"

/* Define your command handler */
base_type temperature_command(char *write_buffer, size_t write_buffer_len,
                              const char *command_string) {
  const char *parameter;
  base_type length = 0;
  char value[32] = {0};
  
  parameter = cli_get_parameter(command_string, 1, &length);
  if (parameter && length < sizeof(value)) {
    strncpy(value, parameter, length);
    value[length] = '\0';
  }
  
  snprintf(write_buffer, write_buffer_len, 
           "Temperature set to: %s°C\r\n", value);
  return FALSE;
}

/* Register it in main() */
const CLI_Command_Definition temp_cmd = {
  "temp",
  "temp <celsius>: Set temperature value",
  temperature_command,
  1,  /* expects 1 parameter */
  FALSE
};

cli_register_command(&temp_cmd);
```

### Running Commands Programmatically

```c
char output[512];
cli_process_command("temp 35", output, sizeof(output));
printf("%s", output);  /* "Temperature set to: 35°C\r\n" */
```

## ⚙️ Configuration

Edit `config.h` to customize behavior:

| Setting | Default | Purpose |
|---------|---------|---------|
| `ARRAY_BASED_COMMAND_REGISTER` | 1 | 1 = array, 0 = linked list |
| `CUSTOM_CLI_MAX_COMMANDS` | 10 | Maximum commands allowed |
| `CLI_WRITE_BUFFER_SIZE` | 512 | Output buffer size |
| `FALSE` | 0 | False value constant |
| `PASS` | 1 | Success value constant |

## 📈 Performance Characteristics

| Operation | Time | Space |
|-----------|------|-------|
| Command Lookup | O(n) | O(1) |
| Parameter Parsing | O(m) | O(1) |
| JSON Parse | O(n) | O(n) |
| Command Execution | O(1) | Variable |

*Where n = number of commands, m = command string length*

##  Security Considerations

-  **Buffer Overflow Protection**: All string operations use bounded functions
-  **Parameter Validation**: Commands validate argument count before execution
-  **No Dynamic Code Execution**: Commands are pre-registered only
-  **JSON Parsing**: Validates JSON structure but doesn't sanitize values
-  **Command Injection**: Careful when accepting raw user input

##  Use Cases

### 1. Embedded Device Configuration
```
device> set wifi_ssid "MyNetwork"
device> set wifi_pass "***"
device> list
```

### 2. Sensor Data Query
```
sensor> get temperature
Requested parameter: temperature
sensor> get humidity
Requested parameter: humidity
```

### 3. System Monitoring
```
monitor> list
LIST command executed
User: admin
UID: 0
Groups: root, wheel, admins
```

##  Debugging

Enable verbose output by modifying `main.c`:

```c
#ifdef DEBUG
printf("Command: %s\n", command_input);
printf("Tokens: %d\n", r);
#endif
```

##  Dependencies

**None!** Pure C99 with only standard library:
- `stdio.h` - I/O operations
- `stdlib.h` - Memory management
- `string.h` - String operations
- `stddef.h` - Standard definitions
- `stdint.h` - Integer types

##  License

This project is provided as-is for educational and commercial use.

##  Contributing

Suggestions for improvement:
- Command history/recall
- Tab completion support
- Scripting capabilities
- Extended JSON features
- Command aliasing

##  Support

For issues or questions, refer to code comments and header documentation in each file.

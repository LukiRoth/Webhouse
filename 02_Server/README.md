# Webhouse Server Application

## Overview
This server application forms the core of the Raspberry-Pi Webhouse project. It handles various utilities, including web communication, JSON data processing, and controlling the Webhouse environment.

## File Descriptions

1. **`base64.c`**: Implements Base64 encoding and decoding as defined in RFC1341, essential for data communication in web contexts.

2. **`base64.h`**: Header file for the Base64 implementation, defining necessary structures and functions.

3. **`data.json`**: Stores the data for the Webhouse.

4. **`handshake.c`**: Manages the WebSocket handshake process, crucial for establishing WebSocket connections.

5. **`jansson.h`**: Header file for the Jansson library, facilitating JSON data creation, manipulation, and querying.

6. **`main.c`**: The main entry point of the application. It initializes the Webhouse utilities and contains the main application loop.

7. **`Makefile`**: The Makefile for the application, defining the build process.

8. **`sha1.c`**:  Implements the SHA-1 hashing algorithm, used for generating secure message digests.

9. **`sha1.h`**: Header file for the SHA-1 implementation, defining necessary structures and functions.

10. **`Template`**: This is the executable file that is generated when the application is built. It is the application itself.

11. **`Webhouse.c`**: Central to the application, this file contains functions for controlling various aspects of the Webhouse, like lighting, heating, and monitoring.

12. **`Webhouse.h`**: Header file declaring the functions in Webhouse.c for external usage within the application.


## Compilation
To compile the server application, navigate to the 02_Server directory and run the following command:
> make

This will generate the executable file `Template` in the same directory.

## Running the Server
To run the server application, navigate to the 02_Server directory and run the following command:
> sudo ./Template

## Additional Notes
- Ensure all dependencies are installed before compiling, especially the Jansson library for JSON processing.
- The server uses port 8000 by default, as defined in the macros.
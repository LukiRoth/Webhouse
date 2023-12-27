Create a README.md file in your 02_Server directory to document the purpose of the server, how to compile and run it, and any other important details.

Summary of each file and its purpose:

1. **`base64.c`【39†source】**: Implements Base64 encoding and decoding as defined in RFC1341. This file contains functions for encoding and decoding data to and from Base64 format, which is often used in data communication, especially in web contexts.

2. **`handshake.c`【43†source】**: Contains functions related to the WebSocket handshake process. It includes a function to generate a handshake response from a given request and functions for encoding and decoding WebSocket messages.

3. **`jansson.h`【47†source】**: The header file for the Jansson library, which is used for handling JSON data. It defines functions and types for creating, manipulating, and querying JSON data, an important aspect for web-based communication and data handling.

4. **`main.c`【51†source】**: The main entry point for your application. It includes the initialization of the webhouse, handling of signals, and the main loop for the application.

5. **`sha1.c`【57†source】**: Implements the SHA-1 hashing algorithm, which is used for generating a message digest. This file contains functions for computing SHA-1 hashes, which are useful for data integrity checks and in various security protocols.

6. **`sha1.h`【61†source】**: The header file for the SHA-1 hashing implementation. It defines the SHA1Context structure and function prototypes for SHA-1 hashing operations.

7. **`Webhouse.c`【65†source】**: Contains functions for controlling various aspects of the webhouse, such as turning on/off the TV, dimming lamps, controlling LEDs, managing the heater, and retrieving temperature and alarm states. This file is central to controlling the I/O of your embedded system.

8. **`Webhouse.h`【69†source】**: The header file for `Webhouse.c`. It declares the functions provided in `Webhouse.c` for external use, ensuring that these functions can be called from other parts of your application.
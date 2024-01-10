/*******************************************************************************
 * @file       main.c
 *******************************************************************************
 *
 * @brief      Main application for the Raspberry-Pi Webhouse project.
 *             Initializes and runs the Webhouse utilities.
 * 
 * @details    This file contains the startup logic and main function 
 *             for the Webhouse project. It was initially templated by Elham 
 *             Firouzi and subsequently developed and finalized by Lukas Roth 
 *             (rothl18) and Patrice Begert (begp1).
 * 
 * @author     Elham Firouzi (Initial template)
 *             Lukas Roth (rothl18) - Development and Completion
 *             Patrice Begert (begp1) - Development and Completion
 *
 * @version    1.0
 * @date       November 2021
 *
 * @remark     Last Modification
 *             \li Elham Firouzi, November 2021, Template Creation
 *             \li Lukas Roth (rothl18), Januar 2024, Project Completion
 *             \li Patrice Begert (begp1), Januar 2024, Project Completion
 *
/******************************************************************************
 *
 *  Functions  global:
 *              main
 *
 *  Functions  local:
 *              shutdownHook
 *              InitWebhouseUtilities
 *              InitSocket
 *              HandleHandshake
 *              CheckAndHandleCloseFrame
 *              DecodeMessage
 *              processCommand
 *              SaveData
 *              LoadData
 * 
 *  Authors:   Elham Firouzi (Initial template creation)
 *             Lukas Roth (rothl18) - Development and Finalization
 *             Patrice Begert (begp1) - Development and Finalization
 *
 ******************************************************************************/

// type definitions
typedef int int32_t;

//----- Header-Files -----------------------------------------------------------
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <signal.h>
#include <pthread.h>
#include <math.h>
#include <errno.h>

#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/socketvar.h>

#include "jansson.h"
#include "Webhouse.h"
#include "handshake.h"

//----- Macros -----------------------------------------------------------------
#define TRUE 1
#define FALSE 0
#define SERVER_PORT 8000		// Port number for the server
#define BACKLOG 5 				// Number of allowed connections
#define RX_BUFFER_SIZE 1024   	// Buffer size for receiving data, maybe 1024

//----- Function prototypes ----------------------------------------------------
static int SaveData(void);
static int LoadData(void);
static void InitWebhouseUtilities(void);
static int InitSocket(void);
static int HandleHandshake(int com_sock_id, char* rxBuf);
static int CheckAndHandleCloseFrame(int com_sock_id, char* rxBuf, int rx_data_len);
static void DecodeMessage(int com_sock_id, char* rxBuf, int rx_data_len);
static int processCommand(char*, char*);
static void shutdownHook (int32_t);

//----- Global variables -------------------------------------------------------
static volatile int eShutdown = FALSE;
static int dutyCycleLed = 25;
static int stateLampFloor = 0;
static int stateLampCeiling = 0;

/*******************************************************************************
 * @brief    Main function of the Webhouse project.
 *
 * @param    argc  Number of command line arguments.
 * @param    argv  Array of command line arguments.
 * @return   EXIT_SUCCESS if successful, EXIT_FAILURE otherwise.
 ******************************************************************************/
int main(int argc, char **argv) {
	// Variables
	int server_sock_id = -1;					// Socket ID for the server
	int com_sock_id = -1;						// Socket ID for the communication
	struct sockaddr_in client;					// Client address
	int addrlen = sizeof (struct sockaddr_in);	// Length of the client address
	int com_addrlen = sizeof(client);			// Length of the communication address
	
	// Register shutdown hook
	signal(SIGINT, shutdownHook);

	// Initialize Webhouse
	printf("Init Webhouse\n");
	fflush(stdout);
	initWebhouse();

    // Init all Webhouse utilities
    InitWebhouseUtilities();

	// Initialize Socket
	printf("Init Socket\n");
	fflush(stdout);
	server_sock_id = InitSocket();

	if(server_sock_id < 0) {
		perror("Socket initialization failed");
		return EXIT_FAILURE;
	}

	// Main Loop
	while (eShutdown == FALSE) {
		if(com_sock_id < 0){
            // Accept a new connection
			com_sock_id = accept(server_sock_id, (struct sockaddr *)&client, &com_addrlen);
			if (com_sock_id < 0) {
				perror("accept failed");
				continue;
			}
			else {
				printf("Connection established\n");
				fflush(stdout);
			}
		}

        // Receive data
		char rxBuf[RX_BUFFER_SIZE];
		int rx_data_len = recv (com_sock_id, (void *)rxBuf, RX_BUFFER_SIZE, MSG_DONTWAIT);

		// If a new WebSocket message have been received
		if (rx_data_len > 0) {
            //printf("Received %d bytes\n", rx_data_len);
			rxBuf[rx_data_len] = '\0';

			// Is the message a handshake request
			if(HandleHandshake(com_sock_id, rxBuf) == TRUE) {	
                printf("Handshake handled\n");			
				continue;
			}

			// Is the message a close frame
			if(!CheckAndHandleCloseFrame(com_sock_id, rxBuf, rx_data_len)) {
                printf("Close frame received\n");
				com_sock_id = -1;	 	// Update as the socket is closed
				continue;
			}

			// Decode the message, execute the command and send the response
			DecodeMessage(com_sock_id, rxBuf, rx_data_len);

		}
		else if (rx_data_len == 0) {
            // Connection closed by the client
			close(com_sock_id);
			com_sock_id = -1;
			printf("Connection closed\n");
			fflush(stdout);
		}
		else {
			// Check if the error is EAGAIN or EWOULDBLOCK (which means "try again")
			if (errno == EAGAIN || errno == EWOULDBLOCK) {
				// No data available right now, try again later
			} else {
				perror("Receive failed");
			}
		}

		sleep(1);
	}

    // Save the current state of the Webhouse utilities
    SaveData();
	
    // Close the Webhouse
	closeWebhouse();
	printf ("Close Webhouse\n");
	fflush (stdout);
	close(server_sock_id);

    // Exit the program successfully
	return EXIT_SUCCESS;
}

/*******************************************************************************
 * @brief    Handles registered signals (SIGTERM, SIGINT) for graceful shutdown.
 *
 *           This function sets a flag to indicate that a shutdown signal has
 *           been received, allowing the program to terminate gracefully.
 *
 * @param    sig   The incoming signal.
 ******************************************************************************/
static void shutdownHook(int32_t sig) {
    printf("Signal %d received, initiating shutdown...\n", sig);
    fflush(stdout);
    eShutdown = TRUE;
}

/*******************************************************************************
 * @brief    Initializes all Webhouse utilities.
 *
 *           Initializes the utilities to ensure consistent starting states.
 *           This is necessary because the initial state reading of utilities 
 *           might sometimes return incorrect values. To handle this, the 
 *           function attempts to load the previous state from a file. If this 
 *           fails, it sets default states for all utilities.
 *
 * @return   void
 ******************************************************************************/
static void InitWebhouseUtilities(void)
{
    // Attempt to load the previous state of utilities
    if (!LoadData()) {
        // If loading fails, set default states
        turnTVOff();
        turnHeatOff();

        // Set the lamp states based on their current state and duty cycle
        if (stateLampFloor)
            dimSLamp(dutyCycleLed);
        else
            dimSLamp(0);

        if (stateLampCeiling)
            dimRLamp(dutyCycleLed);
        else
            dimRLamp(0);
    }
}

/*******************************************************************************
 * @brief    Saves the current state of the Webhouse utilities to a file.
 *           It writes the states of TV, heater, floor lamp, ceiling lamp,
 *           and LED PWM duty cycle to 'data.json' in JSON format.
 *
 * @return   TRUE if successful, FALSE otherwise.
 ******************************************************************************/
static int SaveData(void)
{
    // Define the filename where the data will be saved
    const char* filename = "data.json";

    // Create a new JSON object
    json_t *root = json_object();
    
    // Set the values for each utility in the JSON object
    json_object_set_new(root, "tv", json_integer(getTVState()));
    json_object_set_new(root, "heater", json_integer(getHeatState()));
    json_object_set_new(root, "lamp_floor", json_integer(stateLampFloor));
    json_object_set_new(root, "lamp_ceil", json_integer(stateLampCeiling));
    json_object_set_new(root, "led_pwm", json_integer(dutyCycleLed));

    // Convert the JSON object to a string
    char *res_str = json_dumps(root, JSON_COMPACT);
    if(!res_str){
        // Handle error if conversion fails
        fprintf(stderr, "Error converting JSON to string\n");
        fflush(stderr);
        json_decref(root);      // Release the JSON object
        return FALSE;
    }

    // Open the file for writing
    FILE *fp = fopen(filename, "w");
    if(!fp){
        // Handle error if file opening fails
        fprintf(stderr, "Error opening file: %s\n", filename);
        fflush(stderr);
        free(res_str);          // Free the JSON string
        json_decref(root);      // Release the JSON object
        return FALSE;
    }

    // Write the JSON string to the file
    if(fprintf(fp, "%s", res_str) < 0) {
        // Handle error if writing fails
        fprintf(stderr, "Error writing to file: %s\n", filename);
    } else {
        // Confirm successful data saving
        printf("Data successfully saved to %s\n", filename);
    }

    // Close the file and release resources
    fclose(fp);
    free(res_str);              // Free the JSON string
    json_decref(root);          // Release the JSON object

    return TRUE;
}

/*******************************************************************************
 * @brief    Lädt den aktuellen Zustand der Webhouse-Umgebung aus einer Datei.
 *           Gibt TRUE zurück, wenn das Laden erfolgreich war, sonst FALSE.
 *
 * @return   TRUE if successful, FALSE otherwise.
 ******************************************************************************/
static int LoadData(void)
{
    const char* filename = "data.json";

    // Datei für das Lesen öffnen
    FILE *fp = fopen(filename, "r");
    if (!fp) {
        fprintf(stderr, "Error opening file: %s\n", filename);
        fflush(stderr);
        return FALSE;
    }

    // Datei in einen String einlesen
    char *buffer = NULL;
    long length;
    fseek(fp, 0, SEEK_END);
    length = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    buffer = malloc(length);
    if (buffer) {
        fread(buffer, 1, length, fp);
    }
    fclose(fp);

    if (!buffer) {
        fprintf(stderr, "Error reading file: %s\n", filename);
        fflush(stderr);
        return FALSE;
    }

    // JSON-String parsen
    json_error_t error;
    json_t *root = json_loads(buffer, 0, &error);
    free(buffer);

    if (!root) {
        fprintf(stderr, "Error parsing JSON data: %s\n", error.text);
        fflush(stderr);
        return FALSE;
    }

    // Zustände aus dem JSON-Objekt extrahieren
    json_t *tv = json_object_get(root, "tv");
    if(json_is_integer(tv)){
        if(json_integer_value(tv))
            turnTVOn();
        else
            turnTVOff();
    }

    json_t *heater = json_object_get(root, "heater");
    if(json_is_integer(heater)){
        if(json_integer_value(heater))
            turnHeatOn();
        else
            turnHeatOff();
    }

    json_t *led_pwm = json_object_get(root, "led_pwm");
    if(json_is_integer(led_pwm)){
        dutyCycleLed = (int)json_integer_value(led_pwm);
    }

    json_t *lamp_floor = json_object_get(root, "lamp_floor");
    if(json_is_integer(lamp_floor)){
        stateLampFloor = (int)json_integer_value(lamp_floor);
        
        if(json_integer_value(lamp_floor)){
            dimSLamp((uint16_t)dutyCycleLed);
        } else{
            dimSLamp(0);
        }
    }

    json_t *lamp_ceil = json_object_get(root, "lamp_ceil");
    if(json_is_integer(lamp_ceil)){
        stateLampCeiling = (int)json_integer_value(lamp_ceil);
        
        if(json_integer_value(lamp_ceil)){
            dimRLamp((uint16_t)dutyCycleLed);
        } else{
            dimRLamp(0);
        }
    }

    // JSON-Objekt freigeben
    json_decref(root);

    printf("Data loaded successfully from %s\n", filename);
    return TRUE;
}

/*******************************************************************************
 * @brief    Creates and initializes a server socket with specified settings.
 *           Binds the socket to the server address and listens for incoming
 *           connections.
 *
 * @return   The socket ID if successful, -1 on failure.
 ******************************************************************************/
static int InitSocket(void) 
{
    // Create a socket
    int server_sock_id = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (server_sock_id < 0) {
        perror("Socket creation failed");
        return -1;
    }
    printf("Socket created successfully. Server socket ID: %d\n", server_sock_id);
    fflush(stdout);

    // Set server address
    struct sockaddr_in server;
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = htonl(INADDR_ANY);
    server.sin_port = htons(SERVER_PORT);

    // Bind the socket
    int bind_status = bind(server_sock_id, (struct sockaddr *)&server, sizeof(server));
    if (bind_status < 0) {
        perror("Socket bind failed");
        close(server_sock_id);
        return -1;
    }
    printf("Socket binded successfully\n");
    fflush(stdout);

    // Listen on the socket
    int listen_status = listen(server_sock_id, BACKLOG);
    if (listen_status < 0) {
        perror("Listen failed");
        close(server_sock_id);
        return -1;
    }
    printf("Listen succeeded\n");
    fflush(stdout);

    return server_sock_id;
}

/*******************************************************************************
 * @brief    Handles the WebSocket handshake if the incoming message is a GET request.
 *           Creates and sends a handshake response back.
 *
 * @param    com_sock_id  Socket ID for communication.
 * @param    rxBuf        Buffer containing the received message.
 * @return   TRUE if a handshake was handled, FALSE otherwise.
 ******************************************************************************/
static int HandleHandshake(int com_sock_id, char* rxBuf)
{
	if (strncmp(rxBuf, "GET", 3) == 0) {
		// create the handshake response and send it back
		char response[WS_HS_ACCLEN];
		get_handshake_response(rxBuf, response);
		send(com_sock_id, (void *)response, strlen(response), 0);
		return TRUE;
	}

	return FALSE;
}

/*******************************************************************************
 * @brief    Checks if the received WebSocket message is a close frame.
 *           Handles the close frame by sending a response and closing the socket.
 *
 * @param    com_sock_id  Socket ID for communication.
 * @param    rxBuf        Buffer containing the received message.
 * @param    rx_data_len  Length of the received message.
 * @return   TRUE if not a close frame, FALSE if it is a close frame.
 ******************************************************************************/
static int CheckAndHandleCloseFrame(int com_sock_id, char* rxBuf, int rx_data_len) 
{
    uint8_t opcode = rxBuf[0] & 0x0F;
    if (opcode == 0x8) { // Close frame detected
        char closeFrame[2] = { 0x88, 0x00 }; // Simple close frame
        send(com_sock_id, closeFrame, sizeof(closeFrame), 0);
        close(com_sock_id);
        return FALSE;
    }
	
    return TRUE;
}

/*******************************************************************************
 * @brief    Decodes the received WebSocket message and executes the command.
 *           Sends the response back to the client.
 *
 * @param    com_sock_id  Socket ID for communication.
 * @param    rxBuf        Buffer containing the received message.
 * @param    rx_data_len  Length of the received message.
 * @return   void
 ******************************************************************************/
static void DecodeMessage(int com_sock_id, char* rxBuf, int rx_data_len)
{
    // Decode the incoming request
	char command[rx_data_len];
	if(decode_incoming_request(rxBuf, command, rx_data_len) == -1){
        printf("Error decoding incoming request\n");
        fflush(stdout);
        return;
    }

    // Terminate the command string
	command[strlen(command)] = '\0';

    // Process the command and create a response
    char response[RX_BUFFER_SIZE];
	if(!processCommand(command, response)){
        printf("Error processing command, response: %s \n", response);
        fflush(stdout);
    }

    // Encode the response
	char codedResponse[strlen(response)+2];
	code_outgoing_response (response, codedResponse);

    // Send the response
	send(com_sock_id, (void *)codedResponse, strlen(codedResponse), 0);
}

/*******************************************************************************
 * @brief    Processes the received command and creates a response.
 *
 * @param    command   The received command.
 * @param    response  The response to be sent back.
 * @return   TRUE if successful, FALSE otherwise.
 ******************************************************************************/
static int processCommand(char* command, char* response) 
{
    // Print the received command
    printf("[%d] Command: {%s}\n", __LINE__, command);
    fflush(stdout);

    // Parse the command as JSON
    json_error_t error;
    json_t *root = json_loads(command, 0, &error);

    if (!root) {
        // Error handling
        fprintf(stderr, "error: on line %d: %s\n", error.line, error.text);
        fprintf(stderr, "Command: %s\n", command);
        sprintf(response, "{\"type\":\"CommandResponse\",\"action\":\"-\",\"status\":\"Error\",\"message\":\"Invalid JSON\"}", command);
        return FALSE;
    }

    json_t *action = json_object_get(root, "action");

    if (!json_is_string(action)) {
        sprintf(response, "{\"type\":\"CommandResponse\",\"action\":\"-\",\"status\":\"Error\",\"message\":\"Missing or invalid action\"}", command);
        printf("[%d] Response: {%s}\n", __LINE__, response);
        return FALSE;
    }

    // Handle different actions
    const char *action_str = json_string_value(action);

    if (strcmp(action_str, "read") == 0) {
        json_t *utilities = json_object_get(root, "utilities");
        
        if (!json_is_array(utilities)) {
            sprintf(response, "{\"type\":\"CommandResponse\",\"action\":\"read\",\"status\":\"Error\",\"message\":\"Missing or invalid utilities\"}");
            return FALSE;
        }

        // Create JSON response
        json_t *res = json_object();
        json_object_set_new(res, "type", json_string("DataResponse"));
        json_object_set_new(res, "action", json_string("read"));

        json_t *data = json_object();

        // Iterate through the utilities array
        size_t index;
        json_t *value;
        json_array_foreach(utilities, index, value) {
            if (json_is_string(value)) {
                const char *utility_str = json_string_value(value);

                // Handle different utilities
                if (strcmp(utility_str, "tv") == 0) {
                    json_object_set_new(data, "tv", json_integer(getTVState()));
                } else if (strcmp(utility_str, "heater") == 0) {
                    json_object_set_new(data, "heater", json_integer(getHeatState()));
                } else if (strcmp(utility_str, "temperature") == 0) {
                    json_object_set_new(data, "temperature", json_real(getTemp()));
                } else if (strcmp(utility_str, "alarm") == 0) {
                    json_object_set_new(data, "alarm", json_integer(getAlarmState()));
                } else if (strcmp(utility_str, "lamp_floor") == 0) {
                    json_object_set_new(data, "lamp_floor", json_integer(stateLampFloor));
                } else if (strcmp(utility_str, "lamp_ceil") == 0) {
                    json_object_set_new(data, "lamp_ceil", json_integer(stateLampCeiling));
                } else if (strcmp(utility_str, "led_pwm") == 0) {
                    json_object_set_new(data, "led_pwm", json_integer(dutyCycleLed));
                }
            }
        }

        json_object_set_new(res, "data", data);

        // Convert JSON response to string
        char *res_str = json_dumps(res, JSON_COMPACT);
        if (res_str) {
            strncpy(response, res_str, RX_BUFFER_SIZE);
            free(res_str);
        } else {
            sprintf(response, "{\"type\":\"CommandResponse\",\"action\":\"read\",\"status\":\"Error\",\"message\":\"JSON conversion failed\"}");
            return FALSE;
        }
        json_decref(res);
    } // End of read
    else if (strcmp(action_str, "write") == 0) {
        // Example: {{"action":"write","utility":"led_pwm","value":32}}
        json_t *utility = json_object_get(root, "utility");
        json_t *value = json_object_get(root, "value");

        if (!json_is_string(utility) || !json_is_integer(value)) {
            sprintf(response, "{\"type\":\"CommandResponse\",\"action\":\"write\",\"status\":\"Error\",\"message\":\"Missing or invalid utility or value\"}");
            return FALSE;
        }

        const char *utility_str = json_string_value(utility);

        if (strcmp(utility_str, "led_pwm") == 0) {
            dutyCycleLed = (int)json_integer_value(value);

            // Check if the duty cycle is in the valid range
            if(dutyCycleLed < 0){  
                dutyCycleLed = 0;
            }
            if(dutyCycleLed > 100){
                dutyCycleLed = 100;
            }

            // Set the duty cycle for the lamps based on their current state
            if(stateLampFloor){
                dimSLamp((uint16_t)dutyCycleLed);
            }
            if(stateLampCeiling){
                dimRLamp((uint16_t)dutyCycleLed);
            }
            
            sprintf(response, "{\"type\":\"CommandResponse\",\"action\":\"write\",\"status\":\"Success\",\"message\":\"RLamp set to %d\"}", dutyCycleLed);
        }
        else{
            sprintf(response, "{\"type\":\"CommandResponse\",\"action\":\"write\",\"status\":\"Error\",\"message\":\"Invalid utility: %s\"}", utility_str);
            return FALSE;
        }
    }
    else if (strcmp(action_str, "toggle") == 0) {
        // Example: {{"action":"toggle","utility":"lamp1"}}
        json_t *utility = json_object_get(root, "utility");

        if (!json_is_string(utility)) {
            sprintf(response, "{\"type\":\"CommandResponse\",\"action\":\"toggle\",\"status\":\"Error\",\"message\":\"Missing or invalid utility\"}");
            return FALSE;
        }

        const char *utility_str = json_string_value(utility);
        int utility_toggled = 1;

        if (strcmp(utility_str, "tv") == 0) {
            getTVState() ? turnTVOff() : turnTVOn();
        }
        else if (strcmp(utility_str, "heater") == 0) {
            getHeatState() ? turnHeatOff() : turnHeatOn();
        }
        else if (strcmp(utility_str, "lamp_floor") == 0) {
            if(stateLampFloor){
                stateLampFloor = 0;
                dimSLamp(0);
            } else{
                stateLampFloor = 1;
                dimSLamp((uint16_t)dutyCycleLed);
            }
        }
        else if (strcmp(utility_str, "lamp_ceil") == 0) {
            if(stateLampCeiling){
                stateLampCeiling = 0;
                dimRLamp(0);
            } else{
                stateLampCeiling = 1;
                dimRLamp((uint16_t)dutyCycleLed);
            }
        }
        else{
            utility_toggled = 0;
        }
            
        if(utility_toggled){
            sprintf(response, "{\"type\":\"CommandResponse\",\"action\":\"toggle\",\"status\":\"Success\",\"message\":\"%s toggled successfully\"}", utility_str);
        } else{
            sprintf(response, "{\"type\":\"CommandResponse\",\"action\":\"toggle\",\"status\":\"Error\",\"message\":\"Invalid utility: %s\"}", utility_str);
            return FALSE;
        }
    } else {
        sprintf(response, "{\"type\":\"CommandResponse\",\"action\":\"%s\",\"status\":\"Error\",\"message\":\"Invalid action\"}", action_str);
        return FALSE;
    }

    json_decref(root);
    return TRUE;
}
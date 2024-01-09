/******************************************************************************/
/** \file       startup.c
 *******************************************************************************
 *
 *  \brief      Main application for the Rasberry-Pi webhouse
 *
 *  \author     fue1
 *
 *  \date       November 2021
 *
 *  \remark     Last Modification
 *               \li fue1, November 2021, Created
 *
 ******************************************************************************/
/*
 *  functions  global:
 *              main
 * 
 *  functions  local:
 *              shutdownHook
 * 
 *  Autor	   Elham Firouzi
 *
 ******************************************************************************/

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
static int InitSocket(void);
static int HandleHandshake(int com_sock_id, char* rxBuf);
static int CheckAndHandleCloseFrame(int com_sock_id, char* rxBuf, int rx_data_len);
static void DecodeMessage(int com_sock_id, char* rxBuf, int rx_data_len);
static int processCommand(char*, char*);
static void shutdownHook (int32_t);

//----- Global variables -------------------------------------------------------
static volatile int eShutdown = FALSE;
static int dutyCycleLed = 25;

/*******************************************************************************
 *  function :    main
 ******************************************************************************/
/** \brief     void clear_rxBuffer(void)   Starts the socket server (ip: localhost, port:5000) and waits
 *                on a connection attempt of the client.
 *
 *  \type         global
 *
 *  \return       EXIT_SUCCESS
 *
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
    turnTVOff();
    turnHeatOff();
    dimRLamp(0);
    dimSLamp(0);
    turnLED1Off();
    turnLED2Off();

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

		char rxBuf[RX_BUFFER_SIZE];
		int rx_data_len = recv (com_sock_id, (void *)rxBuf, RX_BUFFER_SIZE, MSG_DONTWAIT);

		// If a new WebSocket message have been received
		if (rx_data_len > 0) {
            printf("Received %d bytes\n", rx_data_len);
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
	
	closeWebhouse();
	printf ("Close Webhouse\n");
	fflush (stdout);
	close(server_sock_id);

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
	char command[rx_data_len];
	if(decode_incoming_request(rxBuf, command, rx_data_len) == -1){
        printf("Error decoding incoming request\n");
        fflush(stdout);
        return;
    }

	command[strlen(command)] = '\0';
	char response[RX_BUFFER_SIZE];
	if(!processCommand(command, response)){
        printf("Error processing command, response: %s \n", response);
        fflush(stdout);
    }

    // Send the response
	char codedResponse[strlen(response)+2];
	code_outgoing_response (response, codedResponse);
    //printf("Response: %s\n", response);
    //printf("Coded response: %s\n", codedResponse);
	send(com_sock_id, (void *)codedResponse, strlen(codedResponse), 0);
}

/*******************************************************************************
 * @brief    Processes the received command and creates a response.
 *
 * @param    command   The received command.
 * @param    response  The response to be sent back.
 * @return   void
 ******************************************************************************/
static int processCommand(char* command, char* response) 
{
    printf("[%d] Command: {%s}\n", __LINE__, command);
    fflush(stdout);

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
                } else if (strcmp(utility_str, "lamp1") == 0) {
                    json_object_set_new(data, "lamp1", json_integer(getLED1State()));
                } else if (strcmp(utility_str, "lamp2") == 0) {
                    json_object_set_new(data, "lamp2", json_integer(getLED2State()));
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
            if(dutyCycleLed < 0)    dutyCycleLed = 0;
            if(dutyCycleLed > 100)  dutyCycleLed = 100;

            if(getLED1State()){
                dimSLamp((uint16_t)dutyCycleLed);
            }

            if(getLED2State()){
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
        else if (strcmp(utility_str, "lamp1") == 0) {
            if(getLED1State()){
                turnLED1Off();
                dimSLamp(0);
            } else{
                turnLED1On();
                dimSLamp((uint16_t)dutyCycleLed);
            }
        }
        else if (strcmp(utility_str, "lamp2") == 0) {
            if(getLED2State()){
                turnLED2Off();
                dimRLamp(0);
            } else{
                turnLED2On();
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
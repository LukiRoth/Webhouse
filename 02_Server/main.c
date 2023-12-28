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
#define RX_BUFFER_SIZE 1024   // Buffer size for receiving data, maybe 1024
#define WS_HS_ACCLEN 256      // Buffer size for WebSocket handshake response


//----- Function prototypes ----------------------------------------------------
static void shutdownHook (int32_t sig);

//----- Data -------------------------------------------------------------------
static volatile int eShutdown = FALSE;

//----- Implementation ---------------------------------------------------------

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
	int bind_status;
	int listen_status;
	int server_sock_id;
	int com_sock_id;
	struct sockaddr_in server;
	struct sockaddr_in client;
	int addrlen = sizeof (struct sockaddr_in);
	
	signal(SIGINT, shutdownHook);

	// Initialize Webhouse
	initWebhouse();
    printf("Init Webhouse\n");
	fflush(stdout);

	//--- Initialize Server
    
	// Create a socket
    server_sock_id = socket(AF_INET, SOCK_STREAM, 0);
    if (server_sock_id < 0) {
        perror("Socket creation failed");
        return(EXIT_FAILURE);
    }

	// Set server address
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = htonl(INADDR_ANY);
    server.sin_port = htons(SERVER_PORT);

	// Bind the socket
	bind_status = bind(server_sock_id, (struct sockaddr *)&server, sizeof(server));
    if (bind_status < 0) {
        perror("Socket bind failed");
        close(server_sock_id);
        return(EXIT_FAILURE);
    }

	// Listen on the socket
	listen_status = listen(server_sock_id, BACKLOG);
    if (listen_status < 0) {
        perror("Listen failed");
        close(server_sock_id);
        return(EXIT_FAILURE);
    }

	// Main Loop
	while (eShutdown == FALSE) {
		sleep(1);

		// Accept a connection
		com_sock_id = accept(server_sock_id, (struct sockaddr *)&client, &addrlen);
		if (com_sock_id < 0) {
			perror("Accept failed");
			continue;
		}

		// Read the client's message
		char rxBuf[RX_BUFFER_SIZE];
		int rx_data_len = recv(com_sock_id, (void *)rxBuf, RX_BUFFER_SIZE, MSG_DONTWAIT);

		if (rx_data_len > 0) {
			rxBuf[rx_data_len] = '\0';

			// Is the message a handshake request
			if (strncmp(rxBuf, "GET", 3) == 0) {
				// Yes -> create the handshake response and send it back
				char response[WS_HS_ACCLEN];
				get_handshake_response(rxBuf, response);
				send(com_sock_id, (void *)response, strlen(response), 0);
			} else {
				// No -> decode incoming message, process the command and send back an acknowledge message

				// Process command
				char command[rx_data_len];
				decode_incoming_request(rxBuf, command);
				processCommand(command);

				// Send acknowledgment
				char response[] = "<Command executed>";
				char codedResponse[strlen(response) + 2];
				code_outgoing_response(response, codedResponse);
				send(com_sock_id, (void *)codedResponse, strlen(codedResponse), 0);
			}
		} 
		else if (rx_data_len == 0) {
			printf("Connection closed\n");
			fflush(stdout);
		}
		else {
			perror("Receive failed");
		}
		close(com_sock_id); // Close the client socket after handling
		

		//usleep(1000);
		
	}

	// Close Webhouse
	closeWebhouse();
	printf ("Close Webhouse\n");
	fflush (stdout);

	// Close socket
	if(close(server_sock_id) < 0) {
		perror("Socket close failed");
	} else {
		printf("Socket closed\n");
		fflush(stdout);
	}

	return EXIT_SUCCESS;
}

/*******************************************************************************
 *  function :    shutdownHook
 ******************************************************************************/
/** \brief        Handle the registered signals (SIGTERM, SIGINT)
 *
 *  \type         static
 *
 *  \param[in]    sig    incoming signal
 *
 *  \return       void
 *
 ******************************************************************************/
static void shutdownHook(int32_t sig) {
    printf("Ctrl-C pressed....shutdown hook in main\n");
    fflush(stdout);
    eShutdown = TRUE;
}

/*******************************************************************************
 *  function :    processCommand
 ******************************************************************************/
/** \brief        Process the command received from the client
 * 
 * \type         static
 * 
 * \param[in]    commandJSON    command received from the client
 * 
 * \return       void
 * 
 ******************************************************************************/
static void processCommand(char *commandJSON) {
    json_error_t error;
    json_t *root = json_loads(commandJSON, 0, &error);
    
    if (!root) {
        fprintf(stderr, "error: on line %d: %s\n", error.line, error.text);
        return;
    }

    // Extract action type and other parameters
    json_t *action = json_object_get(root, "action");
    if (!json_is_string(action)) {
        fprintf(stderr, "error: missing or invalid action\n");
        json_decref(root);
        return;
    }

	// Extract action type
    const char *actionStr = json_string_value(action);

    // Implement different actions
    if (strcmp(actionStr, "toggleLamp") == 0) {
        json_t *lampId = json_object_get(root, "lampId");
        json_t *status = json_object_get(root, "status");

        if (json_is_integer(lampId) && json_is_string(status)) {
            int lamp = (int) json_integer_value(lampId);
            const char *statusStr = json_string_value(status);

            if (lamp == 1) {
                if (strcmp(statusStr, "on") == 0) {
                    turnLED1On();
                } else {
                    turnLED1Off();
                }
            }
            //TODO: Implement other lamps
        }
    }
    //TODO: Implement other actions

    json_decref(root);
}

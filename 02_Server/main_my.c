
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
#define RX_BUFFER_SIZE 512   // Buffer size for receiving data, maybe 1024

//----- Function prototypes ----------------------------------------------------
static void shutdownHook (int32_t sig);
static int InitSocket(void);
static int InitSocket2(void);
static void processCommand(char *commandJSON);

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
	//int bind_status;
	//int listen_status;
	int server_sock_id;
	int com_sock_id;
	//struct sockaddr_in server;
	struct sockaddr_in client;
	int addrlen = sizeof (struct sockaddr_in);
	
	signal(SIGINT, shutdownHook);

	// Initialize Webhouse
	printf("Init Webhouse\n");
	fflush(stdout);
	initWebhouse();

	// Initialize Socket
	printf("Init Socket\n");
	fflush(stdout);
	server_sock_id = InitSocket2();

	if(server_sock_id < 0) {
		perror("Socket initialization failed");
		return EXIT_FAILURE;
	}

	// Main Loop
	while (eShutdown == FALSE) {
		printf("Main loop...\n");
		fflush(stdout);
		sleep(1);

		struct sockaddr_in client;
		int com_addrlen;
		int com_sock_id = accept(server_sock_id, (struct sockaddr *)&client, &com_addrlen);
		if (com_sock_id < 0) {
			close(server_sock_id);
			perror("Accept failed");
			printf ("Close socket connection at accept\n");
			return -1;
		}
		else {
			printf("Connection established. Com id: %d \n", com_sock_id);
			fflush(stdout);
		}

		// Accept a connection
		/*com_sock_id = accept(server_sock_id, (struct sockaddr *)&client, &addrlen);
		if (com_sock_id < 0) {
			perror("Accept failed");
			continue;
		}*/

		//printf("acception succeeded. com sock id: %d \n", com_sock_id);
		//fflush(stdout);

		// Read the client's message
		char rxBuf[RX_BUFFER_SIZE];
		int rx_data_len = recv(com_sock_id, (void *)rxBuf, RX_BUFFER_SIZE, MSG_DONTWAIT);

		printf("Data received from server: %d \n", rx_data_len);
		fflush(stdout);

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
			// Check if the error is EAGAIN or EWOULDBLOCK (which means "try again")
			if (errno == EAGAIN || errno == EWOULDBLOCK) {
				printf("No data available right now, try again later.\n");
				fflush(stdout);
			} else {
				perror("Receive failed");
			}
		}

		//close(com_sock_id); // Close the client socket after handling	
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
 *  function :    InitSocket
 ******************************************************************************/
/** \brief        Initialize the socket
 * 	
 * 	\type         static
 * 
 * \return       server socket id
 * 
 ******************************************************************************/
static int InitSocket(void)
{
	// Create a socket
    int server_sock_id = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (server_sock_id < 0) {
        perror("Socket creation failed");
        return(-1);
    }

	printf("Socket created successfully. server sock id: %d \n", server_sock_id);
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
        return(-1);
    }

	printf("Socket binded successfully\n");
	fflush(stdout);

	// Listen on the socket
	int listen_status = listen(server_sock_id, BACKLOG);
    if (listen_status < 0) {
        perror("Listen failed");
        close(server_sock_id);
        return(-1);
    }

	printf("listen succeeded\n");
	fflush(stdout);
	
	return server_sock_id;
}

/*******************************************************************************
 *  function :    InitSocket2
 ******************************************************************************/
/** \brief        Initialize the socket
 * 
 * \type         static
 * 
 * \return       server socket id
 * 
 ******************************************************************************/
static int InitSocket2() {
	int server_sock_id = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (server_sock_id < 0) {
		close(server_sock_id);
		perror("Socket creation failed");
		printf ("Close socket connection at socket\n");
		return -1;
	}
	else {
		/* 	Sometimes, even after your program has terminated, the OS might keep the socket in a 
			TIME_WAIT state for a while, preventing the re-binding to the same address/port. 
			You can set the SO_REUSEADDR socket option to allow immediate reuse of the port:*/
		int yes = 1;
		if (setsockopt(server_sock_id, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)) == -1) {
			perror("setsockopt");
			return -1;
		}

		struct sockaddr_in server;
		server.sin_family = AF_INET;
		server.sin_port = htons(8000);
		server.sin_addr.s_addr = INADDR_ANY;
		
		int addrlen = sizeof (struct sockaddr_in);
		int bind_status = bind(server_sock_id, (struct sockaddr *)&server, addrlen);
		if (bind_status < 0) {
			close(server_sock_id);
			perror("Socket bind failed");
			printf ("Close socket connection at bind\n");
			return -1;
		}
		else {
			int listen_status = listen(server_sock_id,5);
			if (listen_status < 0) {	
				close(server_sock_id);
				perror("Listen failed");
				printf ("Close socket connection at listen\n");
				return -1;
			}
			else {
				/*struct sockaddr_in client;
				int com_addrlen;
				int com_sock_id = accept(server_sock_id, (struct sockaddr *)&client, &com_addrlen);
				if (com_sock_id < 0) {
					close(server_sock_id);
					perror("Accept failed");
					printf ("Close socket connection at accept\n");
					return -1;
				}
				else {
					printf("Connection established. Com id: %d \n", com_sock_id);
					fflush(stdout);
				}	*/
			}
		}
	}

	return server_sock_id;
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

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
#define RX_BUFFER_SIZE 512   	// Buffer size for receiving data, maybe 1024

//----- Function prototypes ----------------------------------------------------
static int InitSocket(void);
static void processCommand(char*, char*);
static void shutdownHook (int32_t);

//----- Global variables -------------------------------------------------------
static volatile int eShutdown = FALSE;

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
	int com_addrlen;							// Length of the communication address
	
	// Register shutdown hook
	signal(SIGINT, shutdownHook);

	// Initialize Webhouse
	printf("Init Webhouse\n");
	fflush(stdout);
	initWebhouse();

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
			rxBuf[rx_data_len] = '\0';
			// Is the message a handshake request
			if (strncmp(rxBuf, "GET", 3) == 0) {
				// Yes -> create the handshake response and send it back
				char response[WS_HS_ACCLEN];
				get_handshake_response(rxBuf, response);
				send(com_sock_id, (void *)response, strlen(response), 0);
			}
			else {
				/* No -> decode incoming message,
				process the command and
				send back an acknowledge message */
				printf("Received: {%s}\n", rxBuf);


				rxBuf[rx_data_len] = '\0';

				// Check the opcode to determine the frame type
				uint8_t opcode = rxBuf[0] & 0x0F;
				if (opcode == 0x8) { // Close frame
					// Handle close logic here
					printf("Close frame received\n");
					char closeFrame[2] = { 0x88, 0x00 }; // Simple close frame
					send(com_sock_id, closeFrame, sizeof(closeFrame), 0);
					close(com_sock_id);
					com_sock_id = -1;
				} else {
					char command[rx_data_len];
					//char response[RX_BUFFER_SIZE];
					decode_incoming_request(rxBuf, command);
					command[strlen(command)] = '\0';

					printf("Command: {%s}\n", command);

					//processCommand(command, response);
					
					char response[] = "<Command executed>";
					char codedResponse[strlen(response)+2];
					code_outgoing_response (response, codedResponse);
					send(com_sock_id, (void *)codedResponse, strlen(codedResponse), 0);
				}
			}
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
 *  function :    InitSocket
 ******************************************************************************/
/** \brief        Creats server Socket
 *
 *  \type         static
 *
 *  \param[in]    none
 *
 *  \return       void
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
 *  function :    processCommand
 ******************************************************************************/
/** \brief        Processes the command recieved from the client.
 *
 *  \type         static
 *
 *  \param[in]    command command recieved
 *
 *  \return       void
 *
 ******************************************************************************/
 static void processCommand(char* command, char* response) {

	//Divide commands into single commands
	char L1 = command[0];
	char DIMS[4] = {command[1],command[2],command[3], '\0'};
	char L2 = command[4];
	char DIMR[4] = {command[5],command[6],command[7], '\0'};
	char TV = command[8];
	char HEAT = command[9];
	char TEMP[4] = {command[10],command[11],command[12], '\0'};
	
	//Adjust the housing according to commands
	if (L1 == '1') {
		turnLED1On();
		printf("L1on");
		fflush(stdout);
	}
	else {
		turnLED1Off();
		printf("L1off");
		fflush(stdout);
	}
	int dim = atoi(DIMS);
	dimSLamp(dim);
	if (L2 == '1') {
		turnLED2On();
		printf("L2on");
		fflush(stdout);
	}
	else {
		turnLED2Off();
		printf("L2off");
		fflush(stdout);
	}
	dim = atoi(DIMR);
	dimRLamp(dim);
	if (TV == '1') {
		turnTVOn();
		printf("TVon");
		fflush(stdout);
	}
	else {
		turnTVOff();
		printf("TVoff");
		fflush(stdout);
	}
	if (HEAT == '1') {
		turnHeatOn();
		printf("Heaton");
		fflush(stdout);
	}
	else {
		turnHeatOff();
		printf("Heatoff");
		fflush(stdout);
	}
	
	float temp_req = atof(TEMP)/10;
	float temp_cur = getTemp();
	if (temp_cur < temp_req) {
		turnHeatOn();
		printf("Heaton");
		fflush(stdout);
	}
	else if (temp_cur > temp_req) {
		turnHeatOff();	
		printf("Heatoff");
		fflush(stdout);
	}
	
	//Prepare Response with temperature and alarm state
	temp_cur = temp_cur*10;
	int temp_int = temp_cur;
	sprintf(TEMP, "%d", temp_int);
	
	char ALARM[1];
	sprintf(ALARM, "%d", getAlarmState());
	strcat(TEMP,ALARM);
	strcpy(response, TEMP);
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

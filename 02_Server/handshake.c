/*
 * Copyright (C) 2016-2020  Davidson Francis <davidsondfgl@gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>
 */
#define _POSIX_C_SOURCE 200809L
#include "base64.h"
#include "sha1.h"
#include "handshake.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/**
 * @dir src/handshake
 * @brief Handshake routines directory
 *
 * @file handshake.c
 * @brief Handshake routines.
 */

/**
 * @brief Gets the field Sec-WebSocket-Accept on response, by
 * an previously informed key.
 *
 * @param wsKey Sec-WebSocket-Key
 * @param dest source to be stored the value.
 *
 * @return Returns 0 if success and a negative number
 * otherwise.
 *
 * @attention This is part of the internal API and is documented just
 * for completeness.
 */

static int get_handshake_accept(char *wsKey, unsigned char **dest)
{
    unsigned char hash[SHA1HashSize]; /* SHA-1 Hash.                   */
    SHA1Context ctx;                  /* SHA-1 Context.                */
    char *str;                        /* WebSocket key + magic string. */

    /* Invalid key. */
    if (!wsKey)
        return (-1);

    str = (char *) calloc(1, sizeof(char) * (WS_KEY_LEN + WS_MS_LEN + 1));
    if (!str)
        return (-1);

    strncpy(str, wsKey, WS_KEY_LEN);
    strcat(str, MAGIC_STRING);

    SHA1Reset(&ctx);
    SHA1Input(&ctx, (const uint8_t *)str, WS_KEYMS_LEN);
    SHA1Result(&ctx, hash);

    *dest = base64_encode(hash, SHA1HashSize, NULL);
    *(*dest + strlen((const char *)*dest) - 1) = '\0';
    free(str);
    return (0);
}

/**
 * @brief Gets the complete response to accomplish a succesfully
 * handshake.
 *
 * @param hsrequest  Client request.
 * @param hsresponse Server response.
 *
 * @return Returns 0 if success and a negative number
 * otherwise.
 *
 * @attention This is part of the internal API and is documented just
 * for completeness.
 */

int get_handshake_response(char hsrequest[], char hsresponse[])
{
    unsigned char *accept; /* Accept message.     */
    char *saveptr;         /* strtok_r() pointer. */
    char *s;               /* Current string.     */
    int ret;               /* Return value.       */

    saveptr = NULL;
    for (s = strtok_r(hsrequest, "\r\n", &saveptr); s != NULL;
         s = strtok_r(NULL, "\r\n", &saveptr))
    {
        if (strstr(s, WS_HS_REQ) != NULL)
            break;
    }

    /* Ensure that we have a valid pointer. */
    if (s == NULL)
        return (-1);

    saveptr = NULL;
    s       = strtok_r(s, " ", &saveptr);
    s       = strtok_r(NULL, " ", &saveptr);

    ret = get_handshake_accept(s, &accept);
    if (ret < 0)
        return (ret);

    strcpy(hsresponse, WS_HS_ACCEPT);
    strcat(hsresponse, (const char *)accept);
    strcat(hsresponse, "\r\n\r\n");

    free(accept);
    return (0);
}

/*int decode_incoming_request (char coded_request[], char request[]){
    // read the number of received data bytes
    int size = (coded_request[1] & 0x3F);
    
    if (size == 0) {
        return (-1);
    }
    
    // check if the incomming request is coded
    if (coded_request[1] & 0x80) {
        // yes: then decode request
        char masks[4] = {coded_request[2], coded_request[3], coded_request[4], coded_request[5]};
        int i;
        for (i = 0; i < size; i++) {
            request[i] = coded_request[i+6] ^ masks[i%4];
        }
        request[i] = 0;
    }
    else {
        // no: then remove only the 2 first bytes of request 
        int i;
        for (i = 0; i < size; i++) {
            request[i] = coded_request[i+2];
        }
        request[i] = 0;
    }

    return (size);
}*/

// New functionw as created because old lead to errors

/*******************************************************************************
 * @brief    Decodes an incoming WebSocket request.
 *
 *           This function decodes an incoming WebSocket request, considering
 *           the variable length of the payload and the mask applied to the data.
 *           It handles different payload lengths specified by the WebSocket
 *           protocol, including lengths defined by additional bytes (126 for 16-bit
 *           length and 127 for 64-bit length). It also applies the masking keys
 *           as required by the WebSocket standard for messages sent from the client
 *           to the server.
 *
 * @param    coded_request       The incoming WebSocket request in its encoded form.
 * @param    request             Buffer to store the decoded request.
 * @param    coded_request_len   Length of the encoded request.
 *
 * @return   The size of the decoded message, or -1 if an error occurs.
 ******************************************************************************/
int decode_incoming_request (char coded_request[], char request[], int coded_request_len) {
    // Überprüfen der Länge des codierten Requests
    if (coded_request_len < 2) {
        return (-1); // Nicht genug Daten, um die Größe zu bestimmen
    }

    int size = coded_request[1] & 0x7F; // Die ersten 7 Bits der zweiten Byte lesen
    int mask_offset = 2;

    if (size == 126) {
        // Zusätzliche 2 Bytes für die Größe
        if (coded_request_len < 4) {
            return (-1); // Nicht genug Daten für die Größe
        }
        size = ((coded_request[2] & 0xFF) << 8) | (coded_request[3] & 0xFF);
        mask_offset = 4;
    } else if (size == 127) {
        // Zusätzliche 8 Bytes für die Größe
        if (coded_request_len < 10) {
            return (-1); // Nicht genug Daten für die Größe
        }
        // Behandeln Sie die 64-Bit-Ganzzahl für die Größe (falls benötigt)
    }

    if (coded_request_len < mask_offset + 4 + size) {
        return (-1); // Nicht genug Daten für die gesamte Nachricht
    }

    char masks[4] = {coded_request[mask_offset], coded_request[mask_offset+1], coded_request[mask_offset+2], coded_request[mask_offset+3]};
    
    for (int i = 0; i < size; i++) {
        request[i] = coded_request[i + mask_offset + 4] ^ masks[i % 4];
    }
    request[size] = 0;

    return size;
}

int code_outgoing_response (char response[], char coded_response[]){
    // read the number of data bytes to send
    int size = strlen (response);
    
    if (size == 0) {
        return (-1);
    }

    // add the 2 first header bytes to the response
    coded_response[0] = 0x81;
    coded_response[1] = size;
    coded_response[2] = 0;
    strcat(coded_response, response);

    return (size);
}

/*
int code_outgoing_response(char response[], char coded_response[]) {
    int size = strlen(response);
    printf("Size: %d\n", size);

    if (size == 0) {
        return -1;
    }

    int header_size;

    // Set the FIN bit and text frame opcode
    coded_response[0] = 0x81;

    if (size <= 125) {
        coded_response[1] = size;
        header_size = 2;
    } else if (size <= 0xFFFF) {
        coded_response[1] = 126;
        coded_response[2] = (size >> 8) & 0xFF; // Higher order byte
        coded_response[3] = size & 0xFF;        // Lower order byte
        header_size = 4;
    } else {
        // Handling for size greater than 65535 is not implemented
        return -1;
    }

    // Copy the response into the coded_response buffer right after the header
    memcpy(coded_response + header_size, response, size);
    return size + header_size;
}

*/

#ifndef HANDSHAKE_H
#define HANDSHAKE_H

extern int get_handshake_response  (char *hsrequest,     char **hsresponse);
extern int decode_incoming_request (char *coded_request, char **request,       int size);
extern int code_outgoing_response  (char *respons,       char **coded_respons, int *size);

#define WS_KEY_LEN     24
// Magic string length.
#define WS_MS_LEN      36
// Accept message response length.
#define WS_KEYMS_LEN   (WS_KEY_LEN + WS_MS_LEN)
// Magic string.
#define MAGIC_STRING   "258EAFA5-E914-47DA-95CA-C5AB0DC85B11"
// Alias for 'Sec-WebSocket-Key'.
#define WS_HS_REQ      "Sec-WebSocket-Key"
// Handshake accept message length.
#define WS_HS_ACCLEN   130
// Handshake accept message.
#define WS_HS_ACCEPT                       \
    "HTTP/1.1 101 Switching Protocols\r\n" \
    "Upgrade: websocket\r\n"               \
    "Connection: Upgrade\r\n"              \
    "Sec-WebSocket-Accept: "

#endif // HANDSHAKE_H

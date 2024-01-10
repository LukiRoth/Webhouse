#include <QTcpServer>
#include <QTcpSocket>

#include "string.h"
#include "handshake.h"

#define SERVER_PORT_NBR 80
#define BACKLOG 5
#define RX_BUFFER_SIZE 512
#define TX_BUFFER_SIZE 512

#define LocalPort 8000

int main () {
    QTcpServer* qtss = new QTcpServer();

    // Start the listen modus
    if (qtss->listen(QHostAddress::Any, LocalPort) == false) {
        qDebug()<<"Error: could not start listen function";
        return -1;
    }

    // Display the server port number
    qDebug()<<"Server port: "<<qtss->serverPort();

    // Create a communication socket
    QTcpSocket *qtns = NULL;
    if (qtss == NULL) {
        qDebug()<<"Error: could not create a communication socket";
        return -1;
    }

    // Waits indefinitely for a new connection!
    do {
        if (qtss->waitForNewConnection(1000) == true) {
            qtns = qtss->nextPendingConnection();
        }
    } while (qtns == NULL);

    for(;;) {
        // Wait durint Timeout millisecones for a new message
        qtns->waitForReadyRead();

        // Test if the data can be read
        if (qtns->isReadable() == true){
            // Determin how many bytes have been received
            int size = qtns->bytesAvailable();

            if (size > 0) {
                // Read the handshake request
                char request[size];
                qtns->read(request, size);

                if (strncmp (request, "GET", 3) == 0) {
                    qDebug()<<"Beging of handshake";
                    qDebug()<<request;

                    // Generate the handshake respons
                    char *respons;
                    get_handshake_response(request, &respons);
                    qDebug()<<respons;

                    // Send the handshake request
                    if (qtns->isWritable()) {
                        qtns->write(QByteArray((char *) respons, strlen(respons)));
                        qtns->waitForBytesWritten();
                    }
                    free(respons);
                    qDebug()<<"End of handshake";
                    qDebug()<<"***********************************";
                }
                else if (request[1] & 0x80) {
                    qDebug()<<"Beging of message";
                    char *command;
                    decode_incoming_request(request, &command, size);
                    qDebug()<<command;

                    int length;
                    char *response;
                    code_outgoing_response(command, &response, &length);
                    qDebug()<<(char *)(response + 2);

                    // Send the message response
                    if (qtns->isWritable()) {
                        qtns->write(QByteArray((char *)response, length));
                        qtns->waitForBytesWritten();
                    }

                    free(command);
                    free(response);
                    qDebug()<<"End of message";
                }
            }
        }
    }

    return 0;
}

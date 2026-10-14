#include "mprotocol.h"

#include <stdlib.h>
#include <stdio.h>

MorgunovProtocolHeader GetMorgunovProtocolHeader(MorguSocket* socket) {
    uint16_t dataLenBuf;
    MorguSockReceiveAll(*socket, (uint8_t*)&dataLenBuf, 2); // evil bit hack
    dataLenBuf = ntohs(dataLenBuf); // we assume sent data went through htons()

    uint8_t commandId, version;
    MorguSockReceiveAll(*socket, &commandId, 1);
    MorguSockReceiveAll(*socket, &version, 1);

    MorgunovProtocolHeader header = {
        .DataLength = dataLenBuf,
        .CommandID = commandId,
        .Version = version
    };
    return header;
}

char* GetMorgunovProtocolQuery(MorguSocket* socket, uint16_t dataLength) {
    char* dataBuf = malloc(dataLength + 1);
    MorguSockReceiveAll(*socket, dataBuf, (size_t)dataLength);
    dataBuf[dataLength] = '\0';
    return dataBuf;
}

LString* FormatMorgunovProtocolHeader(MorgunovProtocolHeader* header) {
    char dataLengthBuf[6]; // enough for '65535\0'
    char versionBuf[4]; // enough for '255\0'
    char commandIdBuf[4];

    LString* string = LStringCreate("Data Length: ");

    snprintf(dataLengthBuf, sizeof(dataLengthBuf), "%u", header->DataLength);
    LStringAppendCString(&string, dataLengthBuf);

    LStringAppendCString(&string, "\nVersion: ");
    snprintf(versionBuf, sizeof(versionBuf), "%u", header->Version);
    LStringAppendCString(&string, versionBuf);

    LStringAppendCString(&string, "\nCommand ID: ");
    snprintf(commandIdBuf, sizeof(commandIdBuf), "%u", header->CommandID);
    LStringAppendCString(&string, commandIdBuf);

    LStringAppendCString(&string, "\n");
    return string;
}
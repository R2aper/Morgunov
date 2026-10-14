#pragma once

#include <stdint.h>

#include "src/network/morgusock.h"
#include "src/common/lstring.h"

#define MPROTOCOL_VER 1

/*
   Morgunov Transfer Protocol Schema

 0                   1                   2                   3
 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|            Data Length        |   Command ID  |    Version    |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|                            Data                             ...
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

*/

#pragma pack(push, 1)
/**
 * @brief Represents Morgunov Protocol Header object
 */
typedef struct MorgunovProtocolHeader {
    uint16_t DataLength;
    uint8_t CommandID;
    uint8_t Version;
} MorgunovProtocolHeader;
#pragma pack(pop)

/**
 * @brief Get the Morgunov Protocol Header from an open connection
 * @note Indefinitely blocking
 * @attention Expects data to be in big-endian (network) format
 * 
 * @param socket Open socket connection
 * @return Morgunov Protocol Header
 */
MorgunovProtocolHeader GetMorgunovProtocolHeader(MorguSocket* socket);

/**
 * @brief Get data associated with the morgunov protocol header
 * @note Indefinitely blocking
 * 
 * @param socket Open socket connection
 * @param dataLength Amount of bytes to read (header.DataLength property)
 * @return [Transfers Ownership] Received protocol data as a c-string
 */
char* GetMorgunovProtocolQuery(MorguSocket* socket, uint16_t dataLength);

/**
 * @brief Formats protocol header into human readable c-string
 * 
 * @param header [Borrows] Header structure pointer
 * @return [Transfers Ownership] Readable LString
 */
LString* FormatMorgunovProtocolHeader(MorgunovProtocolHeader* header);
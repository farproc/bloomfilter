/*
 * Name        : bloomfilt.c
 * Date        : 20120806
 * Author      : Tim Bateman
 * License     : see LICENSE file
 * Description : Simple bloom-filter implementation
 */
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <openssl/sha.h>
#include "bloomfilt.h"

/*
 * I N T E R N A L   F U N C T I O N S
 */
uint32_t BF_hashDataLen()
{
    return SHA_DIGEST_LENGTH;
}

uint8_t* BF_hashData( uint8_t *pData, uint32_t nDataLen )
{
    uint8_t *pRetVal        = NULL;
    SHA_CTX  ctx;

    if( SHA1_Init( &ctx ) )
    {
        pRetVal = malloc( BF_hashDataLen() );
        if( pRetVal )
        {
            if( 0 != SHA1_Update( &ctx, pData, nDataLen ) )
            {
                if( 0 != SHA1_Final( pRetVal, &ctx ) )
                {
                    // awesome
                }
                else
                {
                    // failed to get the result of the hash :(
                }
            }
            else
            {
                // failed to hash :(
            }
        }
    }
    return pRetVal;
}

uint32_t BF_roundUpToBaseTwo( uint32_t nInput )
{
    nInput--;
    nInput |= nInput >> 1;
    nInput |= nInput >> 2;
    nInput |= nInput >> 4;
    nInput |= nInput >> 8;
    nInput |= nInput >> 16;
    nInput++;

    return nInput;
}

uint32_t BF_logTwoOfABaseTwoNumber( uint32_t v )
{
    uint32_t              i   = 0;
    static const uint32_t b[] = { 0xAAAAAAAA, 0xCCCCCCCC, 0xF0F0F0F0, 0xFF00FF00, 0xFFFF0000 };
    uint32_t              r   = (v & b[0]) != 0;

    for (i = 4; i > 0; i--) // unroll for speed...
    {
      r |= ((v & b[i]) != 0) << i;
    }

    return r;
}

uint32_t BF_getBitsFrom( uint32_t nBits, uint8_t *pSrc, uint32_t nOffset )
{
    uint32_t        nRetValue       = 0;
    uint32_t        nBytePos        = 0;
    uint8_t         nBitPos         = 0;
    uint8_t         nTempByte       = 0;

    // we only support reading in one DWORD at the very most
    if( nBits > sizeof( uint32_t ) * 8 )
        return 0;

    nBytePos = nOffset / 8;
    nBitPos  = nOffset % 8;

    while( nBits-- )
    {
        // shift to make room for the bit
        nRetValue <<= 1;

        nTempByte = pSrc[ nBytePos ];

        // bit 0 is the MSB
        // bit 7 the LSB
        nTempByte >>= ( 7 - nBitPos );

        nRetValue |= (nTempByte & 0x01);

        nBitPos++;
        if( 8 == nBitPos )
        {
            // roll-over to the next byte
            nBytePos++;
            nBitPos = 0;
        }
    }
    return nRetValue;
}

static const uint32_t g_nTwoPowersLookup[] =
        { 0x01,0x02,0x04,0x08,0x10,0x20,0x40,0x80 };

void BF_setBit( uint8_t *pbArray, uint32_t nBit )
{
    uint32_t    nByteIndex = 0;
    uint8_t     nBitIndex  = 0;
    uint32_t    nValue     = 0;

    nByteIndex = nBit / 8;
    nBitIndex  = nBit % 8;

    // nBitIndex    ->  nValue
    //  0x00        ->  0x01
    //  0x01        ->  0x02
    //  0x02        ->  0x04
    //  0x03        ->  0x08
    //  0x04        ->  0x10
    //  0x05        ->  0x20
    //  0x06        ->  0x40
    //  0x07        ->  0x80
    // obvously its 2^nBitIndex - but a simple lookup table is going
    // to be wicked fast
    nValue     = g_nTwoPowersLookup[ nBitIndex ];

    pbArray[ nByteIndex ] |= nValue;
}

bool BF_testBit( uint8_t *pbArray, uint32_t nBit )
{
    bool        bRetVal    = false;
    uint32_t    nByteIndex = 0;
    uint8_t     nBitIndex  = 0;
    uint32_t    nValue     = 0;

    nByteIndex = nBit / 8;
    nBitIndex  = nBit % 8;
    nValue     = g_nTwoPowersLookup[ nBitIndex ];

    bRetVal    = ( ( pbArray[ nByteIndex ] & nValue ) == nValue ) ? true : false ;

    return bRetVal;
}

uint32_t* BF_getHashOffsets( PBF_CTX pCtx, uint8_t *pHash )
{
    uint32_t   *pRetValues  = NULL;
    uint32_t    nIdx        = 0;
    uint32_t    nRequiredBitForAddressing = 0;

    pRetValues = malloc( pCtx->nKValue * sizeof( uint32_t ) );

    if( pRetValues )
    {

        // do some math to figure out how to divide up the hash
        // into a bunch of offsets


        // so if:
        // pCtx->nBitArrayLength = 128
        // and each bit needs to be addressed individually - the question is basically
        // how many bits do we need to count to 128. (remembering we're zero indexed)
        //
        // 2^something == 128
        // so.... log2 ( 128 ) == our magic number (in this case 7)

        // so we need to round our number up to a base2 value
        nRequiredBitForAddressing = BF_roundUpToBaseTwo( pCtx->nBitArrayLength );

        // then log2 it to find our value.
        nRequiredBitForAddressing = BF_logTwoOfABaseTwoNumber( nRequiredBitForAddressing );

        // just a quick check that we have enough index-mat ;)
        if( pCtx->nKValue * nRequiredBitForAddressing < BF_hashDataLen() * 8 /* bits in a byte */ )
        {
            for( nIdx = 0; nIdx < pCtx->nKValue; nIdx++ )
            {
                pRetValues[ nIdx ]  = BF_getBitsFrom(
                                        nRequiredBitForAddressing,          // how much
                                        pHash,                              // from
                                        nRequiredBitForAddressing * nIdx ); // offset (in bits)
            }
        }
        else
        {
            // we don't have enough material for decent index's
            // maybe we should using a different algorithum? or salt our hash?
            // or hash half the file at a time with the same hash? OR pick a longer
            // value for K ;)
            free( pRetValues );
            pRetValues = NULL;
        }
    }

    return pRetValues;
}

/*
 * E X P O R T E D   F U N C T I O N S
 */

PBF_CTX BF_allocCTX( uint32_t nBitArrayLength, uint32_t nKValue )
{
    PBF_CTX     pRetVal = NULL;
    uint32_t    nSize   = 0;
    uint8_t    *pPtr    = NULL;

    // we always want a base2 number - makes the math easier ;)
    nBitArrayLength = BF_roundUpToBaseTwo( nBitArrayLength );

    nSize =  sizeof( BF_CTX );
    nSize += nBitArrayLength / 8;
    if( 0 != ( nBitArrayLength % 8 ) )
        nSize++;

    pRetVal = malloc( nSize );
    if( pRetVal )
    {
        pRetVal->bInited            = true;
        pRetVal->nBitArrayLength    = nBitArrayLength;
        pRetVal->nNumberOfInserts   = 0;
        pRetVal->nKValue            = nKValue;
        pPtr                        = pRetVal->pBitArray;
        memset( pPtr, 0, nSize - sizeof( BF_CTX ) );
    }

    return pRetVal;
}

void BF_freeCTX( PBF_CTX  pCtx )
{
    free( pCtx );
}

bool BF_add( PBF_CTX  pCtx, void *pData, uint32_t nDataLen )
{
    bool         bRetVal= false;
    uint8_t     *pHash  = BF_hashData( pData, nDataLen );
    uint32_t     nIdx   = 0;
    uint32_t    *pnIdxs = NULL;
    if( pHash )
    {
        pnIdxs = BF_getHashOffsets( pCtx, pHash );

        if( pnIdxs )
        {
            for( nIdx = 0; nIdx < pCtx->nKValue; nIdx++ )
            {
                BF_setBit( pCtx->pBitArray, pnIdxs[ nIdx ] );
            }
            bRetVal = true;
            pCtx->nNumberOfInserts++;

            free( pnIdxs );
        }
        free( pHash );
    }
    return bRetVal;
}

bool BF_test( PBF_CTX  pCtx, void *pData, uint32_t nDataLen )
{
    bool         bRetVal= false;
    uint8_t     *pHash  = BF_hashData( pData, nDataLen );
    uint32_t     nIdx   = 0;
    uint32_t    *pnIdxs = NULL;
    if( pHash )
    {
        pnIdxs = BF_getHashOffsets( pCtx, pHash );

        if( pnIdxs )
        {
            bRetVal = true;
            for( nIdx = 0; true == bRetVal && nIdx < pCtx->nKValue; nIdx++ )
            {
                bRetVal = BF_testBit( pCtx->pBitArray, pnIdxs[ nIdx ] );
            }

            free( pnIdxs );
        }
        free( pHash );
    }
    return bRetVal;
}

void BF_outputArray( PBF_CTX pCtx )
{
    uint32_t nIdx = 0;

    for( nIdx = 0; nIdx < pCtx->nBitArrayLength; nIdx++ )
    {
        if( nIdx && 0 == nIdx % 80 )
            printf("\n");
        
        if( BF_testBit( pCtx->pBitArray, nIdx ) )
        {
            printf( "1" );
        } else {
            printf( "0" );
        }
    }
    printf( "\n" );
}

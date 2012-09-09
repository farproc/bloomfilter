#ifndef _BLOOMFILTER_H_
#define _BLOOMFILTER_H_

#include <stdbool.h>
#include <stdint.h>

#define BLOOMFILTER_DEFAULT_K_VALUE     ( 3 )

typedef struct _BF_CTX_t
{
    bool        bInited;                // set to true when created properly (just a basic test)
    uint32_t    nBitArrayLength;        // in bits
    uint32_t    nKValue;                // the numbers of _different_ hash functions
    uint32_t    nNumberOfInserts;       //
    uint8_t     pBitArray[];            // "the" bit array
} BF_CTX, *PBF_CTX;


PBF_CTX     BF_allocCTX( uint32_t nBitArrayLength, uint32_t nKValue );
void        BF_freeCTX ( PBF_CTX  pCtx );
bool        BF_add     ( PBF_CTX  pCtx, void *pData, uint32_t nDataLen );
bool        BF_test    ( PBF_CTX  pCtx, void *pData, uint32_t nDataLen );

// useful for debugging
void        BF_outputArray( PBF_CTX pCtx );

#endif

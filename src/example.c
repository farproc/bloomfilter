#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "bloomfilt.h"

#define BF_ARRAY_LENGTH         ( 24 * 1024 )

int main( int argc, char *argv[] )
{
	int	        nRetVal	        = EXIT_SUCCESS;
    uint32_t    nBitArrayLength = BF_ARRAY_LENGTH;
    uint32_t    nKValue         = BLOOMFILTER_DEFAULT_K_VALUE;
    PBF_CTX     pCtx            = NULL;
    uint32_t    nData           = 0;
    uint32_t    nIncorrectTest  = 0;

    pCtx = BF_allocCTX( nBitArrayLength, nKValue );

    if( pCtx )
    {
        printf( "[i]\tcreated empty bloomfilter\n" );

        // add 1000 entries
        while( nData < 1000 )
        {
            if( BF_add( pCtx, &nData, sizeof( nData ) ) )
            {
                //printf( "[i]\tadded entry (%d)\n", nData );
            } else {
                printf( "[x]\tfailed to add entry (%d)\n", nData );
                nRetVal = EXIT_FAILURE;
            }
            nData++;
        }
        if( EXIT_SUCCESS == nRetVal )
        {
            printf( "[i]\tadded 1000 entries succesfully.\n" );
        }

        // now lets check everything appears to be working...

        // all these should return TRUE
        nData = 0;
        while( nData < 1000 )
        {
            if( BF_test( pCtx, &nData, sizeof( nData ) ) )
            {
                //printf( "[x]\ttest for %d returned TRUE\n", nData );
            }
            else
            {
                printf( "[i]\ttest for %d returned FALSE\n", nData );
                nRetVal = EXIT_FAILURE;
            }
            nData++;
        }

        // these should all return FALSE - although there could be 
        while( nData < 10000 )
        {
            if( BF_test( pCtx, &nData, sizeof( nData ) ) )
            {
                printf( "[x]\ttest for %d returned TRUE\n", nData );
                nIncorrectTest++;
            }
            else
            {
                // printf( "[i]\ttest for %d returned false\n", nData );
            }
            nData++;
        }

        BF_outputArray( pCtx );

        printf( "Invalid test results: %d\n", nIncorrectTest );

        BF_freeCTX( pCtx );
    } else {
        printf( "[x]\tfailed to create bloomfilter\n" );
    }

	return nRetVal;
}
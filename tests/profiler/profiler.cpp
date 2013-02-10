// Copyright (c) 2010 Skew Matrix Software. All rights reserved.

#include <iostream>

#if !defined( __BDFX_PROFILE_ENABLE )

int main()
{
    std::cerr << "Profiling is disabled. You must select BDFX_PROFILE_ENABLE in CMake." << std::endl;
    return( 1 );
}

#else


// Trying to use Bullet profiler in situ doesn't work due
// to statically linking Bullet.
#include <LinearMath/btQuickProf.h>

#include <vector>


int main( int argc, char **argv )
{
    {
        BT_PROFILE( "MyFunc" );

        const int sz( 1000000 );
        {
            BT_PROFILE( "for loop" );
            for( int cnt=0; cnt<3; cnt++ )
            {
                BT_PROFILE( "c array" );
                int myvec[ sz ];
                for( int idx=0; idx<sz; idx++ )
                    myvec[ idx ] = rand();
            }
        }
        {
            BT_PROFILE( "for loop" );
            for( int cnt=0; cnt<3; cnt++ )
            {
                BT_PROFILE( "vector" );
                typedef std::vector< int > IntVec;
                IntVec myvec;
                myvec.resize( static_cast< IntVec::size_type >( sz ) );
                for( IntVec::iterator it=myvec.begin(); it != myvec.end(); it++ )
                    *it = rand();
            }
        }
    }

    return 0;
}

#endif

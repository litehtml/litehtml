#pragma once
#include "types.h"
#include "os_types.h"

namespace litehtml
{
    class string_hash
    {
    public:

        string_hash()
            : HashCode( 0 )
        {

        }

        template< unsigned int character_count >
        string_hash( const litehtml::tchar_t ( &text )[ character_count ] )
            : HashCode( string_hash::HashGenerator< character_count >::Hash( text ) )
        {

        }

        string_hash( const litehtml::tchar_t * text )
            : HashCode( string_hash::LoopedHash( text ) )
        {

        }

        string_hash( const litehtml::tstring & text )
            : HashCode( string_hash::LoopedHash( text.c_str() ) )
        {

        }

        explicit string_hash( unsigned int hash )
            : HashCode( hash )
        {

        }

        string_hash( const string_hash & other )
            : HashCode( other.HashCode )
        {

        }

        string_hash( string_hash && other )
            : HashCode( std::move( other.HashCode ) )
        {

        }

        string_hash & operator=( const string_hash & other )
        {
            HashCode = other.HashCode;
            return *this;
        }

        string_hash & operator=( string_hash && other )
        {
            HashCode = std::move( other.HashCode );
            return *this;
        }

        bool operator==( const string_hash & other ) const
        {
            return HashCode == other.HashCode;
        }

        template< unsigned int character_count >
        bool operator==( const litehtml::tchar_t ( &text )[ character_count ] )
        {
            hash_code temp_hash( string_hash::HashGenerator< character_count >::Hash( text ) );
            return HashCode == temp_hash;
        }

        bool operator==( const litehtml::tchar_t * text )
        {
            hash_code temp_hash( string_hash::LoopedHash( text ) );
            return HashCode == temp_hash;
        }

        bool operator!=( const string_hash & other ) const
        {
            return HashCode != other.HashCode;
        }

        bool operator<( const string_hash & other ) const
        {
            return HashCode < other.HashCode;
        }

        bool is_valid() const
        {
            return HashCode != 0;
        }


        bool empty() const
        {
            return HashCode == 0;
        }

        static string_hash normalizeFromString( const litehtml::tchar_t * string )
        {
            return string_hash( LoopedHash<t_tolower>( string ) );
        }

    private:

        template< unsigned int character_count, unsigned int index = character_count - 1 >
        struct HashGenerator
        {
            inline static hash_code Hash( const litehtml::tchar_t ( &character_array )[ character_count ] )
            {
                #ifdef NDEBUG
                    return ( HashGenerator< character_count, index - 1 >::Hash( character_array ) ^ character_array[ index - 1 ] ) * FNV_Prime;
                #else
                    return LoopedHash( character_array );
                #endif
            }
        };

        static int i(int i) {return i;}

        template<int (_TRANSFROM_)(int) = i >
        inline static unsigned int LoopedHash( const litehtml::tchar_t * input )
        {
            unsigned int result = FNV_OffsetBase;

            while (*input) {
                result ^= _TRANSFROM_(*input++);
                result *= FNV_Prime;
            }

            return result;
        }

        hash_code HashCode;

        static const unsigned int FNV_OffsetBase = 2166136261u;
        static const unsigned int FNV_Prime = 16777619u;
    };

    template< unsigned int character_count >
    struct string_hash::HashGenerator< character_count, 0 >
    {
        inline static hash_code Hash( const litehtml::tchar_t ( &character_array )[ character_count ] )
        {
            return FNV_OffsetBase;
        }
    };

    // Special case: Hash empty string as 0 to make it more human readable
    template<>
    struct string_hash::HashGenerator<1, 0>
    {
        inline static hash_code Hash( const litehtml::tchar_t ( & )[ 1 ] )
        {
            return 0u;
        }
    };
}

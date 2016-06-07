#pragma once
#include "types.h"
#include "os_types.h"

namespace litehtml
{

    constexpr hash_code GetHashCodeConstexp(
       const hash_code code,
       const tchar_t * text
       )
    {
       return *text ? GetHashCodeConstexp( static_cast<hash_code>( ( static_cast<unsigned long long>( code ) ^ *text ) * 16777619u ), text + 1 ) : code;
    }

    // ~~

    constexpr hash_code GetHashCodeConstexp(
       const tchar_t * text
       )
    {
       return GetHashCodeConstexp( 2166136261u, text );
    }

    constexpr hash_code operator ""_hash(
        const char* text,
        size_t /*size*/
        )
    {
        return GetHashCodeConstexp( text );
    }

    class string_hash
    {
    public:

        string_hash()
            : HashCode( 0 )
        {

        }

        template< unsigned int character_count >
        string_hash( const char ( &text )[ character_count ] )
            : HashCode( GetHashCodeConstexp( text ) )
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
        bool operator==( const char( &text )[ character_count ] ) const
        {
            return HashCode == GetHashCodeConstexp( text );
        }

        bool operator==( const litehtml::tchar_t * text ) const
        {
            hash_code temp_hash( string_hash::LoopedHash( text ) );
            return HashCode == temp_hash;
        }

        bool operator==( const hash_code code ) const
        {
            return HashCode == code;
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

        hash_code GetHashCode() const
        {
            return HashCode;
        }

        static string_hash normalizeFromString( const litehtml::tchar_t * string )
        {
            return string_hash( LoopedHash<t_tolower>( string ) );
        }

    private:

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
}

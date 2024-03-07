#include <gtest/gtest.h>
#include <litehtml.h>

using namespace litehtml;
using namespace std;

#define S(s) string{ s, sizeof(s) - 1 }

struct test
{
	string input;
	string output;
};
	
test utf8_tests[] =
{
	// VALID INPUTS

	// input       output
	{ S("\x00"), S("\x00") }, // NUL
	{ S("A"),    S("A") },    // printable ASCII

	// A - ASCII, L2 - lead byte of a 2-byte sequence, C - continuation byte
	{ S("\xC2\xA3"),  S("\xC2\xA3") },  // L2 C
	{ S("\xE2\x82\xAC"),  S("\xE2\x82\xAC") },  // L3 C C
	{ S("\xF0\x90\x8D\x88"),  S("\xF0\x90\x8D\x88") },  // L4 C C C

	{ S("\xEF\xBB\xBF\xC2\xA3"),  S("\xC2\xA3") },  // utf-8 bom is removed

	// KINDA INVALID INPUTS

	{ S("\xFF\xFE\xC2\xA3"),  S("\xEA\x8F\x82") },  // utf-16le bom is removed, the rest is interpreted as utf-16le 
	// Because UTF-32 is not a valid HTML encoding, UTF-32LE BOM is not recognized, it is interpreted as 
	// UTF-16LE BOM. UTF-16LE BOM is removed, the rest is interpreted as UTF-16LE.
	{ S("\xFF\xFE\x00\x00\xC2\xA3"), S("\x00\xEA\x8F\x82") },

	// INVALID INPUTS

	{ S("\xC0\x80"), S("\xEF\xBF\xBD\xEF\xBF\xBD") }, // overlong sequence for NUL -> 2 U+FFFD
	{ S("\xF0\x82\x82\xAC"), S("\xEF\xBF\xBD\xEF\xBF\xBD\xEF\xBF\xBD\xEF\xBF\xBD") }, // overlong sequence for € -> 4 U+FFFD

	{ S("\xED\xB0\x80"), S("\xEF\xBF\xBD\xEF\xBF\xBD\xEF\xBF\xBD") }, // low surrogate U+DC00 -> 3 U+FFFD
	// low surrogate U+DC00 + high surrogate U+D800 -> 6 U+FFFD
	{ S("\xED\xB0\x80\xED\xA0\x80"), S("\xEF\xBF\xBD\xEF\xBF\xBD\xEF\xBF\xBD\xEF\xBF\xBD\xEF\xBF\xBD\xEF\xBF\xBD") }, 

	{ S("\xF4\x90\x80\x80"), S("\xEF\xBF\xBD\xEF\xBF\xBD\xEF\xBF\xBD\xEF\xBF\xBD") }, // U+110000 invalid codepoint (must be <= 0x10FFFF)

	{ S("\x80"),         S("\xEF\xBF\xBD") },     // C
	{ S("\xC2\x41"),     S("\xEF\xBF\xBD\x41") }, // L2 A
	{ S("\xC2"),         S("\xEF\xBF\xBD") },     // L2
	{ S("\xE0\x80\x41"), S("\xEF\xBF\xBD\xEF\xBF\xBD\x41") },     // L3 C A
	{ S("\xE0\x80"),     S("\xEF\xBF\xBD\xEF\xBF\xBD") },         // L3 C
	{ S("\xF0\x80\x80"), S("\xEF\xBF\xBD\xEF\xBF\xBD\xEF\xBF\xBD") },  // L4 C C
	{ S("\xC2\xE0"),     S("\xEF\xBF\xBD\xEF\xBF\xBD") },              // L2 L3
	{ S("\xE0\xC2\x80"), S("\xEF\xBF\xBD\xC2\x80") },                  // L3 L2 C

	{ S("\xF5"),     S("\xEF\xBF\xBD") },    // L4
	{ S("\xF8"),     S("\xEF\xBF\xBD") },    // L5
	{ S("\xFC"),     S("\xEF\xBF\xBD") },    // L6
	{ S("\xFE"),     S("\xEF\xBF\xBD") },
	{ S("\xFF"),     S("\xEF\xBF\xBD") },

};


TEST(encodings, utf8)
{
	for (auto test : utf8_tests)
	{
		string output;
		decode(test.input, encoding::utf_8, output);
		EXPECT_EQ(output, test.output);
	}
}

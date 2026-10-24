// Copyright 2011 Google Inc. All Rights Reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
// Author: jdtang@google.com (Jonathan Tang)

#include "char_ref.h"

#include <assert.h>
#include <ctype.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>  // Only for debug assertions at present.

#include "error.h"
#include "string_piece.h"
#include "utf8.h"
#include "util.h"

enum {
  GUMBO_NAMED_CHAR_REF_MAX_LEN = 32
};

struct GumboInternalParser;

const int kGumboNoChar = -1;

// Table of replacement characters.  The spec specifies that any occurrence of
// the first character should be replaced by the second character, and a parse
// error recorded.
typedef struct {
  int from_char;
  int to_char;
} CharReplacement;

static const CharReplacement kCharReplacements[] = {{0x00, 0xfffd},
    {0x0d, 0x000d}, {0x80, 0x20ac}, {0x81, 0x0081}, {0x82, 0x201A},
    {0x83, 0x0192}, {0x84, 0x201E}, {0x85, 0x2026}, {0x86, 0x2020},
    {0x87, 0x2021}, {0x88, 0x02C6}, {0x89, 0x2030}, {0x8A, 0x0160},
    {0x8B, 0x2039}, {0x8C, 0x0152}, {0x8D, 0x008D}, {0x8E, 0x017D},
    {0x8F, 0x008F}, {0x90, 0x0090}, {0x91, 0x2018}, {0x92, 0x2019},
    {0x93, 0x201C}, {0x94, 0x201D}, {0x95, 0x2022}, {0x96, 0x2013},
    {0x97, 0x2014}, {0x98, 0x02DC}, {0x99, 0x2122}, {0x9A, 0x0161},
    {0x9B, 0x203A}, {0x9C, 0x0153}, {0x9D, 0x009D}, {0x9E, 0x017E},
    {0x9F, 0x0178},
    // Terminator.
    {-1, -1}};

static int parse_digit(int c, bool allow_hex) {
  if (c >= '0' && c <= '9') {
    return c - '0';
  }
  if (allow_hex && c >= 'a' && c <= 'f') {
    return c - 'a' + 10;
  }
  if (allow_hex && c >= 'A' && c <= 'F') {
    return c - 'A' + 10;
  }
  return -1;
}

static void add_no_digit_error(
    struct GumboInternalParser* parser, Utf8Iterator* input) {
  GumboError* error = gumbo_add_error(parser);
  if (!error) {
    return;
  }
  utf8iterator_fill_error_at_mark(input, error);
  error->type = GUMBO_ERR_NUMERIC_CHAR_REF_NO_DIGITS;
}

static void add_codepoint_error(struct GumboInternalParser* parser,
    Utf8Iterator* input, GumboErrorType type, int codepoint) {
  GumboError* error = gumbo_add_error(parser);
  if (!error) {
    return;
  }
  utf8iterator_fill_error_at_mark(input, error);
  error->type = type;
  error->v.codepoint = codepoint;
}

static void add_named_reference_error(struct GumboInternalParser* parser,
    Utf8Iterator* input, GumboErrorType type, GumboStringPiece text) {
  GumboError* error = gumbo_add_error(parser);
  if (!error) {
    return;
  }
  utf8iterator_fill_error_at_mark(input, error);
  error->type = type;
  error->v.text = text;
}

static int maybe_replace_codepoint(int codepoint) {
  for (int i = 0; kCharReplacements[i].from_char != -1; ++i) {
    if (kCharReplacements[i].from_char == codepoint) {
      return kCharReplacements[i].to_char;
    }
  }
  return -1;
}

static bool consume_numeric_ref(
    struct GumboInternalParser* parser, Utf8Iterator* input, int* output) {
  utf8iterator_next(input);
  bool is_hex = false;
  int c = utf8iterator_current(input);
  if (c == 'x' || c == 'X') {
    is_hex = true;
    utf8iterator_next(input);
    c = utf8iterator_current(input);
  }

  int digit = parse_digit(c, is_hex);
  if (digit == -1) {
    // First digit was invalid; add a parse error and return.
    add_no_digit_error(parser, input);
    utf8iterator_reset(input);
    *output = kGumboNoChar;
    return false;
  }

  unsigned int codepoint = 0;
  bool status = true;
  do {
    codepoint = (codepoint * (is_hex ? 16 : 10)) + digit;
    utf8iterator_next(input);
    digit = parse_digit(utf8iterator_current(input), is_hex);
  } while (digit != -1);

  if (utf8iterator_current(input) != ';') {
    add_codepoint_error(
        parser, input, GUMBO_ERR_NUMERIC_CHAR_REF_WITHOUT_SEMICOLON, codepoint);
    status = false;
  } else {
    utf8iterator_next(input);
  }

  int replacement = maybe_replace_codepoint(codepoint);
  if (replacement != -1) {
    add_codepoint_error(
        parser, input, GUMBO_ERR_NUMERIC_CHAR_REF_INVALID, codepoint);
    *output = replacement;
    return false;
  }

  if ((codepoint >= 0xd800 && codepoint <= 0xdfff) || codepoint > 0x10ffff) {
    add_codepoint_error(
        parser, input, GUMBO_ERR_NUMERIC_CHAR_REF_INVALID, codepoint);
    *output = 0xfffd;
    return false;
  }

  if (utf8_is_invalid_code_point(codepoint) || codepoint == 0xb) {
    add_codepoint_error(
        parser, input, GUMBO_ERR_NUMERIC_CHAR_REF_INVALID, codepoint);
    status = false;
    // But return it anyway, per spec.
  }
  *output = codepoint;
  return status;
}

static bool maybe_add_invalid_named_reference(
    struct GumboInternalParser* parser, Utf8Iterator* input) {
  // The iterator will always be reset in this code path, so we don't need to
  // worry about consuming characters.
  const char* start = utf8iterator_get_char_pointer(input);
  int c = utf8iterator_current(input);
  while ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
         (c >= '0' && c <= '9')) {
    utf8iterator_next(input);
    c = utf8iterator_current(input);
  }
  if (c == ';') {
    GumboStringPiece bad_ref;
    bad_ref.data = start;
    bad_ref.length = utf8iterator_get_char_pointer(input) - start;
    add_named_reference_error(
        parser, input, GUMBO_ERR_NAMED_CHAR_REF_INVALID, bad_ref);
    return false;
  }
  return true;
}

static const char* find_named_ref(
    const char* start, const char* end, OneOrTwoCodepoints* output) {
  const char* p = start;
  const char* best_end = NULL;
  OneOrTwoCodepoints best = { kGumboNoChar, kGumboNoChar };

  while (p < end && (size_t) (p - start) < GUMBO_NAMED_CHAR_REF_MAX_LEN) {
    const unsigned char c = (unsigned char) *p;
    if (!isalnum(c) && c != ';') {
      break;
    }

    ++p;

    const struct GumboNamedCharRef* ref = gumbo_named_char_ref_find(start, (size_t)(p - start));
    if (ref) {
      best.first = ref->first;
      best.second = ref->second;
      best_end = p;
    }

    if (c == ';') {
      break;
    }
  }

  if (!best_end) {
    return NULL;
  }

  *output = best;
  return best_end;
}


static bool consume_named_ref(
    struct GumboInternalParser* parser, Utf8Iterator* input, bool is_in_attribute,
    OneOrTwoCodepoints* output) {
  assert(output->first == kGumboNoChar);
  const char* start = utf8iterator_get_char_pointer(input);
  const char* end = utf8iterator_get_end_pointer(input);
  const char* match_end = find_named_ref(start, end, output);

  if (!match_end) {
    output->first = kGumboNoChar;
    output->second = kGumboNoChar;
    bool status = maybe_add_invalid_named_reference(parser, input);
    utf8iterator_reset(input);
    return status;
  }

  assert(output->first != kGumboNoChar);
  char last_char = *(match_end - 1);
  int len = match_end - start;
  if (last_char == ';') {
    bool matched = utf8iterator_maybe_consume_match(input, start, len, true);
    assert(matched);
    return true;
  } else if (is_in_attribute &&
             match_end < end &&
             (*match_end == '=' || isalnum((unsigned char) *match_end))) {
    output->first = kGumboNoChar;
    output->second = kGumboNoChar;
    utf8iterator_reset(input);
    return true;
  } else {
    GumboStringPiece bad_ref;
    bad_ref.length = match_end - start;
    bad_ref.data = start;
    add_named_reference_error(
        parser, input, GUMBO_ERR_NAMED_CHAR_REF_WITHOUT_SEMICOLON, bad_ref);
    bool matched = utf8iterator_maybe_consume_match(input, start, len, true);
    assert(matched);
    return false;
  }
}

bool consume_char_ref(struct GumboInternalParser* parser,
    struct GumboInternalUtf8Iterator* input, int additional_allowed_char,
    bool is_in_attribute, OneOrTwoCodepoints* output) {
  utf8iterator_mark(input);
  utf8iterator_next(input);
  int c = utf8iterator_current(input);
  output->first = kGumboNoChar;
  output->second = kGumboNoChar;
  if (c == additional_allowed_char) {
    utf8iterator_reset(input);
    output->first = kGumboNoChar;
    return true;
  }
  switch (utf8iterator_current(input)) {
    case '\t':
    case '\n':
    case '\f':
    case ' ':
    case '<':
    case '&':
    case -1:
      utf8iterator_reset(input);
      return true;
    case '#':
      return consume_numeric_ref(parser, input, &output->first);
    default:
      return consume_named_ref(parser, input, is_in_attribute, output);
  }
}

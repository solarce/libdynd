//
// Copyright (C) 2011-15 DyND Developers
// BSD 2-Clause License, see LICENSE.txt
//

#include <string>
#include <climits>

#include <dynd/parser_util.hpp>
#include <dynd/func/callable.hpp>
#include <dynd/string_encodings.hpp>
#include <dynd/types/option_type.hpp>
#include <dynd/kernels/assignment_kernels.hpp>

using namespace std;
using namespace dynd;

// [a-zA-Z_][a-zA-Z0-9_]*
bool parse::parse_name_no_ws(const char *&rbegin, const char *end, const char *&out_strbegin, const char *&out_strend)
{
  const char *begin = rbegin;
  if (begin == end) {
    return false;
  }
  if (('a' <= *begin && *begin <= 'z') || ('A' <= *begin && *begin <= 'Z') || *begin == '_') {
    ++begin;
  }
  else {
    return false;
  }
  while (begin < end && (('a' <= *begin && *begin <= 'z') || ('A' <= *begin && *begin <= 'Z') ||
                         ('0' <= *begin && *begin <= '9') || *begin == '_')) {
    ++begin;
  }
  out_strbegin = rbegin;
  out_strend = begin;
  rbegin = begin;
  return true;
}

// [a-zA-Z]+
bool parse::parse_alpha_name_no_ws(const char *&rbegin, const char *end, const char *&out_strbegin,
                                   const char *&out_strend)
{
  const char *begin = rbegin;
  if (begin == end) {
    return false;
  }
  if (('a' <= *begin && *begin <= 'z') || ('A' <= *begin && *begin <= 'Z')) {
    ++begin;
  }
  else {
    return false;
  }
  while (begin < end && (('a' <= *begin && *begin <= 'z') || ('A' <= *begin && *begin <= 'Z'))) {
    ++begin;
  }
  out_strbegin = rbegin;
  out_strend = begin;
  rbegin = begin;
  return true;
}

bool parse::parse_doublequote_string_no_ws(const char *&rbegin, const char *end, const char *&out_strbegin,
                                           const char *&out_strend, bool &out_escaped)
{
  bool escaped = false;
  const char *begin = rbegin;
  if (!parse_token_no_ws(begin, end, '\"')) {
    return false;
  }
  for (;;) {
    if (begin == end) {
      throw parse::parse_error(rbegin, "string has no ending quote");
    }
    char c = *begin++;
    if (c == '\\') {
      escaped = true;
      if (begin == end) {
        throw parse::parse_error(rbegin, "string has no ending quote");
      }
      c = *begin++;
      switch (c) {
      case '"':
      case '\\':
      case '/':
      case 'b':
      case 'f':
      case 'n':
      case 'r':
      case 't':
        break;
      case 'u': {
        if (end - begin < 4) {
          throw parse::parse_error(begin - 2, "invalid unicode escape sequence in string");
        }
        for (int i = 0; i < 4; ++i) {
          char d = *begin++;
          if (!(('0' <= d && d <= '9') || ('A' <= d && d <= 'F') || ('a' <= d && d <= 'f'))) {
            throw parse::parse_error(begin - 1, "invalid unicode escape sequence in string");
          }
        }
        break;
      }
      case 'U': {
        if (end - begin < 8) {
          throw parse::parse_error(begin - 2, "invalid unicode escape sequence in string");
        }
        for (int i = 0; i < 8; ++i) {
          char d = *begin++;
          if (!(('0' <= d && d <= '9') || ('A' <= d && d <= 'F') || ('a' <= d && d <= 'f'))) {
            throw parse::parse_error(begin - 1, "invalid unicode escape sequence in string");
          }
        }
        break;
      }
      default:
        throw parse::parse_error(begin - 2, "invalid escape sequence in string");
      }
    }
    else if (c == '"') {
      out_strbegin = rbegin + 1;
      out_strend = begin - 1;
      out_escaped = escaped;
      rbegin = begin;
      return true;
    }
  }
}

void parse::unescape_string(const char *strbegin, const char *strend, std::string &out)
{
  out.resize(0);
  while (strbegin < strend) {
    char c = *strbegin++;
    if (c == '\\') {
      if (strbegin == strend) {
        return;
      }
      c = *strbegin++;
      switch (c) {
      case '"':
      case '\\':
      case '/':
        out += c;
        break;
      case 'b':
        out += '\b';
        break;
      case 'f':
        out += '\f';
        break;
      case 'n':
        out += '\n';
        break;
      case 'r':
        out += '\r';
        break;
      case 't':
        out += '\t';
        break;
      case 'u': {
        if (strend - strbegin < 4) {
          return;
        }
        uint32_t cp = 0;
        for (int i = 0; i < 4; ++i) {
          char d = *strbegin++;
          cp *= 16;
          if ('0' <= d && d <= '9') {
            cp += d - '0';
          }
          else if ('A' <= d && d <= 'F') {
            cp += d - 'A' + 10;
          }
          else if ('a' <= d && d <= 'f') {
            cp += d - 'a' + 10;
          }
          else {
            cp = '?';
          }
        }
        append_utf8_codepoint(cp, out);
        break;
      }
      case 'U': {
        if (strend - strbegin < 8) {
          return;
        }
        uint32_t cp = 0;
        for (int i = 0; i < 8; ++i) {
          char d = *strbegin++;
          cp *= 16;
          if ('0' <= d && d <= '9') {
            cp += d - '0';
          }
          else if ('A' <= d && d <= 'F') {
            cp += d - 'A' + 10;
          }
          else if ('a' <= d && d <= 'f') {
            cp += d - 'a' + 10;
          }
          else {
            cp = '?';
          }
        }
        append_utf8_codepoint(cp, out);
        break;
      }
      default:
        out += '?';
      }
    }
    else {
      out += c;
    }
  }
}

bool parse::parse_json_number_no_ws(const char *&rbegin, const char *end, const char *&out_nbegin,
                                    const char *&out_nend)
{
  const char *begin = rbegin;
  if (begin == end) {
    return false;
  }
  // Optional minus sign
  if (*begin == '-') {
    ++begin;
  }
  if (begin == end) {
    return false;
  }
  // Either '0' or a non-zero digit followed by digits
  if (*begin == '0') {
    ++begin;
  }
  else if ('1' <= *begin && *begin <= '9') {
    ++begin;
    while (begin < end && ('0' <= *begin && *begin <= '9')) {
      ++begin;
    }
  }
  else {
    return false;
  }
  // Optional decimal point, followed by one or more digits
  if (begin < end && *begin == '.') {
    if (++begin == end) {
      return false;
    }
    if (!('0' <= *begin && *begin <= '9')) {
      return false;
    }
    ++begin;
    while (begin < end && ('0' <= *begin && *begin <= '9')) {
      ++begin;
    }
  }
  // Optional exponent, followed by +/- and some digits
  if (begin < end && (*begin == 'e' || *begin == 'E')) {
    if (++begin == end) {
      return false;
    }
    // +/- is optional
    if (*begin == '+' || *begin == '-') {
      if (++begin == end) {
        return false;
      }
    }
    // At least one digit is required
    if (!('0' <= *begin && *begin <= '9')) {
      return false;
    }
    ++begin;
    while (begin < end && ('0' <= *begin && *begin <= '9')) {
      ++begin;
    }
  }
  out_nbegin = rbegin;
  out_nend = begin;
  rbegin = begin;
  return true;
}

// [0-9][0-9]
bool parse::parse_2digit_int_no_ws(const char *&begin, const char *end, int &out_val)
{
  if (end - begin >= 2) {
    int d0 = begin[0], d1 = begin[1];
    if (d0 >= '0' && d0 <= '9' && d1 >= '0' && d1 <= '9') {
      begin += 2;
      out_val = (d0 - '0') * 10 + (d1 - '0');
      return true;
    }
  }
  return false;
}

// [0-9][0-9]?
bool parse::parse_1or2digit_int_no_ws(const char *&begin, const char *end, int &out_val)
{
  if (end - begin >= 2) {
    int d0 = begin[0], d1 = begin[1];
    if (d0 >= '0' && d0 <= '9') {
      if (d1 >= '0' && d1 <= '9') {
        begin += 2;
        out_val = (d0 - '0') * 10 + (d1 - '0');
        return true;
      }
      else {
        ++begin;
        out_val = (d0 - '0');
        return true;
      }
    }
  }
  else if (end - begin == 1) {
    int d0 = begin[0];
    if (d0 >= '0' && d0 <= '9') {
      ++begin;
      out_val = (d0 - '0');
      return true;
    }
  }
  return false;
}

// [0-9][0-9][0-9][0-9]
bool parse::parse_4digit_int_no_ws(const char *&begin, const char *end, int &out_val)
{
  if (end - begin >= 4) {
    int d0 = begin[0], d1 = begin[1], d2 = begin[2], d3 = begin[3];
    if (d0 >= '0' && d0 <= '9' && d1 >= '0' && d1 <= '9' && d2 >= '0' && d2 <= '9' && d3 >= '0' && d3 <= '9') {
      begin += 4;
      out_val = (((d0 - '0') * 10 + (d1 - '0')) * 10 + (d2 - '0')) * 10 + (d3 - '0');
      return true;
    }
  }
  return false;
}

// [0-9][0-9][0-9][0-9][0-9][0-9]
bool parse::parse_6digit_int_no_ws(const char *&begin, const char *end, int &out_val)
{
  if (end - begin >= 6) {
    int d0 = begin[0], d1 = begin[1], d2 = begin[2], d3 = begin[3], d4 = begin[4], d5 = begin[5];
    if (d0 >= '0' && d0 <= '9' && d1 >= '0' && d1 <= '9' && d2 >= '0' && d2 <= '9' && d3 >= '0' && d3 <= '9' &&
        d4 >= '0' && d4 <= '9' && d5 >= '0' && d5 <= '9') {
      begin += 6;
      out_val =
          (((((d0 - '0') * 10 + (d1 - '0')) * 10 + (d2 - '0')) * 10 + (d3 - '0')) * 10 + (d4 - '0')) * 10 + (d5 - '0');
      return true;
    }
  }
  return false;
}

template <class T>
inline static T checked_string_to_uint(const char *begin, const char *end, bool &out_overflow, bool &out_badparse)
{
  T result = 0, prev_result = 0;
  if (begin == end) {
    out_badparse = true;
  }
  while (begin < end) {
    char c = *begin;
    if ('0' <= c && c <= '9') {
      result = (result * 10u) + (uint32_t)(c - '0');
      if (result < prev_result) {
        out_overflow = true;
      }
    }
    else {
      if (c == '.') {
        // Accept ".", ".0" with trailing decimal zeros as well
        ++begin;
        while (begin < end && *begin == '0') {
          ++begin;
        }
        if (begin == end) {
          break;
        }
      }
      else if (c == 'e' || c == 'E') {
        // Accept "1e5", "1e+5" integers with a positive exponent,
        // a subset of floating point syntax. Note that "1.2e1"
        // is not accepted as the value 12 by this code.
        ++begin;
        if (begin < end && *begin == '+') {
          ++begin;
        }
        if (begin < end) {
          int exponent = 0;
          // Accept any number of zeros followed by at most
          // two digits. Anything greater would overflow.
          while (begin < end && *begin == '0') {
            ++begin;
          }
          if (begin < end && '0' <= *begin && *begin <= '9') {
            exponent = *begin++ - '0';
          }
          if (begin < end && '0' <= *begin && *begin <= '9') {
            exponent = (10 * exponent) + (*begin++ - '0');
          }
          if (begin == end) {
            prev_result = result;
            // Apply the exponent in a naive way, but with
            // overflow checking
            for (int i = 0; i < exponent; ++i) {
              result = result * 10u;
              if (result < prev_result) {
                out_overflow = true;
              }
              prev_result = result;
            }
            return result;
          }
        }
      }
      out_badparse = true;
      break;
    }
    ++begin;
    prev_result = result;
  }
  return result;
}

template <class T>
inline static T unchecked_string_to_uint(const char *begin, const char *end)
{
  T result = 0;
  while (begin < end) {
    char c = *begin;
    if ('0' <= c && c <= '9') {
      result = (result * 10u) + (uint32_t)(c - '0');
    }
    else if (c == 'e' || c == 'E') {
      // Accept "1e5", "1e+5" integers with a positive exponent,
      // a subset of floating point syntax. Note that "1.2e1"
      // is not accepted as the value 12 by this code.
      ++begin;
      if (begin < end && *begin == '+') {
        ++begin;
      }
      if (begin < end) {
        int exponent = 0;
        // Accept any number of zeros followed by at most
        // two digits. Anything greater would overflow.
        while (begin < end && *begin == '0') {
          ++begin;
        }
        if (begin < end && '0' <= *begin && *begin <= '9') {
          exponent = *begin++ - '0';
        }
        if (begin < end && '0' <= *begin && *begin <= '9') {
          exponent = (10 * exponent) + (*begin++ - '0');
        }
        if (begin == end) {
          // Apply the exponent in a naive way
          for (int i = 0; i < exponent; ++i) {
            result = result * 10u;
          }
        }
      }
      break;
    }
    else {
      break;
    }
    ++begin;
  }
  return result;
}

uint64_t parse::checked_string_to_uint64(const char *begin, const char *end, bool &out_overflow, bool &out_badparse)
{
  return checked_string_to_uint<uint64_t>(begin, end, out_overflow, out_badparse);
}

uint128 parse::checked_string_to_uint128(const char *begin, const char *end, bool &out_overflow, bool &out_badparse)
{
  return checked_string_to_uint<uint128>(begin, end, out_overflow, out_badparse);
}

template <class T>
static T checked_string_to_signed_int(const char *begin, const char *end)
{
  bool negative = false, overflow = false, badparse = false;
  if (begin < end && *begin == '-') {
    negative = true;
    ++begin;
  }
  uint64_t uvalue = parse::checked_string_to_uint64(begin, end, overflow, badparse);
  if (overflow || parse::overflow_check<T>::is_overflow(uvalue, negative)) {
    stringstream ss;
    ss << "overflow converting string ";
    ss.write(begin, end - begin);
    ss << " to " << ndt::type::make<T>();
    throw overflow_error(ss.str());
  }
  else if (badparse) {
    stringstream ss;
    ss << "parse error converting string ";
    ss.write(begin, end - begin);
    ss << " to" << ndt::type::make<T>();
    throw invalid_argument(ss.str());
  }
  else {
    return negative ? -static_cast<T>(uvalue) : static_cast<T>(uvalue);
  }
}

intptr_t parse::checked_string_to_intptr(const char *begin, const char *end)
{
  return checked_string_to_signed_int<intptr_t>(begin, end);
}

int64_t parse::checked_string_to_int64(const char *begin, const char *end)
{
  return checked_string_to_signed_int<int64_t>(begin, end);
}

uint64_t parse::unchecked_string_to_uint64(const char *begin, const char *end)
{
  return unchecked_string_to_uint<uint64_t>(begin, end);
}

uint128 parse::unchecked_string_to_uint128(const char *begin, const char *end)
{
  return unchecked_string_to_uint<uint128>(begin, end);
}

inline static double make_double_nan(bool negative)
{
  union {
    uint64_t i;
    double d;
  } nan;
  nan.i = negative ? 0xfff8000000000000ULL : 0x7ff8000000000000ULL;
  return nan.d;
}

double parse::checked_string_to_float64(const char *begin, const char *end, assign_error_mode errmode)
{
  bool negative = false;
  const char *pos = begin;
  if (pos < end && *pos == '-') {
    negative = true;
    ++pos;
  }
  // First check for various NaN/Inf inputs
  size_t size = end - pos;
  if (size == 3) {
    if ((pos[0] == 'N' || pos[0] == 'n') && (pos[1] == 'A' || pos[1] == 'a') && (pos[2] == 'N' || pos[2] == 'n')) {
      return make_double_nan(negative);
    }
    else if ((pos[0] == 'I' || pos[0] == 'i') && (pos[1] == 'N' || pos[1] == 'n') && (pos[2] == 'F' || pos[2] == 'f')) {
      return negative ? -numeric_limits<double>::infinity() : numeric_limits<double>::infinity();
    }
  }
  else if (size == 7) {
    if ((pos[0] == '1') && (pos[1] == '.') && (pos[2] == '#') && (pos[3] == 'Q' || pos[3] == 'q') &&
        (pos[4] == 'N' || pos[4] == 'n') && (pos[5] == 'A' || pos[5] == 'a') && (pos[6] == 'N' || pos[6] == 'n')) {
      return make_double_nan(negative);
    }
  }
  else if (size == 6) {
    if ((pos[0] == '1') && (pos[1] == '.') && (pos[2] == '#')) {
      if ((pos[3] == 'I' || pos[3] == 'i') && (pos[4] == 'N' || pos[4] == 'n') && (pos[5] == 'D' || pos[5] == 'd')) {
        return make_double_nan(negative);
      }
      else if ((pos[3] == 'I' || pos[3] == 'i') && (pos[4] == 'N' || pos[4] == 'n') &&
               (pos[5] == 'F' || pos[5] == 'f')) {
        return negative ? -numeric_limits<double>::infinity() : numeric_limits<double>::infinity();
      }
    }
  }
  else if (size == 8) {
    if ((pos[0] == 'I' || pos[0] == 'i') && (pos[1] == 'N' || pos[1] == 'n') && (pos[2] == 'F' || pos[2] == 'f') &&
        (pos[3] == 'I' || pos[3] == 'i') && (pos[4] == 'N' || pos[4] == 'n') && (pos[5] == 'I' || pos[5] == 'i') &&
        (pos[6] == 'T' || pos[6] == 't') && (pos[7] == 'Y' || pos[7] == 'y')) {
      return negative ? -numeric_limits<double>::infinity() : numeric_limits<double>::infinity();
    }
  }

  // TODO: use http://www.netlib.org/fp/dtoa.c
  char *end_ptr;
  std::string s(begin, end);
  double value = strtod(s.c_str(), &end_ptr);
  if (errmode != assign_error_nocheck && (size_t)(end_ptr - s.c_str()) != s.size()) {
    stringstream ss;
    ss << "parse error converting string ";
    print_escaped_utf8_string(ss, begin, end);
    ss << " to float64";
    throw invalid_argument(ss.str());
  }

  return value;
}

template <class T>
static inline void assign_signed_int_value(char *out_int, uint64_t uvalue, bool &negative, bool &overflow,
                                           bool &badparse)
{
  overflow = overflow || parse::overflow_check<T>::is_overflow(uvalue, negative);
  if (!overflow && !badparse) {
    *reinterpret_cast<T *>(out_int) =
        static_cast<T>(negative ? -static_cast<int64_t>(uvalue) : static_cast<int64_t>(uvalue));
  }
}

static inline void assign_signed_int128_value(char *out_int, uint128 uvalue, bool &negative, bool &overflow,
                                              bool &badparse)
{
  overflow = overflow || parse::overflow_check<int128>::is_overflow(uvalue, negative);
  if (!overflow && !badparse) {
    *reinterpret_cast<int128 *>(out_int) = negative ? -static_cast<int128>(uvalue) : static_cast<int128>(uvalue);
  }
}

template <class T>
static inline void assign_unsigned_int_value(char *out_int, uint64_t uvalue, bool &negative, bool &overflow,
                                             bool &badparse)
{
  overflow = overflow || negative || parse::overflow_check<T>::is_overflow(uvalue);
  if (!overflow && !badparse) {
    *reinterpret_cast<T *>(out_int) = static_cast<T>(uvalue);
  }
}

static float checked_float64_to_float32(double value, assign_error_mode errmode)
{
  union {
    float result;
    char dst[4];
  } out;
  char *src[1] = {reinterpret_cast<char *>(&value)};
  switch (errmode) {
  case assign_error_nocheck:
    dynd::nd::detail::assignment_kernel<float32_type_id, real_kind, float64_type_id, real_kind,
                                        assign_error_nocheck>::single_wrapper(NULL, reinterpret_cast<char *>(&out.dst),
                                                                              src);
    break;
  case assign_error_overflow:
    dynd::nd::detail::assignment_kernel<float32_type_id, real_kind, float64_type_id, real_kind,
                                        assign_error_overflow>::single_wrapper(NULL, reinterpret_cast<char *>(&out.dst),
                                                                               src);
    break;
  case assign_error_fractional:
    dynd::nd::detail::assignment_kernel<float32_type_id, real_kind, float64_type_id, real_kind,
                                        assign_error_fractional>::single_wrapper(NULL,
                                                                                 reinterpret_cast<char *>(&out.dst),
                                                                                 src);
    break;
  case assign_error_inexact:
    dynd::nd::detail::assignment_kernel<float32_type_id, real_kind, float64_type_id, real_kind,
                                        assign_error_inexact>::single_wrapper(NULL, reinterpret_cast<char *>(&out.dst),
                                                                              src);
    break;
  default:
    dynd::nd::detail::assignment_kernel<float32_type_id, real_kind, float64_type_id, real_kind,
                                        assign_error_fractional>::single_wrapper(NULL,
                                                                                 reinterpret_cast<char *>(&out.dst),
                                                                                 src);
    break;
  }
  return out.result;
}

void parse::string_to_number(char *out, type_id_t tid, const char *begin, const char *end, bool option,
                             assign_error_mode errmode)
{
  uint64_t uvalue;
  const char *saved_begin = begin;
  bool negative = false, overflow = false, badparse = false;

  if (option && matches_option_type_na_token(begin, end)) {
    switch (tid) {
    case int8_type_id:
      *reinterpret_cast<int8_t *>(out) = DYND_INT8_NA;
      return;
    case int16_type_id:
      *reinterpret_cast<int16_t *>(out) = DYND_INT16_NA;
      return;
    case int32_type_id:
      *reinterpret_cast<int32_t *>(out) = DYND_INT32_NA;
      return;
    case int64_type_id:
      *reinterpret_cast<int64_t *>(out) = DYND_INT64_NA;
      return;
    case int128_type_id:
      *reinterpret_cast<int128 *>(out) = DYND_INT128_NA;
      return;
    case float16_type_id:
      *reinterpret_cast<uint16_t *>(out) = DYND_FLOAT16_NA_AS_UINT;
      return;
    case float32_type_id:
      *reinterpret_cast<uint32_t *>(out) = DYND_FLOAT32_NA_AS_UINT;
      return;
    case float64_type_id:
      *reinterpret_cast<uint64_t *>(out) = DYND_FLOAT64_NA_AS_UINT;
      return;
    case complex_float32_type_id:
      reinterpret_cast<uint32_t *>(out)[0] = DYND_FLOAT32_NA_AS_UINT;
      reinterpret_cast<uint32_t *>(out)[1] = DYND_FLOAT32_NA_AS_UINT;
      return;
    case complex_float64_type_id:
      reinterpret_cast<uint64_t *>(out)[0] = DYND_FLOAT64_NA_AS_UINT;
      reinterpret_cast<uint64_t *>(out)[1] = DYND_FLOAT64_NA_AS_UINT;
      return;
    default:
      break;
    }
    stringstream ss;
    ss << "No NA value has been configured for option[" << ndt::type(tid) << "]";
    throw type_error(ss.str());
  }

  if (begin < end && *begin == '-') {
    negative = true;
    ++begin;
  }
  if (errmode != assign_error_nocheck) {
    switch (tid) {
    case int8_type_id:
      uvalue = parse::checked_string_to_uint64(begin, end, overflow, badparse);
      assign_signed_int_value<int8_t>(out, uvalue, negative, overflow, badparse);
      break;
    case int16_type_id:
      uvalue = parse::checked_string_to_uint64(begin, end, overflow, badparse);
      assign_signed_int_value<int16_t>(out, uvalue, negative, overflow, badparse);
      break;
    case int32_type_id:
      uvalue = parse::checked_string_to_uint64(begin, end, overflow, badparse);
      assign_signed_int_value<int32_t>(out, uvalue, negative, overflow, badparse);
      break;
    case int64_type_id:
      uvalue = parse::checked_string_to_uint64(begin, end, overflow, badparse);
      assign_signed_int_value<int64_t>(out, uvalue, negative, overflow, badparse);
      break;
    case int128_type_id: {
      uint128 buvalue = parse::checked_string_to_uint128(begin, end, overflow, badparse);
      assign_signed_int128_value(out, buvalue, negative, overflow, badparse);
      break;
    }
    case uint8_type_id:
      uvalue = parse::checked_string_to_uint64(begin, end, overflow, badparse);
      negative = negative && (uvalue != 0);
      assign_unsigned_int_value<uint8_t>(out, uvalue, negative, overflow, badparse);
      break;
    case uint16_type_id:
      uvalue = parse::checked_string_to_uint64(begin, end, overflow, badparse);
      negative = negative && (uvalue != 0);
      assign_unsigned_int_value<uint16_t>(out, uvalue, negative, overflow, badparse);
      break;
    case uint32_type_id:
      uvalue = parse::checked_string_to_uint64(begin, end, overflow, badparse);
      negative = negative && (uvalue != 0);
      assign_unsigned_int_value<uint32_t>(out, uvalue, negative, overflow, badparse);
      break;
    case uint64_type_id:
      uvalue = parse::checked_string_to_uint64(begin, end, overflow, badparse);
      negative = negative && (uvalue != 0);
      overflow = overflow || negative;
      if (!overflow && !badparse) {
        *reinterpret_cast<uint64_t *>(out) = uvalue;
      }
      break;
    case uint128_type_id: {
      uint128 buvalue = parse::checked_string_to_uint128(begin, end, overflow, badparse);
      negative = negative && (buvalue != 0);
      overflow = overflow || negative;
      if (!overflow && !badparse) {
        *reinterpret_cast<uint128 *>(out) = buvalue;
      }
      break;
    }
    case float16_type_id: {
      double value = checked_string_to_float64(saved_begin, end, errmode);
      *reinterpret_cast<uint16_t *>(out) = float16(value).bits();
      break;
    }
    case float32_type_id: {
      double value = checked_string_to_float64(saved_begin, end, errmode);
      *reinterpret_cast<float *>(out) = checked_float64_to_float32(value, errmode);
      break;
    }
    case float64_type_id: {
      *reinterpret_cast<double *>(out) = checked_string_to_float64(saved_begin, end, errmode);
      break;
    }
    default: {
      stringstream ss;
      ss << "cannot parse number, got invalid type id " << tid;
      throw runtime_error(ss.str());
    }
    }
    if (overflow) {
      stringstream ss;
      ss << "overflow converting string ";
      print_escaped_utf8_string(ss, begin, end);
      ss << " to ";
      if (option) {
        ss << "?";
      }
      ss << tid;
      throw overflow_error(ss.str());
    }
    else if (badparse) {
      stringstream ss;
      ss << "parse error converting string ";
      print_escaped_utf8_string(ss, begin, end);
      ss << " to ";
      if (option) {
        ss << "?";
      }
      ss << tid;
      throw invalid_argument(ss.str());
    }
  }
  else {
    // errmode == assign_error_nocheck
    switch (tid) {
    case int8_type_id:
      uvalue = parse::unchecked_string_to_uint64(begin, end);
      *reinterpret_cast<int8_t *>(out) =
          static_cast<int8_t>(negative ? -static_cast<int64_t>(uvalue) : static_cast<int64_t>(uvalue));
      break;
    case int16_type_id:
      uvalue = parse::unchecked_string_to_uint64(begin, end);
      *reinterpret_cast<int16_t *>(out) =
          static_cast<int16_t>(negative ? -static_cast<int64_t>(uvalue) : static_cast<int64_t>(uvalue));
      break;
    case int32_type_id:
      uvalue = parse::unchecked_string_to_uint64(begin, end);
      *reinterpret_cast<int32_t *>(out) =
          static_cast<int32_t>(negative ? -static_cast<int64_t>(uvalue) : static_cast<int64_t>(uvalue));
      break;
    case int64_type_id:
      uvalue = parse::unchecked_string_to_uint64(begin, end);
      *reinterpret_cast<int64_t *>(out) = negative ? -static_cast<int64_t>(uvalue) : static_cast<int64_t>(uvalue);
      break;
    case int128_type_id: {
      uint128 buvalue = parse::unchecked_string_to_uint128(begin, end);
      *reinterpret_cast<int128 *>(out) = negative ? -static_cast<int128>(buvalue) : static_cast<int128>(buvalue);
      break;
    }
    case uint8_type_id:
      uvalue = parse::unchecked_string_to_uint64(begin, end);
      *reinterpret_cast<uint8_t *>(out) = static_cast<uint8_t>(negative ? 0 : uvalue);
      break;
    case uint16_type_id:
      uvalue = parse::unchecked_string_to_uint64(begin, end);
      *reinterpret_cast<uint16_t *>(out) = static_cast<uint16_t>(negative ? 0 : uvalue);
      break;
    case uint32_type_id:
      uvalue = parse::unchecked_string_to_uint64(begin, end);
      *reinterpret_cast<uint32_t *>(out) = static_cast<uint32_t>(negative ? 0 : uvalue);
      break;
    case uint64_type_id:
      uvalue = parse::unchecked_string_to_uint64(begin, end);
      *reinterpret_cast<uint64_t *>(out) = negative ? 0 : uvalue;
      break;
    case uint128_type_id: {
      uint128 buvalue = parse::unchecked_string_to_uint128(begin, end);
      *reinterpret_cast<uint128 *>(out) = negative ? static_cast<uint128>(0) : buvalue;
      break;
    }
    case float16_type_id: {
      double value = checked_string_to_float64(saved_begin, end, errmode);
      *reinterpret_cast<uint16_t *>(out) = float16(value).bits();
      break;
    }
    case float32_type_id: {
      double value = checked_string_to_float64(saved_begin, end, errmode);
      *reinterpret_cast<float *>(out) = checked_float64_to_float32(value, errmode);
      break;
    }
    case float64_type_id: {
      *reinterpret_cast<double *>(out) = checked_string_to_float64(saved_begin, end, errmode);
      break;
    }
    default: {
      stringstream ss;
      ss << "cannot parse number, got invalid type id " << tid;
      throw runtime_error(ss.str());
    }
    }
  }
}

void parse::string_to_bool(char *out_bool, const char *begin, const char *end, bool option, assign_error_mode errmode)
{
  if (option && matches_option_type_na_token(begin, end)) {
    *out_bool = DYND_BOOL_NA;
    return;
  }
  else {
    size_t size = end - begin;
    if (size == 1) {
      char c = *begin;
      if (c == '0' || c == 'n' || c == 'N' || c == 'f' || c == 'F') {
        *out_bool = 0;
        return;
      }
      else if (errmode == assign_error_nocheck || c == '1' || c == 'y' || c == 'Y' || c == 't' || c == 'T') {
        *out_bool = 1;
        return;
      }
    }
    else if (size == 4) {
      if (errmode == assign_error_nocheck) {
        *out_bool = 1;
        return;
      }
      else if ((begin[0] == 'T' || begin[0] == 't') && (begin[1] == 'R' || begin[1] == 'r') &&
               (begin[2] == 'U' || begin[2] == 'u') && (begin[3] == 'E' || begin[3] == 'e')) {
        *out_bool = 1;
        return;
      }
    }
    else if (size == 5) {
      if ((begin[0] == 'F' || begin[0] == 'f') && (begin[1] == 'A' || begin[1] == 'a') &&
          (begin[2] == 'L' || begin[2] == 'l') && (begin[3] == 'S' || begin[3] == 's') &&
          (begin[4] == 'E' || begin[4] == 'e')) {
        *out_bool = 0;
        return;
      }
      else if (errmode == assign_error_nocheck) {
        *out_bool = 1;
        return;
      }
    }
    else if (size == 0) {
      if (errmode == assign_error_nocheck) {
        *out_bool = 0;
        return;
      }
    }
    else if (size == 2) {
      if ((begin[0] == 'N' || begin[0] == 'n') && (begin[1] == 'O' || begin[1] == 'o')) {
        *out_bool = 0;
        return;
      }
      else if (errmode == assign_error_nocheck ||
               ((begin[0] == 'O' || begin[0] == 'o') && (begin[1] == 'N' || begin[1] == 'n'))) {
        *out_bool = 1;
        return;
      }
    }
    else if (size == 3) {
      if ((begin[0] == 'O' || begin[0] == 'o') && (begin[1] == 'F' || begin[1] == 'f') &&
          (begin[2] == 'F' || begin[2] == 'f')) {
        *out_bool = 0;
        return;
      }
      else if (errmode == assign_error_nocheck ||
               ((begin[0] == 'Y' || begin[0] == 'y') && (begin[1] == 'E' || begin[1] == 'e') &&
                (begin[2] == 'S' || begin[2] == 's'))) {
        *out_bool = 1;
        return;
      }
    }
  }

  stringstream ss;
  ss << "cannot cast string ";
  print_escaped_utf8_string(ss, begin, end);
  if (option) {
    ss << " to ?bool";
  }
  else {
    ss << " to bool";
  }
  throw invalid_argument(ss.str());
}

bool parse::matches_option_type_na_token(const char *begin, const char *end)
{
  size_t size = end - begin;
  if (size == 0) {
    return true;
  }
  else if (size == 2) {
    if (begin[0] == 'N' && begin[1] == 'A') {
      return true;
    }
  }
  else if (size == 4) {
    if (((begin[0] == 'N' || begin[0] == 'n') && (begin[1] == 'U' || begin[1] == 'u') &&
         (begin[2] == 'L' || begin[2] == 'l') && (begin[3] == 'L' || begin[3] == 'l'))) {
      return true;
    }
    if (begin[0] == 'N' && begin[1] == 'o' && begin[2] == 'n' && begin[3] == 'e') {
      return true;
    }
  }

  return false;
}

int dynd::parse_uint64(uint64_t &res, const char *begin, const char *end)
{
  bool out_overflow = false, out_badparse = false;
  res = parse::checked_string_to_uint64(begin, end, out_overflow, out_badparse);

  return out_overflow || out_badparse;
}

int dynd::parse_int64(int64_t &res, const char *begin, const char *end)
{
  bool negative = false;
  if (begin < end && *begin == '-') {
    negative = true;
    ++begin;
  }

  uint64_t ures;
  int val = parse_uint64(ures, begin, end);

  res = ures;
  if (negative) {
    res = -res;
  }

  return val;
}

int dynd::parse_double(double &res, const char *begin, const char *end)
{
  try {
    res = parse::checked_string_to_float64(begin, end, assign_error_nocheck);
  }
  catch (...) {
    return 1;
  }

  return 0;
}

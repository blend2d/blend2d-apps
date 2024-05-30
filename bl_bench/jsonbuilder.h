#ifndef BLBENCH_JSONBUILDER_H_INCLUDED
#define BLBENCH_JSONBUILDER_H_INCLUDED

#include <blend2d.h>

namespace blbench {

class JSONBuilder {
public:
  enum Token : uint32_t {
    kTokenNone  = 0,
    kTokenValue = 1
  };

  explicit JSONBuilder(BLString* dst);

  JSONBuilder& openArray();
  JSONBuilder& closeArray(bool nl = false);

  JSONBuilder& openObject();
  JSONBuilder& closeObject(bool nl = false);

  JSONBuilder& comma();

  JSONBuilder& addKey(const char* str);

  JSONBuilder& addBool(bool b);
  JSONBuilder& addInt(int64_t n);
  JSONBuilder& addUInt(uint64_t n);
  JSONBuilder& addDouble(double d);
  JSONBuilder& addDoublef(const char* fmt, double d);
  JSONBuilder& addString(const char* str);
  JSONBuilder& addStringf(const char* fmt, ...);
  JSONBuilder& addStringWithoutQuotes(const char* str);

  JSONBuilder& alignTo(size_t n);
  JSONBuilder& beforeRecord();

  JSONBuilder& nl() { _dst->append('\n'); return *this; }
  JSONBuilder& indent() { _dst->append(' ', _level); return *this; }

  BLString* _dst {};
  uint32_t _last {};
  uint32_t _level {};
};

} // {blbench} namespace

#endif // BLBENCH_JSONBUILDER_H_INCLUDED

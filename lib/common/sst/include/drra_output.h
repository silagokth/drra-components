#include <iostream>

#include <sst/core/output.h>

using namespace SST;

class DRRAOutput : public Output {
private:
  std::string prefix;

public:
  DRRAOutput(const std::string &prefix = "") : prefix(prefix) {}

  void setPrefix(const std::string &new_prefix) { prefix = new_prefix; }

  template <typename... Args> void output(const char *format, Args... args) {
    std::string prefixed_format = prefix + format;
    if constexpr (sizeof...(args) == 0) {
      Output::output("%s", prefixed_format.c_str());
    } else {
      Output::output(prefixed_format.c_str(), args...);
    }
    std::cout.flush();
  }

  template <typename... Args>
  void fatal(uint32_t line, const char *file, const char *func, int exit_code,
             const char *format, Args... args) {
    std::string prefixed_format = prefix + format;
    if constexpr (sizeof...(args) == 0) {
      Output::fatal(line, file, func, exit_code, "%s", prefixed_format.c_str());
    } else {
      Output::fatal(line, file, func, exit_code, prefixed_format.c_str(),
                    args...);
    }
    std::cout.flush();
  }

  template <typename... Args>
  void verbose(uint32_t line, const char *file, const char *func,
               uint32_t output_level, uint32_t output_bits, const char *format,
               Args... args) {
    std::string prefixed_format = prefix + format;
    if constexpr (sizeof...(args) == 0) {
      Output::verbose(line, file, func, output_level, output_bits, "%s",
                      prefixed_format.c_str());
    } else {
      Output::verbose(line, file, func, output_level, output_bits,
                      prefixed_format.c_str(), args...);
    }
  }

  template <typename... Args> void print(const char *format, Args... args) {
    if constexpr (sizeof...(args) == 0) {
      Output::output("%s", format);
    } else {
      Output::output(format, args...);
    }
  }
};

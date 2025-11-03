#ifndef TRACE_EVENT_H
#define TRACE_EVENT_H

#include <iomanip>
#include <map>
#include <sstream>
#include <string>

class TraceEvent {
public:
  // Constructor for complete events (phase X)
  // Simplified with default values for common cases
  TraceEvent(const std::string &name, long long timestamp,
             long long duration = 1, int process_id = 0, int thread_id = 0)
      : name_(name), category_("event"), timestamp_(timestamp),
        duration_(duration), process_id_(process_id), thread_id_(thread_id) {}

  // Set category (default is "event")
  TraceEvent &setCategory(const std::string &category) {
    category_ = category;
    return *this;
  }

  // Set thread ID with formatted coordinates
  // Example: setThreadId(1, 123, 45, 6) creates thread ID "1123045006"
  TraceEvent &setThreadId(int base, int coord0, int coord1, int slot) {
    std::ostringstream oss;
    oss << base << std::setw(3) << std::setfill('0') << coord0 << std::setw(3)
        << std::setfill('0') << coord1 << std::setw(3) << std::setfill('0')
        << slot;
    thread_id_str_ = oss.str();
    use_thread_id_str_ = true;
    return *this;
  }

  // Set simple thread ID
  TraceEvent &setThreadId(int tid) {
    thread_id_ = tid;
    use_thread_id_str_ = false;
    return *this;
  }

  // Set process ID
  TraceEvent &setProcessId(int pid) {
    process_id_ = pid;
    return *this;
  }

  // Add a string argument
  TraceEvent &addArg(const std::string &key, const std::string &value) {
    args_[key] = "\"" + escapeJson(value) + "\"";
    return *this;
  }

  // Add a numeric argument (int)
  TraceEvent &addArg(const std::string &key, int value) {
    args_[key] = std::to_string(value);
    return *this;
  }

  // Add a numeric argument (long long)
  TraceEvent &addArg(const std::string &key, long long value) {
    args_[key] = std::to_string(value);
    return *this;
  }

  // Add a numeric argument (double)
  TraceEvent &addArg(const std::string &key, double value) {
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(6) << value;
    args_[key] = oss.str();
    return *this;
  }

  // Add a boolean argument
  TraceEvent &addArg(const std::string &key, bool value) {
    args_[key] = value ? "true" : "false";
    return *this;
  }

  // Export to JSON string (without trailing newline or comma)
  std::string toJson() const {
    std::ostringstream json;
    json << "{\"name\": \"" << escapeJson(name_) << "\", \"cat\": \""
         << escapeJson(category_) << "\", \"ph\": \"X\", \"ts\": " << timestamp_
         << ", \"dur\": " << duration_ << ", \"tid\": ";

    if (use_thread_id_str_) {
      json << thread_id_str_;
    } else {
      json << thread_id_;
    }

    json << ", \"pid\": " << process_id_;

    if (!args_.empty()) {
      json << ", \"args\": {";
      bool first = true;
      for (const auto &arg : args_) {
        if (!first)
          json << ", ";
        json << "\"" << escapeJson(arg.first) << "\": " << arg.second;
        first = false;
      }
      json << "}";
    }

    json << "}";
    return json.str();
  }

  // Export to JSON string with trailing comma and newline (for streaming to
  // file)
  std::string toJsonLine() const { return toJson() + ",\n"; }

private:
  std::string name_;
  std::string category_;
  long long timestamp_;
  long long duration_;
  int process_id_;
  int thread_id_;
  std::string thread_id_str_;
  bool use_thread_id_str_ = false;
  std::map<std::string, std::string> args_;

  // Escape special characters for JSON
  std::string escapeJson(const std::string &str) const {
    std::ostringstream escaped;
    for (char c : str) {
      switch (c) {
      case '"':
        escaped << "\\\"";
        break;
      case '\\':
        escaped << "\\\\";
        break;
      case '\b':
        escaped << "\\b";
        break;
      case '\f':
        escaped << "\\f";
        break;
      case '\n':
        escaped << "\\n";
        break;
      case '\r':
        escaped << "\\r";
        break;
      case '\t':
        escaped << "\\t";
        break;
      default:
        if (c < 32) {
          escaped << "\\u" << std::hex << std::setw(4) << std::setfill('0')
                  << static_cast<int>(c);
        } else {
          escaped << c;
        }
      }
    }
    return escaped.str();
  }
};

#endif // TRACE_EVENT_H

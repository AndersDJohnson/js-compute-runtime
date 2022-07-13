#ifndef JS_COMPUTE_RUNTIME_LOGGER_H
#define JS_COMPUTE_RUNTIME_LOGGER_H

#include "builtin.h"

namespace builtins {

class Logger final : public BuiltinNoConstructor<Logger> {
public:
  enum Slots { Endpoint, Count };

  static constexpr const char *class_name = "Logger";

  static const JSPropertySpec properties[];
  static const JSFunctionSpec methods[];

  // Create an instance of the Logger class.
  static JSObject *create(JSContext *cx, const char *name);
};

} // namespace builtins

#endif

#ifndef JS_COMPUTE_RUNTIME_BUILTIN_H
#define JS_COMPUTE_RUNTIME_BUILTIN_H

#include "../xqd.h"
#include <optional>

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Winvalid-offsetof"

#include "js/CallArgs.h"
#include "jsapi.h"
#include "jsfriendapi.h"

#pragma clang diagnostic pop

#include "js/Value.h"

class MethodHelpers final {
private:
  ~MethodHelpers() = delete;

public:
  template <typename Impl>
  static inline bool check_receiver(JSContext *cx, JS::HandleValue receiver,
                                    const char *method_name) {
    if (!Impl::is_instance(receiver)) {
      JS_ReportErrorUTF8(cx, "Method %s called on receiver that's not an instance of %s\n",
                         method_name, Impl::class_.name);
      return false;
    }
    return true;
  }
};

template <typename Impl> class BuiltinImpl {
private:
  static constexpr const JSClassOps class_ops{};
  static constexpr const uint32_t class_flags = 0;

public:
  static constexpr JSClass class_{
      Impl::class_name,
      JSCLASS_HAS_RESERVED_SLOTS(Impl::Slots::Count) | class_flags,
      &class_ops,
  };

  static JS::PersistentRooted<JSObject *> proto_obj;

  static bool is_instance(JSObject *obj) { return !!obj && JS::GetClass(obj) == &class_; }

  static bool is_instance(JS::Value val) { return val.isObject() && is_instance(&val.toObject()); }

  static bool init_class_impl(JSContext *cx, JS::HandleObject global,
                              JS::HandleObject parent_proto = nullptr) {
    proto_obj.init(cx, JS_InitClass(cx, global, parent_proto, &class_, Impl::constructor,
                                    Impl::ctor_length, Impl::properties, Impl::methods, nullptr,
                                    nullptr));

    return proto_obj != nullptr;
  }
};

template <typename Impl> JS::PersistentRooted<JSObject *> BuiltinImpl<Impl>::proto_obj{};

template <typename Impl> class BuiltinNoConstructor : public BuiltinImpl<Impl> {
public:
  static constexpr const int ctor_length = 1;

  static bool constructor(JSContext *cx, unsigned argc, JS::Value *vp) {
    JS_ReportErrorUTF8(cx, "%s can't be instantiated directly", Impl::class_name);
    return false;
  }

  static bool init_class(JSContext *cx, JS::HandleObject global) {
    return BuiltinNoConstructor<Impl>::init_class_impl(cx, global) &&
           JS_DeleteProperty(cx, global, BuiltinImpl<Impl>::class_.name);
  }
};

namespace {

inline bool handle_fastly_result(JSContext *cx, int result, int line, const char *func) {
  switch (result) {
  case 0:
    return true;
  case 1:
    JS_ReportErrorUTF8(cx,
                       "%s: Generic error value. This means that some unexpected error "
                       "occurred during a hostcall. - Fastly error code %d\n",
                       func, result);
    return false;
  case 2:
    JS_ReportErrorUTF8(cx, "%s: Invalid argument. - Fastly error code %d\n", func, result);
    return false;
  case 3:
    JS_ReportErrorUTF8(cx,
                       "%s: Invalid handle. Thrown when a request, response, dictionary, or "
                       "body handle is not valid. - Fastly error code %d\n",
                       func, result);
    return false;
  case 4:
    JS_ReportErrorUTF8(cx, "%s: Buffer length error. Buffer is too long. - Fastly error code %d\n",
                       func, result);
    return false;
  case 5:
    JS_ReportErrorUTF8(cx,
                       "%s: Unsupported operation error. This error is thrown "
                       "when some operation cannot be performed, because it is "
                       "not supported. - Fastly error code %d\n",
                       func, result);
    return false;
  case 6:
    JS_ReportErrorUTF8(cx,
                       "%s: Alignment error. This is thrown when a pointer does not point to "
                       "a properly aligned slice of memory. - Fastly error code %d\n",
                       func, result);
    return false;
  case 7:
    JS_ReportErrorUTF8(cx,
                       "%s: HTTP parse error. This can be thrown when a method, URI, header, "
                       "or status is not valid. This can also be thrown if a message head is "
                       "too large. - Fastly error code %d\n",
                       func, result);
    return false;
  case 8:
    JS_ReportErrorUTF8(cx,
                       "%s: HTTP user error. This is thrown in cases where user code caused "
                       "an HTTP error. For example, attempt to send a 1xx response code, or a "
                       "request with a non-absolute URI. This can also be caused by an "
                       "unexpected header: both `content-length` and `transfer-encoding`, for "
                       "example. - Fastly error code %d\n",
                       func, result);
    return false;
  case 9:
    JS_ReportErrorUTF8(cx,
                       "%s: HTTP incomplete message error. A stream ended "
                       "unexpectedly. - Fastly error code %d\n",
                       func, result);
    return false;
  case 10:
    JS_ReportErrorUTF8(cx,
                       "%s: A `None` error. This status code is used to "
                       "indicate when an optional value did not exist, as "
                       "opposed to an empty value. - Fastly error code %d\n",
                       func, result);
    return false;
  case 11:
    JS_ReportErrorUTF8(cx,
                       "%s: HTTP head too large error. This error will be thrown when the "
                       "message head is too large. - Fastly error code %d\n",
                       func, result);
    return false;
  case 12:
    JS_ReportErrorUTF8(cx,
                       "%s: HTTP invalid status error. This error will be "
                       "thrown when the HTTP message contains an invalid "
                       "status code. - Fastly error code %d\n",
                       func, result);
    return false;
  default:
    fprintf(stdout, __FILE__ ":%d (%s) - Fastly error code %d\n", line, func, result);
    JS_ReportErrorUTF8(cx, "Fastly error code %d", result);
    return false;
  }
}

#define HANDLE_RESULT(cx, result) handle_fastly_result(cx, result, __LINE__, __func__)

} // namespace

#endif

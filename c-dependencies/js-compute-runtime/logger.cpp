
#include "logger.h"
#include "../xqd.h"

#include "js/Conversions.h"

namespace builtins {

namespace {

// TODO: introduce a version that writes into an existing buffer, and use that
// with the hostcall buffer where possible.
JS::UniqueChars encode(JSContext *cx, JS::HandleString str, size_t *encoded_len) {
  JS::UniqueChars text = JS_EncodeStringToUTF8(cx, str);
  if (!text)
    return nullptr;

  // This shouldn't fail, since the encode operation ensured `str` is linear.
  JSLinearString *linear = JS_EnsureLinearString(cx, str);
  *encoded_len = JS::GetDeflatedUTF8StringLength(linear);
  return text;
}

JS::UniqueChars encode(JSContext *cx, JS::HandleValue val, size_t *encoded_len) {
  JS::RootedString str(cx, JS::ToString(cx, val));
  if (!str)
    return nullptr;

  return encode(cx, str, encoded_len);
}

#define METHOD_HEADER_WITH_NAME(Impl, required_argc, name)                                         \
  JS::CallArgs args = JS::CallArgsFromVp(argc, vp);                                                \
  if (!MethodHelpers::check_receiver<Impl>(cx, args.thisv(), name))                                \
    return false;                                                                                  \
  JS::RootedObject self(cx, &args.thisv().toObject());                                             \
  if (!args.requireAtLeast(cx, name, required_argc))                                               \
    return false;

#define METHOD_HEADER(Impl, required_argc) METHOD_HEADER_WITH_NAME(Impl, required_argc, __func__)

bool log(JSContext *cx, unsigned argc, JS::Value *vp) {
  METHOD_HEADER(Logger, 1);

  auto endpoint =
      LogEndpointHandle{(uint32_t)JS::GetReservedSlot(self, Logger::Slots::Endpoint).toInt32()};

  size_t msg_len;
  JS::UniqueChars msg = encode(cx, args.get(0), &msg_len);
  if (!msg)
    return false;

  size_t nwritten;
  if (!HANDLE_RESULT(cx, xqd_log_write(endpoint, msg.get(), msg_len, &nwritten)))
    return false;

  args.rval().setUndefined();
  return true;
}
} // namespace

const JSFunctionSpec Logger::methods[] = {JS_FN("log", log, 1, JSPROP_ENUMERATE), JS_FS_END};
const JSPropertySpec Logger::properties[] = {JS_PS_END};

JSObject *Logger::create(JSContext *cx, const char *name) {
  JS::RootedObject logger(cx, JS_NewObjectWithGivenProto(cx, &class_, proto_obj));
  if (!logger)
    return nullptr;

  auto handle = LogEndpointHandle{INVALID_HANDLE};

  if (!HANDLE_RESULT(cx, xqd_log_endpoint_get(name, strlen(name), &handle)))
    return nullptr;

  JS::SetReservedSlot(logger, Slots::Endpoint, JS::Int32Value(handle.handle));

  return logger;
}

} // namespace builtins

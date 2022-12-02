#include "config-store.h"
#include "host_call.h"

namespace builtins {

fastly_dictionary_handle_t ConfigStore::config_store_handle(JSObject *obj) {
  JS::Value val = JS::GetReservedSlot(obj, ConfigStore::Slots::Handle);
  return static_cast<fastly_dictionary_handle_t>(val.toInt32());
}

bool ConfigStore::get(JSContext *cx, unsigned argc, JS::Value *vp) {
  METHOD_HEADER(1)

  xqd_world_string_t key_str;
  JS::UniqueChars key = encode(cx, args[0], &key_str.len);
  key_str.ptr = key.get();

  fastly_option_string_t ret;
  auto status = convert_to_fastly_status(
      xqd_fastly_dictionary_get(ConfigStore::config_store_handle(self), &key_str, &ret));

  // Ensure that we throw an exception for all unexpected host errors.
  if (!HANDLE_RESULT(cx, status))
    return false;

  if (!ret.is_some) {
    args.rval().setNull();
    return true;
  }

  JS::RootedString text(cx, JS_NewStringCopyUTF8N(cx, JS::UTF8Chars(ret.val.ptr, ret.val.len)));
  JS_free(cx, ret.val.ptr);
  if (!text)
    return false;

  args.rval().setString(text);
  return true;
}

const JSFunctionSpec ConfigStore::methods[] = {JS_FN("get", get, 1, JSPROP_ENUMERATE), JS_FS_END};

const JSPropertySpec ConfigStore::properties[] = {JS_PS_END};

bool ConfigStore::constructor(JSContext *cx, unsigned argc, JS::Value *vp) {
  REQUEST_HANDLER_ONLY("The ConfigStore builtin");
  CTOR_HEADER("ConfigStore", 1);

  xqd_world_string_t name_str;
  JS::UniqueChars name = encode(cx, args[0], &name_str.len);
  name_str.ptr = name.get();

  JS::RootedObject config_store(cx, JS_NewObjectForConstructor(cx, &class_, args));
  fastly_dictionary_handle_t dict_handle = INVALID_HANDLE;
  if (!HANDLE_RESULT(cx, xqd_fastly_dictionary_open(&name_str, &dict_handle)))
    return false;

  JS::SetReservedSlot(config_store, ConfigStore::Slots::Handle, JS::Int32Value(dict_handle));
  if (!config_store)
    return false;
  args.rval().setObject(*config_store);
  return true;
}

bool ConfigStore::init_class(JSContext *cx, JS::HandleObject global) {
  return BuiltinImpl<ConfigStore>::init_class_impl(cx, global);
}

} // namespace builtins

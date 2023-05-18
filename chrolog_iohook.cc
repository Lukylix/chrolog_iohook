#include <napi.h>
#include <thread>
#include <node_api.h> // Add this line to include node_api.h for node-addon-api
#include <iostream>
#include <atomic>
namespace chrolog_iohook
{
  int main(int argc, char **argv, Napi::ThreadSafeFunction tsfn_mouse, Napi::ThreadSafeFunction tsfn_keyboard);
}

class ChrologIOhook : public Napi::ObjectWrap<ChrologIOhook>
{
public:
  static Napi::Object Init(Napi::Env env, Napi::Object exports);
  ChrologIOhook(const Napi::CallbackInfo &info);

private:
  static Napi::FunctionReference constructor;
  Napi::ThreadSafeFunction tsfn_mouse;
  Napi::ThreadSafeFunction tsfn_keyboard;
  std::thread nativeThread;
  Napi::Value SetMouseCallback(const Napi::CallbackInfo &info);
  Napi::Value SetKeyboardCallback(const Napi::CallbackInfo &info);
  Napi::Value Log(const Napi::CallbackInfo &info);

  // ... other methods ...
};

Napi::FunctionReference ChrologIOhook::constructor;

Napi::Object ChrologIOhook::Init(Napi::Env env, Napi::Object exports)
{
  Napi::HandleScope scope(env);

  Napi::Function func = DefineClass(env, "ChrologIOhook", {
                                                              InstanceMethod("setMouseCallback", &ChrologIOhook::SetMouseCallback),
                                                              InstanceMethod("setKeyboardCallback", &ChrologIOhook::SetKeyboardCallback),
                                                              InstanceMethod("log", &ChrologIOhook::Log),
                                                          });

  constructor = Napi::Persistent(func);
  constructor.SuppressDestruct();

  exports.Set("ChrologIOhook", func);

  return exports;
}

class LogWorker : public Napi::AsyncWorker
{
public:
  LogWorker(const Napi::Function &callback, Napi::ThreadSafeFunction tsfn_mouse, Napi::ThreadSafeFunction tsfn_keyboard)
      : Napi::AsyncWorker(callback), tsfn_mouse(tsfn_mouse), tsfn_keyboard(tsfn_keyboard) {}

  void Execute() override
  {
    int argc = 2;
    const char *argv[] = {"chrolog_iohook", "-s"}; // Provide the necessary command-line arguments for chrolog_iohook

    int result = chrolog_iohook::main(argc, const_cast<char **>(argv), tsfn_mouse, tsfn_keyboard);

    // ...
  }

  // ...

private:
  Napi::ThreadSafeFunction tsfn_mouse;
  Napi::ThreadSafeFunction tsfn_keyboard;
};

Napi::Value ChrologIOhook::Log(const Napi::CallbackInfo &info)
{
  Napi::Env env = info.Env();

  if (info.Length() != 0)
  {
    Napi::TypeError::New(env, "No arguments expected").ThrowAsJavaScriptException();
    return env.Undefined();
  }

  Napi::Promise::Deferred deferred = Napi::Promise::Deferred::New(env);

  LogWorker *worker = new LogWorker(Napi::Function::New(env, [](const Napi::CallbackInfo &info) {}), tsfn_mouse, tsfn_keyboard);

  worker->Queue();

  return deferred.Promise();
}

ChrologIOhook::ChrologIOhook(const Napi::CallbackInfo &info) : Napi::ObjectWrap<ChrologIOhook>(info)
{
  // constructor implementation
}

Napi::Value ChrologIOhook::SetMouseCallback(const Napi::CallbackInfo &info)
{
  Napi::Env env = info.Env();
  if (info.Length() != 1 || !info[0].IsFunction())
  {
    Napi::TypeError::New(env, "Expected a function").ThrowAsJavaScriptException();
    return env.Undefined();
  }

  tsfn_mouse = Napi::ThreadSafeFunction::New(
      env,
      info[0].As<Napi::Function>(), // JavaScript function called asynchronously
      "SetMouseCallback",           // Name
      0,                            // Unlimited queue
      1,                            // Only one thread will use this initially
      [this](Napi::Env) {           // Capture the 'this' pointer
        nativeThread.join();
        tsfn_mouse.Release();
      });
  return env.Undefined();
}

Napi::Value ChrologIOhook::SetKeyboardCallback(const Napi::CallbackInfo &info)
{
  Napi::Env env = info.Env();
  if (info.Length() != 1 || !info[0].IsFunction())
  {
    Napi::TypeError::New(env, "Expected a function").ThrowAsJavaScriptException();
    return env.Undefined();
  }

  tsfn_keyboard = Napi::ThreadSafeFunction::New(
      env,
      info[0].As<Napi::Function>(), // JavaScript function called asynchronously
      "SetKeyboardCallback",        // Name
      0,                            // Unlimited queue
      1,                            // Only one thread will use this initially
      [this](Napi::Env) {           // Capture the 'this' pointer
        nativeThread.join();
        tsfn_keyboard.Release();
      });

  return env.Undefined();
}

Napi::Object InitAddon(Napi::Env env, Napi::Object exports)
{
  return ChrologIOhook::Init(env, exports);
}

NODE_API_MODULE(NODE_GYP_MODULE_NAME, InitAddon)

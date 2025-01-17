#include <node.h>
#include "../core/api_drivers/web_api.h"
#include "../core/json.hpp"

using namespace v8;
using json = nlohmann::json;

web_api api;

void get_responses_from_controller(const FunctionCallbackInfo<Value>& args){
  Isolate* isolate = args.GetIsolate();
  json response;

  unsigned int ret = api.get_completed_calls(&response);

  if(ret != 0){
    isolate->ThrowException(Exception::TypeError(
    String::NewFromUtf8(isolate, "Error getting completed calls").ToLocalChecked()));
  }

  // Convert the nlohmann::json object to a string
    std::string jsonString = response.dump();

    // Use V8's JSON::Parse to convert the JSON string into a V8 object
    Local<String> v8String = String::NewFromUtf8(isolate, jsonString.c_str()).ToLocalChecked();
    Local<Context> context = isolate->GetCurrentContext();
    Local<Value> v8JsonObject;
    if (!JSON::Parse(context, v8String).ToLocal(&v8JsonObject)) {
        isolate->ThrowException(Exception::SyntaxError(
            String::NewFromUtf8(isolate, "Failed to parse JSON").ToLocalChecked()));
        return;
    }

    // Return the V8 JSON object to Node.js
    args.GetReturnValue().Set(v8JsonObject);

  return;
}

void send_data_to_controller(const FunctionCallbackInfo<Value>& args){
  Isolate* isolate = args.GetIsolate();

  unsigned int ret = api.write_web_calls_to_controller();

  if(ret != 0){
    isolate->ThrowException(Exception::TypeError(
    String::NewFromUtf8(isolate, "Failed to write data to controller").ToLocalChecked()));
    return;
  }

  return;
}

void api_call(const FunctionCallbackInfo<Value>& args) {
    Isolate* isolate = args.GetIsolate();

    // Variable to hold the resulting string
    std::string jsonString;

    // Check if the first argument is a string
    if (args[0]->IsString()) {
        // Convert the V8 string to a C++ std::string
        String::Utf8Value utf8(isolate, args[0]);
        jsonString = std::string(*utf8);
    } 
    // Check if the first argument is an object
    else if (args[0]->IsObject()) {
        Local<Context> context = isolate->GetCurrentContext();
        Local<Object> obj = args[0]->ToObject(context).ToLocalChecked();

        // Use JSON::Stringify to convert the object to a JSON string
        Local<String> jsonStr;
        Local<Value> jsonObj = JSON::Stringify(context, obj).ToLocalChecked();

        // Convert the V8 JSON string to a C++ std::string
        String::Utf8Value utf8Json(isolate, jsonObj);
        jsonString = std::string(*utf8Json);
    } 
    else {
        // If the argument is neither a string nor an object, throw an error
        isolate->ThrowException(Exception::TypeError(String::NewFromUtf8(isolate, "Argument must be a string or object").ToLocalChecked()));
        return;
    }

    json parsedJson;
    parsedJson = json::parse(jsonString);

    unsigned int ret = 0;
    unsigned int call_number = 0;

    ret = api.add_call(&parsedJson, &call_number);

    if(ret != 0){
        isolate->ThrowException(Exception::TypeError(
        String::NewFromUtf8(isolate, "Failed to add call").ToLocalChecked()));
        return;
    }

    args.GetReturnValue().Set(Number::New(isolate, call_number));

  return;
}

void Initialize(Local<Object> exports) {
  NODE_SET_METHOD(exports, "api_call", api_call);
  NODE_SET_METHOD(exports, "send_data", send_data_to_controller);
  NODE_SET_METHOD(exports, "get_responses", get_responses_from_controller);
}

NODE_MODULE(NODE_GYP_MODULE_NAME, Initialize)
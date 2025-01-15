#include <node.h>
#include <string.h>
#include <api_structs.h>

using namespace v8;

class API_Helper {
  public:
    void set_args(const FunctionCallbackInfo<Value>& args){
        isolate = args.GetIsolate();
    }

    void check_arg_count(int count){
        if (args.Length() < count) {
            isolate->ThrowException(Exception::TypeError(String::NewFromUtf8(isolate, "Missing argument(s)").ToLocalChecked()));
            throw std::invalid_argument("Missing argument(s)");
        }
    }

    std::string get_string_arg(int index){
        check_arg_count(index);

        if (!args[index]->IsString()) {
            isolate->ThrowException(Exception::TypeError(String::NewFromUtf8(isolate, "Could not parse argument as string").ToLocalChecked()));
            throw std::invalid_argument("Could not parse argument as string");
        }
        String::Utf8Value utf8_value(isolate, args[index]);
        return std::string(*utf8_value);
    }

    Local<Object> get_object_arg(int index){
        check_arg_count(index);
        // Check if the first argument is an object
        if (!args[index]->IsObject()) {
            isolate->ThrowException(Exception::TypeError(
                String::NewFromUtf8(isolate, "Expected an object").ToLocalChecked()));
            throw std::invalid_argument("Expected an object");
        }

        // Convert the first argument to a V8 Object
        Local<Object> obj = args[index]->ToObject(isolate->GetCurrentContext()).ToLocalChecked();
        return obj;
    }

  private:
    Isolate* isolate;

    
};
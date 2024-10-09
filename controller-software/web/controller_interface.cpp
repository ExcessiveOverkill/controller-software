#include <node.h>
#include <string.h>
#include <thread>
#include <chrono>
#include <api_helper.h>

using namespace v8;


int get_shared_addr_to_write_to(unsigned int size, unsigned int &addr, unsigned int &block) {
  block = 0;  // section of shared memory used for this API call
  return 1; // fails if unable to get shared memory address
}

bool shared_memory_updated(unsigned int block){ // return true when the specified block has been updated by the controller
  return true;
}

void api_call(const FunctionCallbackInfo<Value>& args) {
    API_Helper api_helper;
    api_helper.set_args(args);

    Isolate* isolate = args.GetIsolate();
    //HandleScope scope(isolate);

    // Check if the first argument is an object
    if (args.Length() < 1 || !args[0]->IsObject()) {
        isolate->ThrowException(Exception::TypeError(
            String::NewFromUtf8(isolate, "Expected an object").ToLocalChecked()));
        return;
    }

    // Convert the first argument to a V8 Object
    Local<Object> obj = args[0]->ToObject(isolate->GetCurrentContext()).ToLocalChecked();

    // Get the property names (keys) from the object
    Local<Array> propertyNames = obj->GetPropertyNames(isolate->GetCurrentContext()).ToLocalChecked();

    // Loop over the properties
    for (uint32_t i = 0; i < propertyNames->Length(); i++) {
        // Get the property name (key)
        Local<Value> key = propertyNames->Get(isolate->GetCurrentContext(), i).ToLocalChecked();
        Local<Value> value = obj->Get(isolate->GetCurrentContext(), key).ToLocalChecked();

        // Convert key and value to UTF8 strings
        String::Utf8Value keyStr(isolate, key);
        String::Utf8Value valueStr(isolate, value);

        // Print the key-value pair
        //std::cout << *keyStr << ": " << *valueStr << std::endl;
    }
}


// void api_call(const FunctionCallbackInfo<Value>& args) {
//   Isolate* isolate = args.GetIsolate();
//   Local<Context> context = isolate->GetCurrentContext();
//   Local<Object> ret_obj = Object::New(isolate);
//   ret_obj->Set(context, String::NewFromUtf8(isolate, "success").ToLocalChecked(), Boolean::New(isolate, false)).FromJust();
//   ret_obj->Set(context, String::NewFromUtf8(isolate, "error").ToLocalChecked(), String::NewFromUtf8(isolate, "").ToLocalChecked()).FromJust();

//   // Check the number of arguments passed.
//   if (args.Length() < 1) {
//     isolate->ThrowException(Exception::TypeError(String::NewFromUtf8(isolate, "No arguments found").ToLocalChecked()));
//     return;
//   }

//   if (!args[0]->IsString()) {
//     isolate->ThrowException(Exception::TypeError(String::NewFromUtf8(isolate, "Could not parse API call name as string").ToLocalChecked()));
//     return;
//   }

//     // get API call name
//     String::Utf8Value utf8_value(isolate, args[0]);
//     std::string api_call_name(*utf8_value);

//   if (api_call_name.compare("machine_state") != 0) {

//     // check arguments
//     if (args.Length() > 2) {
//       isolate->ThrowException(Exception::TypeError(String::NewFromUtf8(isolate, "Wrong number of arguments").ToLocalChecked()));
//       return;
//     }
    
//     call = new machine_state;

//     if (args.Length() == 2) {

//       if (!args[1]->IsString()) {
//       isolate->ThrowException(Exception::TypeError(String::NewFromUtf8(isolate, "Wrong argument type for [commanded_state]").ToLocalChecked()));
//       return;
//       }

//       String::Utf8Value utf8_value(isolate, args[1]);
//       std::string commanded_state(*utf8_value);

//       if (commanded_state.compare("") == 0) {
//         call->command_state = machine_state::NONE;
//       }
//       else if (commanded_state.compare("on") == 0) {
//         call->command_state = machine_state::ON;
//       }
//       else if (commanded_state.compare("off") == 0) {
//         call->command_state = machine_state::OFF;
//       }
//       else {
//         isolate->ThrowException(Exception::TypeError(String::NewFromUtf8(isolate, "Invalid commanded state").ToLocalChecked()));
//         return;
//       }
//     }

//     // get memory address we can safely write to
//     unsigned int addr, block;
//     if(get_shared_addr_to_write_to(sizeof(machine_state), addr, block) != 0){ // fails if unable to get shared memory address
//       isolate->ThrowException(Exception::TypeError(String::NewFromUtf8(isolate, "Unable to get shared memory address").ToLocalChecked()));
//       return;
//     }

//     //memcpy((void*)addr, call, sizeof(machine_state), sizeof(machine_state));  // copy to shared memory

//     for(int i = 0; i < 10; i++){ // wait for controller to update shared memory
//       if(shared_memory_updated(block)){
//         break;
//       }
//       std::sleep(1);
//       if(i == 9){
//         isolate->ThrowException(Exception::TypeError(String::NewFromUtf8(isolate, "Controller did not respond in time").ToLocalChecked()));
//         return;
//       }
//     }

//   }

//   ret_obj->Set(context, String::NewFromUtf8(isolate, "success").ToLocalChecked(), Boolean::New(isolate, true)).FromJust();
//   args.GetReturnValue().Set(ret_obj);
//   return;
// }

// void Add(const FunctionCallbackInfo<Value>& args) {
//   Isolate* isolate = args.GetIsolate();

//   // Check the number of arguments passed.
//   if (args.Length() < 2) {
//     // Throw an Error that is passed back to JavaScript
//     isolate->ThrowException(Exception::TypeError(
//         String::NewFromUtf8(isolate,
//                             "Wrong number of arguments").ToLocalChecked()));
//     return;
//   }

//   // Check the argument types
//   if (!args[0]->IsNumber() || !args[1]->IsNumber()) {
//     isolate->ThrowException(Exception::TypeError(
//         String::NewFromUtf8(isolate,
//                             "Wrong arguments").ToLocalChecked()));
//     return;
//   }

//   // Perform the operation
//   double value =
//       args[0].As<Number>()->Value() + args[1].As<Number>()->Value();
//   Local<Number> num = Number::New(isolate, value);

//   // Set the return value (using the passed in
//   // FunctionCallbackInfo<Value>&)
//   args.GetReturnValue().Set(num);
// }

void Init(Local<Object> exports) {
  NODE_SET_METHOD(exports, "api_call", api_call);
}

NODE_MODULE(NODE_GYP_MODULE_NAME, Init)
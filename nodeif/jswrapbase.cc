// Copyright Joyent, Inc. and other Node contributors.
//
// Permission is hereby granted, free of charge, to any person obtaining a
// copy of this software and associated documentation files (the
// "Software"), to deal in the Software without restriction, including
// without limitation the rights to use, copy, modify, merge, publish,
// distribute, sublicense, and/or sell copies of the Software, and to permit
// persons to whom the Software is furnished to do so, subject to the
// following conditions:
//
// The above copyright notice and this permission notice shall be included
// in all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
// OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
// MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN
// NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
// DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
// OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
// USE OR OTHER DEALINGS IN THE SOFTWARE.

#include "../common/std_headers.h"
#include "../common/jsonio.h"
#include "./jswrapbase.h"


bool fastJsonFlag;

Handle<Value> ThrowInvalidArgs() {
  return ThrowException(Exception::TypeError(String::New("Invalid arguments")));
}

Handle<Value> ThrowInvalidThis() {
  return ThrowException(Exception::TypeError(String::New("Invalid this, did you forget to call new?")));
}

Handle<Value> ThrowTypeError(char const *s) {
  return ThrowException(Exception::TypeError(String::New(s)));
}

Handle<Value> ThrowRuntimeError(char const *s) {
  return ThrowException(Exception::TypeError(String::New(s))); // WRITEME
}



bool canConvJsToString(Handle<Value> it) {
  return it->IsString() || node::Buffer::HasInstance(it);
}
string convJsToString(Handle<Value> it) {
  if (it->IsString()) {
    String::Utf8Value v8str(it);
    return string((char *) *v8str, v8str.length());
  }
  else if (node::Buffer::HasInstance(it)) {
    char *data = node::Buffer::Data(it);
    size_t len = node::Buffer::Length(it);
    return string(data, data+len);
  }
  else {
    throw runtime_error("Can't convert to string");
  }
}
Handle<Value> convStringToJs(string const &it) {
  return String::New(it.data(), it.size());
}
Handle<Value> convStringToJsBuffer(string const &it) {
  node::Buffer *buf = node::Buffer::New(it.data(), it.size());
  return Handle<Value>(buf->handle_);
}


/* ----------------------------------------------------------------------
   Convert JS arrays, both regular and native, to vector<double>
   See https://github.com/joyent/node/issues/4201 for details on native arrays
*/

bool canConvJsToVectorDouble(Handle<Value> itv) {
  if (itv->IsObject()) {
    Handle<Object> it = itv->ToObject();
    if (it->GetIndexedPropertiesExternalArrayDataType() == kExternalDoubleArray) return true;
    if (it->GetIndexedPropertiesExternalArrayDataType() == kExternalFloatArray) return true;
    if (it->IsArray()) return true;
  }
  return false;
}

vector<double> convJsToVectorDouble(Handle<Value> itv) {
  if (itv->IsObject()) {
    Handle<Object> it = itv->ToObject();

    if (it->GetIndexedPropertiesExternalArrayDataType() == kExternalDoubleArray) {
      size_t itLen = it->GetIndexedPropertiesExternalArrayDataLength();
      double* itData = static_cast<double*>(it->GetIndexedPropertiesExternalArrayData());

      return vector<double>(itData, itData+itLen);
    }

    if (it->GetIndexedPropertiesExternalArrayDataType() == kExternalFloatArray) {
      size_t itLen = it->GetIndexedPropertiesExternalArrayDataLength();
      float* itData = static_cast<float*>(it->GetIndexedPropertiesExternalArrayData());

      return vector<double>(itData, itData+itLen);
    }

    // Also handle regular JS arrays
    if (it->IsArray()) {
      Handle<Array> itArr = Handle<Array>::Cast(it);
      size_t itArrLen = itArr->Length();
      vector<double> ret(itArrLen);
      for (size_t i=0; i<itArrLen; i++) {
        ret[i] = itArr->Get(i)->NumberValue();
      }
      return ret;
    }
  }
  throw runtime_error("convJsToVectorDouble: not an array");
}

Handle<Object> convVectorDoubleToJs(vector<double> const &it) {
  static Persistent<Function> float64_array_constructor;

  if (float64_array_constructor.IsEmpty()) {
    Local<Object> global = Context::GetCurrent()->Global();
    Local<Value> val = global->Get(String::New("Float64Array"));
    assert(!val.IsEmpty() && "type not found: Float64Array");
    assert(val->IsFunction() && "not a constructor: Float64Array");
    float64_array_constructor = Persistent<Function>::New(val.As<Function>());
  }

  Local<Value> itSize = Integer::NewFromUnsigned((u_int)it.size());
  Local<Object> ret = float64_array_constructor->NewInstance(1, &itSize);
  assert(ret->GetIndexedPropertiesExternalArrayDataType() == kExternalDoubleArray);
  assert((size_t)ret->GetIndexedPropertiesExternalArrayDataLength() == it.size());

  double* retData = static_cast<double*>(ret->GetIndexedPropertiesExternalArrayData());
  memcpy(retData, &it[0], it.size() * sizeof(double));
  
  return ret;
}


static Persistent<Object> JSON;
static Persistent<Function> JSON_stringify;
static Persistent<Function> JSON_parse;

static void setupJSON() {
  if (JSON.IsEmpty()) {
    Local<Object> global = Context::GetCurrent()->Global();
    Local<Value> tmpJSON = global->Get(String::New("JSON"));
    assert(tmpJSON->IsObject());
    JSON = Persistent<Object>::New(tmpJSON->ToObject());
    
    Local<Value> tmpStringify = tmpJSON->ToObject()->Get(String::New("stringify"));
    assert(!tmpStringify.IsEmpty() && "function not found: JSON.stringify");
    assert(tmpStringify->IsFunction() && "not a function: JSON.stringify");
    JSON_stringify = Persistent<Function>::New(tmpStringify.As<Function>());
    
    Local<Value> tmpParse = tmpJSON->ToObject()->Get(String::New("parse"));
    assert(!tmpParse.IsEmpty() && "function not found: JSON.parse");
    assert(tmpParse->IsFunction() && "not a function: JSON.parse");
    JSON_parse = Persistent<Function>::New(tmpParse.As<Function>());
  }
}

jsonstr convJsToJsonstr(Handle<Value> value) {
  setupJSON();

  if (value->IsObject()) {
    Handle<Value> toJsonString = value->ToObject()->Get(String::New("toJsonString")); // defined on all generated stubs
    if (!toJsonString.IsEmpty() && toJsonString->IsFunction()) {
      Handle<Value> ret = toJsonString.As<Function>()->Call(value->ToObject(), 0, NULL);
      return jsonstr(convJsToString(ret->ToString()));
    }
  }
    
  return jsonstr(convJsToString(JSON_stringify->Call(JSON, 1, &value)->ToString()));
}

Handle<Value> convJsonstrToJs(jsonstr const &it)
{
  setupJSON();
  Handle<Value> itJs = convStringToJs(it.it);
  Handle<Value> ret = JSON_parse->Call(JSON, 1, &itJs);
  return ret;
}




bool canConvJsToMapStringJsonstr(Handle<Value> itv) {
  if (itv->IsObject()) return true;
  return false;
}

map<string, jsonstr> convJsToMapStringJsonstr(Handle<Value> itv) {

  if (itv->IsObject()) {
    map < string, jsonstr > ret;

    Handle<Object> it = itv->ToObject();
    Handle<Array> itKeys = it->GetOwnPropertyNames();
    
    size_t itKeysLen = itKeys->Length();
    for (size_t i=0; i<itKeysLen; i++) {
      Handle<Value> itKey = itKeys->Get(i);
      Handle<Value> itVal = it->Get(itKey);

      string cKey = convJsToString(itKey->ToString());
      jsonstr cVal = convJsToJsonstr(itVal);

      ret[cKey] = cVal;
    }
    return ret;
  }
  throw runtime_error("convJsToMapStringJsonstr: not an object");
}


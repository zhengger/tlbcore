// -*- C++ -*-
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

#ifndef _TLBCORE_JSWRAPBASE_H_
#define _TLBCORE_JSWRAPBASE_H_

#include <node.h>
#include <node_buffer.h>
#include <node_object_wrap.h>
#include <armadillo>
using namespace node;
using namespace v8;

extern bool fastJsonFlag;

void ThrowInvalidArgs(Isolate *isolate);
void ThrowInvalidArgs();
void ThrowInvalidThis(Isolate *isolate);
void ThrowInvalidThis();
void ThrowTypeError(Isolate *isolate, char const *s);
void ThrowTypeError(char const *s);
void ThrowRuntimeError(Isolate *isolate, char const *s);
void ThrowRuntimeError(char const *s);

// stl::string conversion
bool canConvJsToString(Handle<Value> it);
string convJsToString(Handle<Value> it);
Handle<Value> convStringToJs(string const &it);
Handle<Value> convStringToJs(Isolate *isolate, string const &it);
Handle<Value> convStringToJsBuffer(string const &it);
Handle<Value> convStringToJsBuffer(Isolate *isolate, string const &it);

// arma::Col conversion

template<typename T> bool canConvJsToArmaCol(Handle<Value> itv);
template<typename T> arma::Col<T> convJsToArmaCol(Handle<Value> itv);
template<typename T> Local<Object> convArmaColToJs(arma::Col<T> const &it);

template<typename T> bool canConvJsToArmaRow(Handle<Value> itv);
template<typename T> arma::Row<T> convJsToArmaRow(Handle<Value> itv);
template<typename T> Local<Object> convArmaRowToJs(arma::Row<T> const &it);

template<typename T> bool canConvJsToArmaMat(Handle<Value> it);
template<typename T> arma::Mat<T> convJsToArmaMat(Handle<Value> it, size_t nRows=0, size_t nCols=0);
template<typename T> Local<Object> convArmaMatToJs(arma::Mat<T> const &it);


// arma::cx_double conversion
bool canConvJsToCxDouble(Handle<Value> it);
arma::cx_double convJsToCxDouble(Handle<Value> it);
Local<Object> convCxDoubleToJs(arma::cx_double const &it);

// map<string, jsonstr> conversion
bool canConvJsToMapStringJsonstr(Handle<Value> itv);
map<string, jsonstr> convJsToMapStringJsonstr(Handle<Value> itv);
Local<Value> convJsonstrToJs(map<string, jsonstr> const &it);

// jsonstr conversion
bool canConvJsToJsonstr(Handle<Value> value);
jsonstr convJsToJsonstr(Handle<Value> value);
Local<Value> convJsonstrToJs(jsonstr const &it);

/*
  A template for wrapping any kind of object
*/
template <typename CONTENTS>
struct JsWrapGeneric : node::ObjectWrap {
  JsWrapGeneric(Isolate *_isolate)
  :isolate(_isolate)
  {
  }

  template<typename... Args>
  JsWrapGeneric(Isolate *_isolate, Args &&... _args)
    :isolate(_isolate),
     it(make_shared<CONTENTS>(std::forward<Args>(_args)...))
  {
  }
  
  JsWrapGeneric(Isolate *_isolate, shared_ptr<CONTENTS> _it)
    :isolate(_isolate),
     it(_it)
  {
  }

  void assign(shared_ptr<CONTENTS> _it)
  {
    it = _it;
  }
  
  template<typename... Args>
  void assignConstruct(Args &&... _args)
  {
    it = make_shared<CONTENTS>(std::forward<Args>(_args)...);
  }

  void assignDefault()
  {
    it = make_shared<CONTENTS>();
  }
  
  ~JsWrapGeneric()
  {
  }
  
  shared_ptr<CONTENTS> it;
  Persistent<Value> owner;

  template<typename... Args>
  static Handle<Value> NewInstance(Isolate *isolate, Args &&... _args) {
    EscapableHandleScope scope(isolate);

    Local<Function> localConstructor = Local<Function>::New(isolate, constructor);
    Local<Object> instance = localConstructor->NewInstance(0, nullptr);
    JsWrapGeneric<CONTENTS> * w = node::ObjectWrap::Unwrap< JsWrapGeneric<CONTENTS> >(instance);
    w->assignConstruct(std::forward<Args>(_args)...);
    return scope.Escape(instance);
  }

  static Handle<Value> NewInstance(Isolate *isolate, shared_ptr<CONTENTS> _it) {
    EscapableHandleScope scope(isolate);
    Local<Function> localConstructor = Local<Function>::New(isolate, constructor);
    Local<Object> instance = localConstructor->NewInstance(0, nullptr);
    JsWrapGeneric<CONTENTS> * w = node::ObjectWrap::Unwrap< JsWrapGeneric<CONTENTS> >(instance);
    w->assign(_it);
    return scope.Escape(instance);
  }

  template<class OWNER>
  static Handle<Value> MemberInstance(Isolate *isolate, shared_ptr<OWNER> _parent, CONTENTS *_ptr) {
    EscapableHandleScope scope(isolate);
    Local<Function> localConstructor = Local<Function>::New(isolate, constructor);
    Local<Object> instance = localConstructor->NewInstance(0, nullptr);
    JsWrapGeneric<CONTENTS> * w = node::ObjectWrap::Unwrap< JsWrapGeneric<CONTENTS> >(instance);
    w->assign(shared_ptr<CONTENTS>(_parent, _ptr));
    return scope.Escape(instance);
  }

  static Handle<Value> DependentInstance(Isolate *isolate, Handle<Value> _owner, CONTENTS const &_contents) {
    EscapableHandleScope scope(isolate);
    Local<Function> localConstructor = Local<Function>::New(isolate, constructor);
    Local<Object> instance = localConstructor->NewInstance(0, nullptr);
    JsWrapGeneric<CONTENTS> * w = node::ObjectWrap::Unwrap< JsWrapGeneric<CONTENTS> >(instance);
    w->assignConstruct(_contents);
    w->owner.Reset(isolate, _owner);
    return scope.Escape(instance);
  }

  static shared_ptr<CONTENTS> Extract(Isolate *isolate, Handle<Value> value) {
    Local<Function> localConstructor = Local<Function>::New(isolate, constructor);
    if (value->IsObject()) {
      Handle<Object> valueObject = value->ToObject();
      Local<String> valueTypeName = valueObject->GetConstructorName();
      if (valueTypeName == localConstructor->GetName()) {
        return node::ObjectWrap::Unwrap< JsWrapGeneric<CONTENTS> >(valueObject)->it;
      }
    }
    return shared_ptr<CONTENTS>();
  }
  static shared_ptr<CONTENTS> Extract(Handle<Value> value) {
    return Extract(Isolate::GetCurrent(), value);
  }


  // Because node::ObjectWrap::Wrap is protected
  inline void Wrap2 (Handle<Object> handle) {
    return Wrap(handle);
  }

  Isolate *GetIsolate() {
    return isolate;
  }

  Isolate *isolate;
  static Persistent<Function> constructor;
};

template <typename CONTENTS>
Persistent<Function> JsWrapGeneric<CONTENTS>::constructor;


#endif

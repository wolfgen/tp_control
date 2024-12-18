/*
  Copyright (c) 2009, Hideyuki Tanaka
  All rights reserved.

  Redistribution and use in source and binary forms, with or without
  modification, are permitted provided that the following conditions are met:
  * Redistributions of source code must retain the above copyright
  notice, this list of conditions and the following disclaimer.
  * Redistributions in binary form must reproduce the above copyright
  notice, this list of conditions and the following disclaimer in the
  documentation and/or other materials provided with the distribution.
  * Neither the name of the <organization> nor the
  names of its contributors may be used to endorse or promote products
  derived from this software without specific prior written permission.

  THIS SOFTWARE IS PROVIDED BY <copyright holder> ''AS IS'' AND ANY
  EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
  WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
  DISCLAIMED. IN NO EVENT SHALL <copyright holder> BE LIABLE FOR ANY
  DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
  (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
  LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
  ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
  SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#pragma once

#include <iostream>
#include <sstream>
#include <vector>
#include <map>
#include <string>
#include <stdexcept>
#include <typeinfo>
#include <cstring>
#include <algorithm>
#ifdef __GNUC__
#include <cxxabi.h>#endif
#endif
#include <cstdlib>

using namespace std;

#ifdef _UNICODE
#define tstring			wstring
#define tstringstream	wstringstream
#define tofstream		wofstream
#define tostringstream  wostringstream
#define tistringstream  wistringstream
#define tcerr           wcerr
#define tcout           wcout

#else
#define tstring			string
#define tstringstream	stringstream
#define tofstream		ofstream
#define tostringstream  ostringstream
#define tistringstream  istringstream
#define tcerr           cerr
#define tcout           cout
#endif

namespace cmdline{

namespace detail{

template <typename Target, typename Source, bool Same>
class lexical_cast_t{
public:
  static Target cast(const Source &arg){
    Target ret;
    std::tstringstream ss;
    if (!(ss<<arg && ss>>ret && ss.eof()))
      throw std::bad_cast();
    
    return ret;
  }
};

template <typename Target, typename Source>
class lexical_cast_t<Target, Source, true>{
public:
  static Target cast(const Source &arg){
    return arg;
  }  
};

template <typename Source>
class lexical_cast_t<std::tstring, Source, false>{
public:
  static std::tstring cast(const Source &arg){
    std::tostringstream ss;
    ss<<arg;
    return ss.str();
  }
};

template <typename Target>
class lexical_cast_t<Target, std::tstring, false>{
public:
  static Target cast(const std::tstring &arg){
    Target ret;
    tistringstream ss(arg);
    if (!(ss>>ret && ss.eof()))
      throw std::bad_cast();
    return ret;
  }
};

template <typename T1, typename T2>
struct is_same {
  static const bool value = false;
};

template <typename T>
struct is_same<T, T>{
  static const bool value = true;
};

template<typename Target, typename Source>
Target lexical_cast(const Source &arg)
{
  return lexical_cast_t<Target, Source, detail::is_same<Target, Source>::value>::cast(arg);
}

static inline std::tstring demangle(const std::tstring &name)
{
#ifdef _MSC_VER
	return name;
#elif defined(__GNUC__) 
	// 为gcc编译器时还调用原来的代码
	int status = 0;  char* p = abi::__cxa_demangle(name.c_str(), 0, 0, &status);
	std::tstring ret(p);
	free(p);  
    return ret; 
#else
#error unexpected c complier (msc/gcc), Need to implement this method for demangle
#endif
}

template <class T>
std::tstring readable_typename()
{
    std::string name = typeid(T).name();

#ifdef _UNICODE
    std::wstring wname(name.begin(), name.end());
    return demangle(wname);
#else
    return demangle(name);
#endif
}

template <class T>
std::tstring default_value(T def)
{
  return detail::lexical_cast<std::tstring>(def);
}

template <>
inline std::tstring readable_typename<std::tstring>()
{
  return _T("tstring");
}

} // detail

//-----

class cmdline_error : public std::exception {
public:
  cmdline_error(const std::tstring &msg): msg(msg){}
  ~cmdline_error() throw() {}
  //const TCHAR *what() const throw() { return msg.c_str(); }
private:
  std::tstring msg;
};

template <class T>
struct default_reader{
  T operator()(const std::tstring &str){
    return detail::lexical_cast<T>(str);
  }
};

template <class T>
struct range_reader{
  range_reader(const T &low, const T &high): low(low), high(high) {}
  T operator()(const std::tstring &s) const {
    T ret=default_reader<T>()(s);
    if (!(ret>=low && ret<=high)) throw cmdline::cmdline_error("range_error");
    return ret;
  }
private:
  T low, high;
};

template <class T>
range_reader<T> range(const T &low, const T &high)
{
  return range_reader<T>(low, high);
}

template <class T>
struct oneof_reader{
  T operator()(const std::tstring &s){
    T ret=default_reader<T>()(s);
    if (std::find(alt.begin(), alt.end(), ret)==alt.end())
      throw cmdline_error(_T(""));
    return ret;
  }
  void add(const T &v){ alt.push_back(v); }
private:
  std::vector<T> alt;
};

template <class T>
oneof_reader<T> oneof(T a1)
{
  oneof_reader<T> ret;
  ret.add(a1);
  return ret;
}

template <class T>
oneof_reader<T> oneof(T a1, T a2)
{
  oneof_reader<T> ret;
  ret.add(a1);
  ret.add(a2);
  return ret;
}

template <class T>
oneof_reader<T> oneof(T a1, T a2, T a3)
{
  oneof_reader<T> ret;
  ret.add(a1);
  ret.add(a2);
  ret.add(a3);
  return ret;
}

template <class T>
oneof_reader<T> oneof(T a1, T a2, T a3, T a4)
{
  oneof_reader<T> ret;
  ret.add(a1);
  ret.add(a2);
  ret.add(a3);
  ret.add(a4);
  return ret;
}

template <class T>
oneof_reader<T> oneof(T a1, T a2, T a3, T a4, T a5)
{
  oneof_reader<T> ret;
  ret.add(a1);
  ret.add(a2);
  ret.add(a3);
  ret.add(a4);
  ret.add(a5);
  return ret;
}

template <class T>
oneof_reader<T> oneof(T a1, T a2, T a3, T a4, T a5, T a6)
{
  oneof_reader<T> ret;
  ret.add(a1);
  ret.add(a2);
  ret.add(a3);
  ret.add(a4);
  ret.add(a5);
  ret.add(a6);
  return ret;
}

template <class T>
oneof_reader<T> oneof(T a1, T a2, T a3, T a4, T a5, T a6, T a7)
{
  oneof_reader<T> ret;
  ret.add(a1);
  ret.add(a2);
  ret.add(a3);
  ret.add(a4);
  ret.add(a5);
  ret.add(a6);
  ret.add(a7);
  return ret;
}

template <class T>
oneof_reader<T> oneof(T a1, T a2, T a3, T a4, T a5, T a6, T a7, T a8)
{
  oneof_reader<T> ret;
  ret.add(a1);
  ret.add(a2);
  ret.add(a3);
  ret.add(a4);
  ret.add(a5);
  ret.add(a6);
  ret.add(a7);
  ret.add(a8);
  return ret;
}

template <class T>
oneof_reader<T> oneof(T a1, T a2, T a3, T a4, T a5, T a6, T a7, T a8, T a9)
{
  oneof_reader<T> ret;
  ret.add(a1);
  ret.add(a2);
  ret.add(a3);
  ret.add(a4);
  ret.add(a5);
  ret.add(a6);
  ret.add(a7);
  ret.add(a8);
  ret.add(a9);
  return ret;
}

template <class T>
oneof_reader<T> oneof(T a1, T a2, T a3, T a4, T a5, T a6, T a7, T a8, T a9, T a10)
{
  oneof_reader<T> ret;
  ret.add(a1);
  ret.add(a2);
  ret.add(a3);
  ret.add(a4);
  ret.add(a5);
  ret.add(a6);
  ret.add(a7);
  ret.add(a8);
  ret.add(a9);
  ret.add(a10);
  return ret;
}

//-----

class parser{
public:
  parser(){
  }
  ~parser(){
    for (std::map<std::tstring, option_base*>::iterator p=options.begin();
         p!=options.end(); p++)
      delete p->second;
  }

  void add(const std::tstring &name,
           TCHAR short_name=0,
           const std::tstring &desc= _T("")){
    if (options.count(name)) throw cmdline_error(_T("multiple definition: ")+name);
    options[name]=new option_without_value(name, short_name, desc);
    ordered.push_back(options[name]);
  }

  template <class T>
  void add(const std::tstring &name,
      TCHAR short_name=0,
           const std::tstring &desc= _T(""),
           bool need=true,
           const T def=T()){
    add(name, short_name, desc, need, def, default_reader<T>());
  }

  template <class T, class F>
  void add(const std::tstring &name,
      TCHAR short_name=0,
           const std::tstring &desc= _T(""),
           bool need=true,
           const T def=T(),
           F reader=F()){
    if (options.count(name)) throw cmdline_error(_T("multiple definition: ")+name);
    options[name]=new option_with_value_with_reader<T, F>(name, short_name, need, def, desc, reader);
    ordered.push_back(options[name]);
  }

  void footer(const std::tstring &f){
    ftr=f;
  }

  void set_program_name(const std::tstring &name){
    prog_name=name;
  }

  bool exist(const std::tstring &name) const {
    if (options.count(name)==0) throw cmdline_error(_T("there is no flag: --")+name);
    return options.find(name)->second->has_set();
  }

  template <class T>
  const T &get(const std::tstring &name) const {
    if (options.count(name)==0) throw cmdline_error(_T("there is no flag: --")+name);
    const option_with_value<T> *p=dynamic_cast<const option_with_value<T>*>(options.find(name)->second);
    if (p==NULL) throw cmdline_error(_T("type mismatch flag '")+name+_T("'"));
    return p->get();
  }

  const std::vector<std::tstring> &rest() const {
    return others;
  }

  bool parse(const std::tstring &arg){
    std::vector<std::tstring> args;

    std::tstring buf;
    bool in_quote=false;
    for (std::tstring::size_type i=0; i<arg.length(); i++){
      if (arg[i]=='\"'){
        in_quote=!in_quote;
        continue;
      }

      if (arg[i]==' ' && !in_quote){
        args.push_back(buf);
        buf=_T("");
        continue;
      }

      if (arg[i]=='\\'){
        i++;
        if (i>=arg.length()){
          errors.push_back(_T("unexpected occurrence of '\\' at end of string"));
          return false;
        }
      }

      buf+=arg[i];
    }

    if (in_quote){
      errors.push_back(_T("quote is not closed"));
      return false;
    }

    if (buf.length()>0)
      args.push_back(buf);

    for (size_t i=0; i<args.size(); i++)
      std::tcout<<_T("\"")<<args[i]<<_T("\"")<<std::endl;

    return parse(args);
  }

  bool parse(const std::vector<std::tstring> &args){
    int argc=static_cast<int>(args.size());
    std::vector<const TCHAR*> argv(argc);

    for (int i=0; i<argc; i++)
      argv[i]=args[i].c_str();

    return parse(argc, &argv[0]);
  }

  bool parse(int argc, const TCHAR* const argv[]){
    errors.clear();
    others.clear();

    if (argc<1){
      errors.push_back(_T("argument number must be longer than 0"));
      return false;
    }
    if (prog_name==_T(""))
      prog_name=argv[0];

    std::map<TCHAR, std::tstring> lookup;
    for (std::map<std::tstring, option_base*>::iterator p=options.begin();
         p!=options.end(); p++){
      if (p->first.length()==0) continue;
      TCHAR initial=p->second->short_name();
      if (initial){
        if (lookup.count(initial)>0){
          lookup[initial]=_T("");
          errors.push_back(std::tstring(_T("short option '")) + initial + _T("' is ambiguous"));
          return false;
        }
        else lookup[initial]=p->first;
      }
    }

    for (int i=0; i<argc; i++){
      if (_tcsnccmp(argv[i], _T("--"), 2)==0){
        const TCHAR*p= _tcschr(argv[i]+2, '=');
        if (p){
          std::tstring name(argv[i]+2, p);
          std::tstring val(p+1);
          set_option(name, val);
        }
        else{
          std::tstring name(argv[i]+2);
          if (options.count(name)==0){
            errors.push_back(_T("undefined option: --")+name);
            continue;
          }
          if (options[name]->has_value()){
            if (i+1>=argc){
              errors.push_back(_T("option needs value: --")+name);
              continue;
            }
            else{
              i++;
              set_option(name, argv[i]);
            }
          }
          else{
            set_option(name);
          }
        }
      }
      else if (_tcsnccmp(argv[i], _T("-"), 1)==0){
        if (!argv[i][1]) continue;
        TCHAR last=argv[i][1];
        for (int j=2; argv[i][j]; j++){
          last=argv[i][j];
          if (lookup.count(argv[i][j-1])==0){
            errors.push_back(std::tstring(_T("undefined short option: -"))+argv[i][j-1]);
            continue;
          }
          if (lookup[argv[i][j-1]]==_T("")){
            errors.push_back(std::tstring(_T("ambiguous short option: -"))+argv[i][j-1]);
            continue;
          }
          set_option(lookup[argv[i][j-1]]);
        }

        if (lookup.count(last)==0){
          errors.push_back(std::tstring(_T("undefined short option: -"))+last);
          continue;
        }
        if (lookup[last]==_T("")){
          errors.push_back(std::tstring(_T("ambiguous short option: -"))+last);
          continue;
        }

        if (i+1<argc && options[lookup[last]]->has_value()){
          set_option(lookup[last], argv[i+1]);
          i++;
        }
        else{
          set_option(lookup[last]);
        }
      }
      else{
        others.push_back(argv[i]);
      }
    }

    for (std::map<std::tstring, option_base*>::iterator p=options.begin();
         p!=options.end(); p++)
      if (p->second->valid() == false)
        errors.push_back(_T("need option: --")+std::tstring(p->first));

    return errors.size()==0;
  }

  void parse_check(const std::tstring &arg){
    if (!options.count(_T("help")))
      add(_T("help"), _T('?'), _T("print this message"));
    check(0, parse(arg));
  }

  void parse_check(const std::vector<std::tstring> &args){
    if (!options.count(_T("help")))
      add(_T("help"), _T('?'), _T("print this message"));
    check(args.size(), parse(args));
  }

  void parse_check(int argc, TCHAR*argv[]){
    if (!options.count(_T("help")))
      add(_T("help"), _T('?'), _T("print this message"));
    check(argc, parse(argc, argv));
  }

  std::tstring error() const{
    return errors.size()>0?errors[0]:_T("");
  }

  std::tstring error_full() const{
    std::tostringstream oss;
    for (size_t i=0; i<errors.size(); i++)
      oss<<errors[i]<<std::endl;
    return oss.str();
  }

  std::tstring usage() const {
    std::tostringstream oss;
    oss<<"usage: "<<prog_name<<" ";
    for (size_t i=0; i<ordered.size(); i++){
      if (ordered[i]->must())
        oss<<ordered[i]->short_description()<<" ";
    }
    
    oss<<"[options] ... "<<ftr<<std::endl;
    oss<<"options:"<<std::endl;

    size_t max_width=0;
    for (size_t i=0; i<ordered.size(); i++){
      max_width=std::max<size_t>(max_width, ordered[i]->name().length());
    }
    for (size_t i=0; i<ordered.size(); i++){
      if (ordered[i]->short_name()){
        oss<<"  -"<<ordered[i]->short_name()<<", ";
      }
      else{
        oss<<"      ";
      }

      oss<<"--"<<ordered[i]->name();
      for (size_t j=ordered[i]->name().length(); j<max_width+4; j++)
        oss<<' ';
      oss<<ordered[i]->description()<<std::endl;
    }
    return oss.str();
  }

private:

  void check(int argc, bool ok){
/*
    if ((argc==1 && !ok) || exist(_T("help"))){
      std::tcerr<<usage();
      exit(0);
    }

    if (!ok){
      std::tcerr<<error()<<std::endl<<usage();
      exit(1);
    }*/
  }

  void set_option(const std::tstring &name){
    if (options.count(name)==0){
      errors.push_back(_T("undefined option: --")+name);
      return;
    }
    if (!options[name]->set()){
      errors.push_back(_T("option needs value: --")+name);
      return;
    }
  }

  void set_option(const std::tstring &name, const std::tstring &value){
    if (options.count(name)==0){
      errors.push_back(_T("undefined option: --")+name);
      return;
    }
    if (!options[name]->set(value)){
      errors.push_back(_T("option value is invalid: --")+name+_T("=")+value);
      return;
    }
  }

  class option_base{
  public:
    virtual ~option_base(){}

    virtual bool has_value() const=0;
    virtual bool set()=0;
    virtual bool set(const std::tstring &value)=0;
    virtual bool has_set() const=0;
    virtual bool valid() const=0;
    virtual bool must() const=0;

    virtual const std::tstring &name() const=0;
    virtual TCHAR short_name() const=0;
    virtual const std::tstring &description() const=0;
    virtual std::tstring short_description() const=0;
  };

  class option_without_value : public option_base {
  public:
    option_without_value(const std::tstring &name,
                         TCHAR short_name,
                         const std::tstring &desc)
      :nam(name), snam(short_name), desc(desc), has(false){
    }
    ~option_without_value(){}

    bool has_value() const { return false; }

    bool set(){
      has=true;
      return true;
    }

    bool set(const std::tstring &){
      return false;
    }

    bool has_set() const {
      return has;
    }

    bool valid() const{
      return true;
    }

    bool must() const{
      return false;
    }

    const std::tstring &name() const{
      return nam;
    }

    TCHAR short_name() const{
      return snam;
    }

    const std::tstring &description() const {
      return desc;
    }

    std::tstring short_description() const{
      return _T("--")+nam;
    }

  private:
    std::tstring nam;
    TCHAR snam;
    std::tstring desc;
    bool has;
  };

  template <class T>
  class option_with_value : public option_base {
  public:
    option_with_value(const std::tstring &name,
                      TCHAR short_name,
                      bool need,
                      const T &def,
                      const std::tstring &desc)
      : nam(name), snam(short_name), need(need), has(false)
      , def(def), actual(def) {
      this->desc=full_description(desc);
    }
    ~option_with_value(){}

    const T &get() const {
      return actual;
    }

    bool has_value() const { return true; }

    bool set(){
      return false;
    }

    bool set(const std::tstring &value){
      try{
        actual=read(value);
        has=true;
      }
      catch(const std::exception &e)
      {
          UNREFERENCED_PARAMETER(e);
        return false;
      }
      return true;
    }

    bool has_set() const{
      return has;
    }

    bool valid() const{
      if (need && !has) return false;
      return true;
    }

    bool must() const{
      return need;
    }

    const std::tstring &name() const{
      return nam;
    }

    TCHAR short_name() const{
      return snam;
    }

    const std::tstring &description() const {
      return desc;
    }

    std::tstring short_description() const{
      return tstring(_T("--"))+nam+_T("=")+detail::readable_typename<T>();
    }

  protected:
    std::tstring full_description(const std::tstring& desc){
      return
        desc+_T(" (")+detail::readable_typename<T>()+
        (need ? _T("") : _T(" [=") + detail::default_value<T>(def)+ _T("]"))
        + _T(")");
    }

    virtual T read(const std::tstring &s)=0;

    std::tstring nam;
    TCHAR snam;
    bool need;
    std::tstring desc;

    bool has;
    T def;
    T actual;
  };

  template <class T, class F>
  class option_with_value_with_reader : public option_with_value<T> {
  public:
    option_with_value_with_reader(const std::tstring &name,
                                  TCHAR short_name,
                                  bool need,
                                  const T def,
                                  const std::tstring &desc,
                                  F reader)
      : option_with_value<T>(name, short_name, need, def, desc), reader(reader){
    }

  private:
    T read(const std::tstring &s){
      return reader(s);
    }

    F reader;
  };

  std::map<std::tstring, option_base*> options;
  std::vector<option_base*> ordered;
  std::tstring ftr;

  std::tstring prog_name;
  std::vector<std::tstring> others;

  std::vector<std::tstring> errors;
};

} // cmdline

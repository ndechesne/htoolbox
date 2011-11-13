/*
    Copyright (C) 2011  Hervé Fache

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, version 3.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef _TLV_HELPER_H
#define _TLV_HELPER_H

#include <stdio.h>
#include <string.h>
#include <errno.h>

#include <string>
#include <list>

#include <socket.h>

namespace htoolbox {
namespace tlv {

class IReceptionManager {
public:
  virtual int submit(uint16_t tag, size_t size, const char* val) = 0;
};

class ReceptionManager : public IReceptionManager {
  class IObject {
  protected:
    uint16_t _tag;
  public:
    IObject(uint16_t tag) : _tag(tag) {}
    uint16_t tag() const { return _tag; }
    virtual int submit(size_t, const char*) { return -EPERM; }
  };
  class Void : public IObject {
  public:
    Void(uint16_t tag) : IObject(tag) {}
    int submit(size_t, const char*) { return 0; }
  };
  class Bool : public IObject {
    bool* _value;
  public:
    Bool(uint16_t tag, bool* val) : IObject(tag), _value(val) {}
    int submit(size_t len, const char*) {
      *_value = len > 0;
      return 0;
    }
  };
  class Blob : public IObject {
    char*  _buffer;
    size_t _capacity;
    size_t _received;
  public:
    Blob(uint16_t tag, char* buf, size_t cap) :
      IObject(tag), _buffer(buf), _capacity(cap), _received(0) {}
    int submit(size_t size, const char* val);
  };
  class BigBlob : public IObject {
  public:
    typedef int (*callback_f)(const char* buffer, size_t size, void* user);
  private:
    callback_f _callback;
    void*      _user;
  public:
    BigBlob(uint16_t tag, callback_f cb, void* user) :
      IObject(tag), _callback(cb), _user(user) {}
    int submit(size_t size, const char* val) {
      return _callback(val, size, _user);
    }
  };
  class Int : public IObject {
    int32_t*  _int_32;
    uint32_t* _uint_32;
    int64_t*  _int_64;
    uint64_t* _uint_64;
  public:
    Int(uint16_t tag, int32_t* val_p): IObject(tag),
      _int_32(val_p), _uint_32(NULL), _int_64(NULL), _uint_64(NULL) {}
    Int(uint16_t tag, uint32_t* val_p): IObject(tag),
      _int_32(NULL), _uint_32(val_p), _int_64(NULL), _uint_64(NULL) {}
    Int(uint16_t tag, int64_t* val_p): IObject(tag),
      _int_32(NULL), _uint_32(NULL), _int_64(val_p), _uint_64(NULL) {}
    Int(uint16_t tag, uint64_t* val_p): IObject(tag),
      _int_32(NULL), _uint_32(NULL), _int_64(NULL), _uint_64(val_p) {}
    int submit(size_t size, const char* val);
  };
  class String : public IObject {
    std::string& _value;
  public:
    String(uint16_t tag, std::string& value) : IObject(tag), _value(value) {}
    int submit(size_t, const char* val) {
      _value = val;
      return 0;
    }
  };
  IReceptionManager*  _next;
  std::list<IObject*> _objects;
public:
  ReceptionManager(IReceptionManager* next = NULL): _next(next) {}
  ~ReceptionManager() {
    for (std::list<IObject*>::iterator it = _objects.begin();
        it != _objects.end(); ++it) {
      delete *it;
    }
  }
  void add(uint16_t tag) {
    _objects.push_back(new Void(tag));
  }
  void add(uint16_t tag, bool* val) {
    _objects.push_back(new Bool(tag, val));
  }
  void add(uint16_t tag, char* val, size_t capacity) {
    _objects.push_back(new Blob(tag, val, capacity));
  }
  void add(uint16_t tag, int32_t* val) {
    _objects.push_back(new Int(tag, val));
  }
  void add(uint16_t tag, uint32_t* val) {
    _objects.push_back(new Int(tag, val));
  }
  void add(uint16_t tag, int64_t* val) {
    _objects.push_back(new Int(tag, val));
  }
  void add(uint16_t tag, uint64_t* val) {
    _objects.push_back(new Int(tag, val));
  }
  void add(uint16_t tag, std::string& val) {
    _objects.push_back(new String(tag, val));
  }
  void add(uint16_t tag, BigBlob::callback_f cb, void* user) {
    _objects.push_back(new BigBlob(tag, cb, user));
  }
  void remove(uint16_t tag);
  int submit(uint16_t tag, size_t size, const char* val);
  // Called on CHECK message, return true do abort reception
  typedef bool (*abort_cb_f)(void*);
  int receive(Receiver& rec, abort_cb_f abort_cb = NULL, void* user = NULL);
};

class ITransmissionManager {
  const ITransmissionManager* _next;
public:
  ITransmissionManager(const ITransmissionManager* next): _next(next) {}
  const ITransmissionManager* next() const { return _next; }
  virtual ssize_t send(Sender& sender, bool insert) = 0;
};

class TransmissionManager : public ITransmissionManager {
  class IObject {
  protected:
    bool     _ready;
    uint16_t _tag;
    ssize_t  _len;
  public:
    IObject(uint16_t tag) : _ready(true), _tag(tag), _len(-1) {}
    virtual bool ready() {
      if (_ready) {
        _ready = false;
        return true;
      } else {
          // Re-arm
        _ready = true;
        return false;
      }
    }
    virtual uint16_t tag() const { return _tag; }
    virtual ssize_t length() const { return _len; }
    virtual const char* value() const = 0;
  };
  class Bool : public IObject {
  public:
    Bool(uint16_t tag, bool val) : IObject(tag) {
      _len = val ? 1 : 0;
    }
    const char* value() const {
      if (_len > 0) {
        return "1";
      } else {
        return "";
      }
    }
  };
  class Blob : public IObject {
    const char* _buffer;
    size_t      _size;
    const char* _reader;
    size_t      _sent;
    bool        _ended;
  public:
    Blob(uint16_t tag, const char* buf, size_t size):
      IObject(tag), _buffer(buf), _size(size), _sent(0), _ended(false) {}
    bool ready();
    const char* value() const {
      return _reader;
    }
  };
  class BigBlob : public IObject {
  public:
    typedef ssize_t (*callback_f)(const char** buf, size_t size, void* user);
  private:
    callback_f  _callback;
    void*       _user;
    const char* _buffer;
    bool        _ended;
  public:
    BigBlob(uint16_t tag, callback_f cb, void* user) :
        IObject(tag), _callback(cb), _user(user), _buffer(NULL), _ended(false) {}
    bool ready() {
      if (_ended) {
        // Re-arm
        _ended = false;
        return false;
      }
      _len = _callback(&_buffer, BUFFER_MAX, _user);
      if (_len == 0) {
        _ended = true;
      }
      return true;
    }
    const char* value() const {
      return _buffer;
    }
  };
  class Int : public IObject {
    char   _val[32];
  public:
    Int(uint16_t tag, int32_t val): IObject(tag) {
      _len = sprintf(_val, "%d", val);
    }
    Int(uint16_t tag, uint32_t val): IObject(tag) {
      _len = sprintf(_val, "%u", val);
    }
    Int(uint16_t tag, int64_t val): IObject(tag) {
      _len = sprintf(_val, "%jd", val);
    }
    Int(uint16_t tag, uint64_t val): IObject(tag) {
      _len = sprintf(_val, "%ju", val);
    }
    const char* value() const { return _val; }
  };
  class String : public IObject {
    const std::string& _value;
  public:
    String(uint16_t tag, const std::string& value):
        IObject(tag), _value(value) {
      _len = _value.length();
    }
    const char* value() const {
      return _value.c_str();
    }
  };
  std::list<IObject*> _objects;
  bool                _started;
public:
  TransmissionManager(const ITransmissionManager* next = NULL):
      ITransmissionManager(next), _started(false) {}
  ~TransmissionManager() {
    for (std::list<IObject*>::iterator it = _objects.begin();
        it != _objects.end(); ++it) {
      delete *it;
    }
  }
  void add(uint16_t tag, bool val) {
    _objects.push_back(new Bool(tag, val));
  }
  void add(uint16_t tag, const char* val, size_t size) {
    _objects.push_back(new Blob(tag, val, size));
  }
  void add(uint16_t tag, int32_t val) {
    _objects.push_back(new Int(tag, val));
  }
  void add(uint16_t tag, uint32_t val) {
    _objects.push_back(new Int(tag, val));
  }
  void add(uint16_t tag, int64_t val) {
    _objects.push_back(new Int(tag, val));
  }
  void add(uint16_t tag, uint64_t val) {
    _objects.push_back(new Int(tag, val));
  }
  void add(uint16_t tag, const std::string& val) {
    _objects.push_back(new String(tag, val));
  }
  void add(uint16_t tag, BigBlob::callback_f cb, void* user) {
    _objects.push_back(new BigBlob(tag, cb, user));
  }
  int start(Sender& sender) {
    int rc = sender.start();
    if (rc >= 0) {
      _started = true;
    }
    return rc;
  }
  ssize_t send(Sender& sender, bool start_and_end = true);
};

}
}

#endif // _TLV_HELPER_H

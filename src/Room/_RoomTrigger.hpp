#pragma once
#include "Engine/Engine.hpp"
#include "Engine/ThreadBase.hpp"
#include "Engine/Trigger.hpp"
#include "squirrel.h"
#include <string>

namespace ng {
class _RoomTriggerThread : public ThreadBase {
public:
  _RoomTriggerThread(HSQUIRRELVM vm, const std::string &name, HSQOBJECT thread_obj);
  ~_RoomTriggerThread() override;

  [[nodiscard]] std::string getName() const override {
    return _name;
  }
  [[nodiscard]] HSQUIRRELVM getThread() const override { return _thread_obj._unVal.pThread; }

private:
  HSQUIRRELVM _vm;
  std::string _name;
  HSQOBJECT _thread_obj;
};

class _RoomTrigger : public Trigger {
public:
  _RoomTrigger(Engine &engine, Object &object, HSQOBJECT inside, HSQOBJECT outside);
  ~_RoomTrigger() override;

  HSQOBJECT &getInside() { return _inside; }
  HSQOBJECT &getOutside() { return _outside; }

  std::string getName() override;

private:
  HSQUIRRELVM createThread();
  void trigCore() override;
  void callTrigger(std::vector<HSQOBJECT> &params, const std::string &name);

private:
  Engine &_engine;
  HSQUIRRELVM _vm{};
  Object &_object;
  HSQOBJECT _inside{};
  HSQOBJECT _outside{};
  bool _isInside{false};
  SQInteger _insideParamsCount{0};
  SQInteger _outsideParamsCount{0};
  std::string _insideName;
  std::string _outsideName;
  std::string _name;
  int _id{0};
};
} // namespace ng

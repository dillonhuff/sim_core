#pragma once

#include "coreir.h"

namespace sim_core {

  static inline bool isSelect(CoreIR::Wireable* fst) {
    return fst->getKind() == CoreIR::Wireable::WK_Select;
  }

  static inline bool isSelect(const CoreIR::Wireable& fst) {
    return fst.getKind() == CoreIR::Wireable::WK_Select;
  }

  static inline CoreIR::Select& toSelect(CoreIR::Wireable& fst) {
    assert(isSelect(fst));
    return static_cast<CoreIR::Select&>(fst);
  }

  static inline bool isInstance(CoreIR::Wireable* fst) {
    return fst->getKind() == CoreIR::Wireable::WK_Instance;
  }

  static inline bool isInterface(CoreIR::Wireable* fst) {
    return fst->getKind() == CoreIR::Wireable::WK_Interface;
  }
  
  static inline CoreIR::Instance* toInstance(CoreIR::Wireable* fst) {
    assert(isInstance(fst));
    return static_cast<CoreIR::Instance*>(fst);
  }

  static inline bool isRegisterInstance(CoreIR::Wireable* fst) {
    if (!isInstance(fst)) {
      return false;
    }

    CoreIR::Instance* inst = toInstance(fst);
    string genRefName = inst->getGeneratorRef()->getName();

    return genRefName == "reg";
  }
  
  static inline std::string cVar(CoreIR::Wireable& w) {
    if (isSelect(w)) {
      CoreIR::Select& s = toSelect(w);
      if (CoreIR::isNumber(s.getSelStr())) {
	return cVar(*(s.getParent())) + "[" + s.getSelStr() + "]";
      } else {
	return cVar(*(s.getParent())) + "_" + s.getSelStr();
      }
    } else {
      return w.toString();
    }
  }

}

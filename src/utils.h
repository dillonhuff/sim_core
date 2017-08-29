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

  
}

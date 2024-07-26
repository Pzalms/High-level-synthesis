#pragma once

#ifdef COREIR_BACKEND

#include "coreir.h"

#include "microarchitecture.h"

namespace ahaHLS {

  void emitCoreIR(const std::string& name,
                  MicroArchitecture& arch,
                  CoreIR::Context* const c,
                  CoreIR::Namespace* const n);
}

#endif

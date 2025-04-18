//==                                                                     --==//
//
// by Thomas Hauth [ Thomas.Hauth@cern.ch ]
//
//===----------------------------------------------------------------------===//

#include "ConstCastChecker.h"
#include "TrunCastChecker.h"
#include "ConstCastAwayChecker.h"
#include "GlobalStaticChecker.h"
#include "StaticLocalChecker.h"
#include "MutableMemberModificationChecker.h"
#include "ClassChecker.h"
#include "ClassDumper.h"
#include "edmChecker.h"
#include "getByChecker.h"
#include "FiniteMathChecker.h"
#include "CatchAll.h"
#include "UsingNamespace.h"
#include "ArgSizeChecker.h"
#include "FunctionChecker.h"
#include "FunctionDumper.h"
#include "EDMPluginDumper.h"
#include "ThrUnsafeFCallChecker.h"
#include "ESRecordGetChecker.h"
#include "PsetExistsFCallChecker.h"
#include "PublicMutableChecker.h"
#include "UnnecessaryMutableChecker.h"

#include <clang/StaticAnalyzer/Frontend/CheckerRegistry.h>

// register all custom checkers with clang
// add new entries here if you want to create a new checker
extern "C" void clang_registerCheckers(clang::ento::CheckerRegistry &registry) {
  registry.addChecker<clangcms::ConstCastAwayChecker>(
      "threadsafety.ConstCastAway",
      "Checks for casts which remove const qualifier and might result in thread-unsafe code",
      "no docs");
  registry.addChecker<clangcms::ConstCastChecker>(
      "threadsafety.ConstCast",
      "Checks for casts which remove const qualifier and might result in thread-unsafe code",
      "no docs");
  registry.addChecker<clangcms::TrunCastChecker>(
      "optional.TruncatingCast",
      "Checks for implicit casts where ToType is smaller than FromType which might result in truncation",
      "no docs");
  registry.addChecker<clangcms::StaticLocalChecker>(
      "threadsafety.StaticLocal",
      "Checks for non-const method local statics which might not be thread-safe",
      "no docs");
  registry.addChecker<clangcms::MutableMemberModificationChecker>(
      "threadsafety.MutableMember",
      "Checks for modifying members with the mutable keyword in const member functions, which might not be thread-safe",
      "no docs");
  registry.addChecker<clangcms::GlobalStaticChecker>(
      "threadsafety.GlobalStatic", "Checks for global non-const statics which might not be thread-safe", "no docs");
  registry.addChecker<clangcms::ClassChecker>(
      "optional.ClassChecker", "Checks data classes for thread safety issues", "no docs");
  registry.addChecker<clangcms::ClassDumperCT>("optional.ClassDumperCT",
                                               "dumps template edm::Wrapper,edm::RunCache,edm::LuminosityBlockCache, "
                                               "and edm::GlobalCache types which define data classes ",
                                               "no docs");
  registry.addChecker<clangcms::ClassDumperFT>(
      "optional.ClassDumperFT", "dumps macro TYPELOOKUP_DATA_REG types which define data classes", "no docs");
  registry.addChecker<clangcms::ClassDumperInherit>(
      "optional.ClassDumperInherit", "Dumps classes inheriting from data classes", "no docs");
  registry.addChecker<clangcms::ClassDumper>(
      "optional.ClassDumper", "Dumps class memmbers and base classes ", "no docs");
  registry.addChecker<clangcms::FiniteMathChecker>("cms.NonFiniteMath", "Reports usage of isnan and isinf.", "no docs");
  registry.addChecker<clangcms::UsingNamespace>(
      "cms.CodeRules.UsingNamespace", "Checks for 'using namespace' or 'using std::' in header files", "no docs");
  registry.addChecker<clangcms::CatchAll>(
      "cms.CodeRules.CatchAll", "Checks for 'catch(...)' in source files", "no docs");
  registry.addChecker<clangcms::edmChecker>(
      "deprecated.edmChecker",
      "Flags classes inheriting from edm::EDProducer,edm::EDFilter,edm::Analyzer or edm::OutputModule",
      "no docs");
  registry.addChecker<clangcms::getByChecker>(
      "optional.getByChecker",
      "Checks for calls to edm::getByLabel or edm::getManyByType and reports edm::Handle type passed",
      "no docs");
  registry.addChecker<clangcms::PsetExistsFCallChecker>(
      "cms.CodeRules.psetExistsChecker",
      "Checks for calls to edm::ParameterSet::exists() or edm::ParameterSet::existsAs<>()",
      "no docs");
  registry.addChecker<clangcms::ArgSizeChecker>(
      "optional.ArgSize", "Reports args passed by value with size>4k.", "no docs");
  registry.addChecker<clangcms::FunctionChecker>(
      "cms.FunctionChecker", "Reports functions which access non-const statics", "no docs");
  registry.addChecker<clangcms::FunctionDumper>(
      "cms.FunctionDumper", "Reports function calls and overrides", "no docs");
  registry.addChecker<clangcms::EDMPluginDumper>(
      "optional.EDMPluginDumper", "Dumps macro DEFINE_EDM_PLUGIN types", "no docs");
  registry.addChecker<clangcms::ThrUnsafeFCallChecker>(
      "threadsafety.ThrUnsafeFCallChecker", "Reports calls of known thread unsafe functions", "no docs");
  registry.addChecker<clangcms::PublicMutableChecker>(
      "threadsafety.PublicMutableMember", "Checks for non-private mutable member", "no docs");
  registry.addChecker<clangcms::UnnecessaryMutableChecker>(
      "cms.CodeRules.UnnecessaryMutableChecker",
      "Checks for mutable members that are not modified in any const method",
      "no focs");
}

extern "C" const char clang_analyzerAPIVersionString[] = CLANG_ANALYZER_API_VERSION_STRING;

#include "DataFormats/Provenance/interface/ProcessConfiguration.h"
#include "FWCore/Utilities/interface/Digest.h"
#include "FWCore/Utilities/interface/EDMException.h"

#include <ostream>
#include <cassert>
#include <sstream>
#include <cctype>

/*----------------------------------------------------------------------

----------------------------------------------------------------------*/

namespace edm {

  ProcessConfiguration::ProcessConfiguration() = default;

  ProcessConfiguration::ProcessConfiguration(std::string const& procName,
                                             ParameterSetID const& pSetID,
                                             ReleaseVersion const& relVersion,
                                             HardwareResourcesDescription const& hwDescription)
      : processName_(procName),
        parameterSetID_(pSetID),
        releaseVersion_(relVersion),
        passID_(hwDescription.serialize()) {}

  ProcessConfiguration::ProcessConfiguration(std::string const& procName,
                                             ReleaseVersion const& relVersion,
                                             HardwareResourcesDescription const& hwDescription)
      : processName_(procName), parameterSetID_(), releaseVersion_(relVersion), passID_(hwDescription.serialize()) {
    setCurrentProcess();
  }

  ParameterSetID const& ProcessConfiguration::parameterSetID() const {
    if (parameterSetID_ == ParameterSetID() && isCurrentProcess()) {
      throw edm::Exception(errors::LogicError) << "Illegal attempt to access the process top level parameter set ID\n"
                                               << "from the ProcessConfiguration before that parameter\n"
                                               << "set has been frozen and registered.  The parameter set\n"
                                               << "can be changed during module validation, which occurs\n"
                                               << "concurrently with module construction.  The ID of the\n"
                                               << "ProcessConfiguration itself also depends on that parameter\n"
                                               << "set ID.  It is illegal to access either before they are frozen.\n";
    }
    return parameterSetID_;
  }

  ProcessConfigurationID ProcessConfiguration::id() const {
    if (transient_.pcid_.isValid()) {
      return transient_.pcid_;
    }
    // This implementation is ripe for optimization.
    std::ostringstream oss;
    oss << *this;
    std::string stringrep = oss.str();
    cms::Digest md5alg(stringrep);
    ProcessConfigurationID pcid(md5alg.digest().toString());
    return pcid;
  }

  void ProcessConfiguration::setParameterSetID(ParameterSetID const& pSetID) {
    assert(parameterSetID_ == ParameterSetID());
    parameterSetID_ = pSetID;
  }

  ProcessConfigurationID ProcessConfiguration::setProcessConfigurationID() {
    if (!transient_.pcid_.isValid()) {
      transient_.pcid_ = id();
    }
    return transient_.pcid_;
  }

  void ProcessConfiguration::reduce() {
    // Skip to the part of the release version just after
    // the first two numbers and erase the rest of it.
    std::string::iterator iter = releaseVersion_.begin();
    std::string::iterator iEnd = releaseVersion_.end();
    while (iter != iEnd && isdigit(*iter) == 0)
      ++iter;
    while (iter != iEnd && isdigit(*iter) != 0)
      ++iter;
    while (iter != iEnd && isdigit(*iter) == 0)
      ++iter;
    while (iter != iEnd && isdigit(*iter) != 0)
      ++iter;
    if (iter == iEnd)
      return;
    transient_.pcid_ = ProcessConfigurationID();
    releaseVersion_.erase(iter, iEnd);
    passID_ = edm::HardwareResourcesDescription().serialize();
  }

  bool operator<(ProcessConfiguration const& a, ProcessConfiguration const& b) {
    if (a.processName() < b.processName())
      return true;
    if (b.processName() < a.processName())
      return false;
    if (a.parameterSetID() < b.parameterSetID())
      return true;
    if (b.parameterSetID() < a.parameterSetID())
      return false;
    if (a.releaseVersion() < b.releaseVersion())
      return true;
    if (b.releaseVersion() < a.releaseVersion())
      return false;
    if (a.hardwareResourcesDescriptionSerialized() < b.hardwareResourcesDescriptionSerialized())
      return true;
    return false;
  }

  std::ostream& operator<<(std::ostream& os, ProcessConfiguration const& pc) {
    os << pc.processName() << ' ' << pc.parameterSetID() << ' ' << pc.releaseVersion() << ' '
       << pc.hardwareResourcesDescription();
    return os;
  }
}  // namespace edm

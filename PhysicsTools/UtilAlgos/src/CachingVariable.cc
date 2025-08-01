#include "PhysicsTools/UtilAlgos/interface/CachingVariable.h"

VariableComputer::VariableComputer(const CachingVariable::CachingVariableFactoryArg& arg, edm::ConsumesCollector& iC)
    : arg_(arg) {
  if (arg_.iConfig.exists("separator"))
    separator_ = arg_.iConfig.getParameter<std::string>("separator");
  else
    separator_ = "_";

  name_ = arg_.n;
  method_ = arg_.iConfig.getParameter<std::string>("computer");
}

void VariableComputer::declare(std::string var, edm::ConsumesCollector& iC) {
  if (iCompute_.find(var) != iCompute_.end()) {
    edm::LogError("VariableComputer") << "redeclaring: " << var << " skipping.";
    return;
  }
  std::string aName = name_ + separator_ + var;
  ComputedVariable* newVar = new ComputedVariable(method_, aName, arg_.iConfig, this, iC);
  iCompute_[var] = newVar;
  arg_.m.insert(std::make_pair(aName, newVar));
}
void VariableComputer::assign(std::string var, double& value) const {
  std::map<std::string, const ComputedVariable*>::const_iterator it = iCompute_.find(var);
  if (it == iCompute_.end()) {
    std::stringstream ss;
    ss << "cannot assign: " << var << ". List of variable declared:\n";
    for (std::map<std::string, const ComputedVariable*>::const_iterator it = iCompute_.begin(); it != iCompute_.end();
         ++it)
      ss << it->first << std::endl;
    edm::LogError("VariableComputer") << ss.str();
  } else
    it->second->setCache(value);
}

void VariableComputer::doesNotCompute() const {
  for (std::map<std::string, const ComputedVariable*>::const_iterator it = iCompute_.begin(); it != iCompute_.end();
       ++it)
    it->second->setNotCompute();
}
void VariableComputer::doesNotCompute(std::string var) const {
  std::map<std::string, const ComputedVariable*>::const_iterator it = iCompute_.find(var);
  if (it == iCompute_.end()) {
    std::stringstream ss;
    ss << "cannot act on: " << var << ". List of variable declared:\n";
    for (std::map<std::string, const ComputedVariable*>::const_iterator it = iCompute_.begin(); it != iCompute_.end();
         ++it)
      ss << it->first << std::endl;
    edm::LogError("VariableComputer") << ss.str();
  } else
    it->second->setNotCompute();
}

ComputedVariable::ComputedVariable(const CachingVariableFactoryArg& arg, edm::ConsumesCollector& iC)
    : CachingVariable("ComputedVariable", arg.n, arg.iConfig, iC),
      myComputer{VariableComputerFactory::get()->create(arg.iConfig.getParameter<std::string>("computer"), arg, iC)} {}

VariableComputerTest::VariableComputerTest(const CachingVariable::CachingVariableFactoryArg& arg,
                                           edm::ConsumesCollector& iC)
    : VariableComputer(arg, iC) {
  declare("toto", iC);
  declare("tutu", iC);
  declare("much", iC);
}

void VariableComputerTest::compute(const edm::Event& iEvent) const {
  //does some mumbo jumbo with the event.
  // computes a bunch of doubles
  double toto = 3;
  double tutu = 4;

  //set the  variables  value (which do as if they had been cached)
  assign("toto", toto);
  assign("tutu", tutu);
  doesNotCompute("much");
}

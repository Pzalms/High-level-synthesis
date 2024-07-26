#pragma once

#include "scheduling.h"

namespace ahaHLS {

  class ControlFlowPosition {

    StateId state;
    bool inPipe;
    int pipeStage;

  public:
    llvm::Instruction* instr;

    ControlFlowPosition(const StateId state_,
                        const bool inPipe_,
                        const int pipeStage_,
                        llvm::Instruction* const instr_) :
      state(state_), inPipe(inPipe_), pipeStage(pipeStage_), instr(instr_) {
    }

    bool inPipeline() const {
      return inPipe;
    }

    StateId stateId() const {
      return state;
    }

    BasicBlock* getBB() const {
      return instr->getParent();
    }

    int pipelineStage() const {
      return pipeStage;
    }
  };

  std::ostream& operator<<(std::ostream& out, ControlFlowPosition& pos);
  
  ControlFlowPosition position(const StateId state, Instruction* const instr);

  ControlFlowPosition pipelinePosition(Instruction* const instr,
                                       const StateId state,
                                       const int stage);

  bool hasOutput(llvm::Instruction* instr);

  class Memory {
  public:
    int width;
    std::string name;
    int depth;
  };

  static inline
  Wire wire(const int width, const std::string& name) {
    return {false, width, name};
  }

  static inline
  Wire reg(const int width, const std::string& name) {
    return {true, width, name};
  }

  static inline
  Wire constWire(const int width, const int value) {
    return {width, value};
  }
  
  Port wireToOutputPort(const Wire w);
  Port wireToInputPort(const Wire w);
  
  static inline
  bool operator<(const Wire a, const Wire b) {
    return a.name < b.name;
  }

  std::ostream& operator<<(std::ostream& out, const Wire w);

  class ModuleInstance {
  public:
    std::string modName;
    std::map<std::string, std::string> params;
    std::string instName;
    std::map<std::string, std::string> portConnections;

    ModuleInstance(const std::string modName_,
                   const std::string instName_,
                   const std::map<std::string, std::string> portConnections_) :
      modName(modName_), params({}), instName(instName_), portConnections(portConnections_) {}

    ModuleInstance(const std::string modName_,
                   const std::map<std::string, std::string> params_,
                   const std::string instName_,
                   const std::map<std::string, std::string> portConnections_) :
      modName(modName_), params(params_), instName(instName_), portConnections(portConnections_) {}
    
  };

  static inline
  void print(std::ostream& out, int level, const ModuleInstance& b) {

    std::vector<std::string> portStrings;
    for (auto pt : b.portConnections) {
      portStrings.push_back("." + pt.first + "(" + pt.second + ")");
    }

    std::vector<std::string> paramStrs;
    for (auto pt : b.params) {
      paramStrs.push_back("." + pt.first + "(" + pt.second + ")");
    }

    out << tab(level) << b.modName;
    if (paramStrs.size() > 0) {
      out << " #(" << commaListString(paramStrs) << ")";
    }

    out << " ";
    out << b.instName << "(";
    out << commaListString(portStrings);
    out << ");" << std::endl;
  }

  class FunctionalUnit {
  public:

    ModuleSpec module;
    std::string instName;

    std::map<std::string, Wire> portWires;
    std::map<std::string, Wire> outWires;

    bool external;

    bool isExternal() const {
      return external;
    }

    std::string getModName() const {
      return module.name;
    }

    std::string outputWire(const std::string& name) const {
      if (!dbhc::contains_key(name, outWires)) {
        std::cout << "Error: No wire named " << name << std::endl;
        assert(false);
      }
      
      auto n = dbhc::map_find(name, outWires).name;
      return n;
    }

    Wire outputWire() const {
      assert(outWires.size() == 1);
      return (std::begin(outWires))->second;
    }
    
    std::string inputWire(const std::string& name) const {
      if (!dbhc::contains_key(name, portWires)) {
        std::cout << "Error: No wire named " << name << std::endl;
        assert(false);
      }
      
      auto n = dbhc::map_find(name, portWires).name;
      if (isExternal()) {
        return n + "_reg";
      }

      return n;
    }

    Wire input(const std::string& name) const {
      if (!dbhc::contains_key(name, portWires)) {
        std::cout << "Error: No wire named " << name << ", input wires in unit " <<  instName << std::endl;
        for (auto w : portWires) {
          std::cout << tab(1) << w.first << " -> " << w.second.valueString() << endl;
        }

        std::cout << "Module spec = " << module << std::endl;
        assert(false);
      }
      
      auto n = dbhc::map_find(name, portWires);
      if (isExternal()) {
        return wire(n.width, n.name + "_reg");
      }

      return n;
    }
    
    std::map<std::string, std::string> getParams() const {
      return module.params;
    }

    Wire onlyInput() const {
      assert(portWires.size() == 1);

      return (*begin(portWires)).second;
    }
    
    std::string onlyOutputVar() const {
      assert(outWires.size() == 1);

      return (*begin(outWires)).second.name;
    }
  };

  class InstructionBinding {
  public:
    FunctionalUnit unit;
    std::map<llvm::Value*, std::string> instrWires;

    InstructionBinding() {}
    InstructionBinding(const FunctionalUnit& unit_) : unit(unit_), instrWires({}) {}
    InstructionBinding(const FunctionalUnit& unit_,
                       const std::map<llvm::Value*, std::string>& wires_) :
      unit(unit_), instrWires(wires_) {}    
  };

  class ElaboratedPipeline {
  public:
    Pipeline p;
    Wire inPipe;
    StateId stateId;

    ElaboratedPipeline(const Pipeline& p_) : p(p_) {}

    Wire inPipeWire() const {
      return wire(1, inPipe.name + "_out_data");
    }

    int II() const {
      return p.II();
    }

    int stateIndex(const StateId id) const {
      return id - p.startState();
    }

    int stageForState(const StateId id) const {
      return stateIndex(id);
    }

    int stateForStage(const int stageNo) const {
      return p.startState() + stageNo;
    }
    
  };

  bool isPipelineState(const StateId id,
                       const std::vector<ElaboratedPipeline>& pipelines);

  ElaboratedPipeline getPipeline(const StateId id,
                                 const std::vector<ElaboratedPipeline>& pipelines);

  FunctionalUnit functionalUnitForSpec(const std::string unitName,
                                       const ModuleSpec& mSpec);
  
  class RAM {
    
  public:

    std::string name;
    int width;
    int depth;

    RAM(const std::string& name_,
        const int width_,
        const int depth_) : name(name_), width(width_), depth(depth_) {}
  };

  class ControlState {
    Wire globalState;
    Wire lastBB;

    std::map<llvm::BasicBlock*, int> basicBlockNos;

  public:

    ControlState() {
      globalState = reg(32, "global_state");
      lastBB = reg(32, "last_BB_reg");
    }

    void setBasicBlockNo(BasicBlock* const bb, const int val) {
      basicBlockNos[bb] = val;
    }

    int getBasicBlockNo(BasicBlock* bb) const {
      return dbhc::map_find(bb, basicBlockNos);
    }

    Wire getGlobalState() const {
      return globalState;
    }

    Wire getLastBB() const {
      return lastBB;
    }
  };

  class UnitController {
  public:
    FunctionalUnit unit;
    std::map<StateId, std::vector<Instruction*> > instructions;
  };

  typedef std::vector<std::string> StallConds;
  typedef std::map<std::string, std::string> PortAssignments;

  // Represents all possible values that can be assigned to
  // a port, and the cycles at which they are assigned
  class PortValues {
  public:
    // This map should include
    //  1. States where the port is assigned
    //  2. States where it is an unused value
    bool isInsensitive;
    map<Wire, Wire> portVals;
    std::string defaultValue;
  };

  // Need to move to one map from ports to states and the values
  // they take in each state
  class PortController {
  public:
    UnitController unitController;
    PortAssignments statelessDefaults;

    map<string, PortValues> inputControllers;

    void setCond(const std::string& port, const Wire& condition, const Wire& value);

    FunctionalUnit& functionalUnit() {
      return unitController.unit;
    }

    void setAlways(const std::string& portName, const Wire value) {
      inputControllers[functionalUnit().inputWire(portName)].portVals[constWire(1, 1)] = value;
    }

    Wire onlyInput() const {
      const FunctionalUnit& unit = unitController.unit;
      return unit.onlyInput();
    }

    bool isExternal() const { return unitController.unit.isExternal(); }

    string defaultValue(const std::string& port) {
      return dbhc::map_find(port, statelessDefaults);
    }

    bool hasDefault(const std::string& port) {
      return dbhc::contains_key(port, statelessDefaults);
    }

  };

  class RegController {
  public:
    Wire reg;
    std::string resetValue;
    //std::map<string, string> values;
    std::map<Wire, Wire> values;
  };

  class WorldState {
  public:
    std::map<Instruction*, Wire> values;
  };

  class DataPath {
  public:
    std::map<StateId, WorldState> stateData;
    std::map<StateId, WorldState> stateDataInputs;    
  };
  
  class MicroArchitecture {
  public:
    ControlState cs;

    STG stg;
    //std::map<llvm::Instruction*, FunctionalUnit> unitAssignment;
    std::map<llvm::Instruction*, InstructionBinding> unitAssignment;
    std::map<llvm::Value*, int> memoryMap;
    //std::map<llvm::Instruction*, Wire> names;
    std::vector<ElaboratedPipeline> pipelines;

    std::map<Wire, std::string> resetValues;
    HardwareConstraints hcs;

    std::map<std::string, RegController> regControllers;
    std::map<std::string, PortController> portControllers;
    std::vector<FunctionalUnit> functionalUnits;

    int uniqueNum;

    std::map<std::pair<llvm::BasicBlock*, llvm::BasicBlock*>, Wire> edgeTakenWires;
    std::map<StateId, Wire> atStateWires;
    std::map<StateId, Wire> lastBBWires;
    std::map<StateId, Wire> entryBBWires;
    std::map<StateId, Wire> lastStateWires;

    DataPath dp;
    
    Wire isActiveBlockVar(const StateId state, llvm::BasicBlock* const bb);
    std::string uniqueName(const std::string& prefix) {
      std::string name = prefix + "_" + std::to_string(uniqueNum);
      uniqueNum++;
      return name;
    }

    FunctionalUnit& makeUnit(std::string name, ModuleSpec& mSpec) {
      functionalUnits.push_back(functionalUnitForSpec(name, mSpec));
      return functionalUnits.back();
    }

    PortController& addPortController(FunctionalUnit& unit) {
      int earlierSize = portControllers.size();
      PortController c;
      c.unitController.unit = unit;
      portControllers[unit.instName] = c;

      assert(portControllers.size() == (earlierSize + 1));

      return portControllers[unit.instName];
    }
    
    PortController& portController(const std::string& name) {
      if (!dbhc::contains_key(name, portControllers)) {
        std::cout << "Error: No controller for " << name << std::endl;
        assert(dbhc::contains_key(name, portControllers));
      }
      return portControllers[name];
      // for (auto& c : portControllers) {
      //   if (c.unitController.unit.instName == name) {
      //     return c.second;
      //   }
      // }

      // cout << "Error: Could not find controller for " << name << endl;
      // assert(false);
    }
    
    void addController(const std::string& name, const int width) {
      RegController ctr;
      ctr.reg = reg(width, name);
      ctr.resetValue = "0";
      regControllers[name] = ctr;
    }
    
    RegController& getController(const std::string& name) {
      if (dbhc::contains_key(name, regControllers)) {
        return regControllers[name];
      } else {
        addController(name, 1);
        return regControllers[name];
      }
    }

    RegController& getController(const Wire& w) {
      auto name = w.name;
      if (dbhc::contains_key(name, regControllers)) {
        return regControllers[name];
      } else {
        addController(name, w.width);
        return regControllers[name];
      }
    }
    
    MicroArchitecture(const ControlState& cs_,
                      //const ArchOptions& archOptions_,
                      const STG& stg_,
                      //const std::map<llvm::Instruction*, FunctionalUnit>& unitAssignment_,
                      const std::map<llvm::Instruction*, InstructionBinding>& unitAssignment_,
                      const std::map<llvm::Value*, int>& memoryMap_,
                      const std::map<llvm::Instruction*, Wire>& names_,
                      const std::vector<ElaboratedPipeline>& pipelines_,
                      const HardwareConstraints& hcs_) :
      cs(cs_),
      //archOptions(archOptions_),
      stg(stg_),
      unitAssignment(unitAssignment_),
      memoryMap(memoryMap_),
      //names(names_),
      pipelines(pipelines_),
      hcs(hcs_),
      uniqueNum(0) {

      resetValues.insert({Wire(true, 32, "global_state"),
            std::to_string(cs.getBasicBlockNo(&(stg.getFunction()->getEntryBlock())))});
    }

    // bool hasGlobalStall() const {
    //   return cs.hasGlobalStall();
    // }

    ElaboratedPipeline getPipeline(const StateId state) const {
      assert(ahaHLS::isPipelineState(state, pipelines));
      return ahaHLS::getPipeline(state, pipelines);
    }

    int numFUsWithName(const std::string& name) const {
      int n = 0;
      std::set<std::string> alreadyAdded;
      for (auto ua : unitAssignment) {
        if (!dbhc::elem(ua.second.unit.instName, alreadyAdded) && (ua.second.unit.getModName() == name)) {
          n++;
        }
        alreadyAdded.insert(ua.second.unit.instName);
      }
      return n;
    }

    int numReadPorts() const {
      return numFUsWithName("load");
    }

    int numWritePorts() const {
      return numFUsWithName("store");
    }
    
    bool isPipelineState(const StateId id) const {
      return ahaHLS::isPipelineState(id, pipelines);
    }

  };

  Wire buildIncCounter(const Wire incrCond,
                       const int width,
                       MicroArchitecture& arch);
  
  MicroArchitecture
  buildMicroArchitecture(const STG& stg,
                         std::map<std::string, int>& memoryMap);

  MicroArchitecture
  buildMicroArchitecture(const STG& stg,
                         std::map<std::string, int>& memoryMap,
                         HardwareConstraints& hcs);
  
  MicroArchitecture
  buildMicroArchitecture(const STG& stg,
                         std::map<llvm::Value*, int>& memoryMap,
                         HardwareConstraints& hcs);
  
  MicroArchitecture
  buildMicroArchitecture(const STG& stg,
                         std::map<llvm::Value*, int>& memoryMap);

  // TODO: Move to separate memory analysis file, and eventually
  // to an LLVM dataflow pass
  std::map<llvm::Instruction*, llvm::Value*>
  memoryOpLocations(llvm::Function* f);

  std::string atState(const StateId state, MicroArchitecture& arch);  
  std::string notAtState(const StateId state, MicroArchitecture& arch);

  llvm::Instruction* lastInstructionInState(const StateId state,
                                            MicroArchitecture& arch);

  std::string floatBits(const float f);

  ControlFlowPosition position(const StateId state,
                               Instruction* const instr,
                               MicroArchitecture& arch);

  bool stateless(FunctionalUnit& unit);

  Wire inAnyPipeline(MicroArchitecture& arch);

  Wire checkEqual(const int value, const Wire w, MicroArchitecture& arch);
  Wire checkEqual(const Wire valWire, const Wire w, MicroArchitecture& arch);

  PortController& addPortController(const std::string& name,
                                    const int width,
                                    MicroArchitecture& arch);
  
  Wire blockActiveInState(const StateId state,
                          BasicBlock* const blk,
                          MicroArchitecture& arch);
  
  bool jumpToSameState(BasicBlock* const predecessor,
                       BasicBlock* const successor,
                       MicroArchitecture& arch);

  Wire atStateWire(const StateId state, MicroArchitecture& arch);

  Wire checkNotWire(const Wire in, MicroArchitecture& arch);  
  Wire checkAnd(const Wire in0, const Wire in1, MicroArchitecture& arch);

  std::string dataOutput(llvm::Instruction* instr0, const MicroArchitecture& arch);

  std::vector<Port>
  getPorts(MicroArchitecture& arch);

  std::string outputName(Value* val,
                         ControlFlowPosition& currentPosition,
                         MicroArchitecture& arch);

  Wire outputWire(Value* val,
                  ControlFlowPosition& currentPosition,
                  MicroArchitecture& arch);
  
  bool isInsensitive(const std::string& port,
                     PortController& portController);

  bool needsTempStorage(llvm::Instruction* const instr,
                        MicroArchitecture& arch);

  std::ostream& operator<<(std::ostream& out, const RegController& controller);
  std::ostream& operator<<(std::ostream& out, const FunctionalUnit& unit);  

  void emitPipelineValidChainBlock(MicroArchitecture& arch);

  std::vector<ElaboratedPipeline>
  buildPipelines(llvm::Function* f, const STG& stg);

  void
  emitPipelineInitiationBlock(MicroArchitecture& arch);

  void emitPipelineResetBlock(MicroArchitecture& arch);

  void emitPipelineRegisterChains(MicroArchitecture& arch);

  Wire stateActiveReg(const StateId state, MicroArchitecture& arch);

  Wire nextBBReg(const StateId state, MicroArchitecture& arch);
  Wire lastBBReg(const StateId state, MicroArchitecture& arch);

  void convertRegisterControllersToPortControllers(MicroArchitecture& arch);
}

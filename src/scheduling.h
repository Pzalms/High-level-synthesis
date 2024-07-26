#pragma once

#include "binding.h"
#include "expression.h"

namespace ahaHLS {

  template<typename T>
  class LinearExpr {
    //std::map<std::string, int> vars;
    std::map<T, int> vars;
    int c;

  public:

    LinearExpr(const T var_) :
      vars({{var_, 1}}), c(0) {}

    LinearExpr(int c_) :
      vars({}), c(c_) {}
    
    LinearExpr(std::map<T, int>& vars_, const int c_) :
      vars(vars_), c(c_) {}

    LinearExpr() : vars({}), c(0) {}

    LinearExpr<T> scalarMul(const int k) const {
      std::map<T, int> mulVars;
      for (auto v : vars) {
        mulVars.insert({v.first, k*v.second});
      }
      return {mulVars, k*getCoeff()};
    }
    
    LinearExpr<T> sub(const LinearExpr<T>& right) const {
      std::map<T, int> subVars;
      auto rightVars = right.getVars();
      for (auto v : vars) {
        if (dbhc::contains_key(v.first, rightVars)) {
          subVars.insert({v.first, v.second - dbhc::map_find(v.first, rightVars)});
        } else {
          subVars.insert(v);
        }
      }

      for (auto v : rightVars) {
        if (!dbhc::contains_key(v.first, vars)) {
          subVars.insert({v.first, -v.second});
        }
      }

      return {subVars, c - right.getCoeff()};
    }

    LinearExpr<T> add(const LinearExpr<T>& right) const {
      std::map<T, int> addVars;
      auto rightVars = right.getVars();
      for (auto v : vars) {
        if (dbhc::contains_key(v.first, rightVars)) {
          addVars.insert({v.first, v.second + dbhc::map_find(v.first, rightVars)});
        } else {
          addVars.insert(v);
        }
      }

      for (auto v : rightVars) {
        if (!dbhc::contains_key(v.first, vars)) {
          addVars.insert({v.first, v.second});
        }
      }

      return {addVars, c + right.getCoeff()};
    }
    
    int getCoeff() const {
      return c;
    }

    std::map<T, int> getVars() const {
      return vars;
    }

    int getValue(const T& var) {
      assert(dbhc::contains_key(var, vars));
      return dbhc::map_find(var, vars);
    }
  };

  typedef LinearExpr<std::string> LinearExpression;

  template<typename T>
  static inline
  std::ostream& operator<<(std::ostream& out, const LinearExpr<T> expr) {
    auto vars = expr.getVars();
    for (auto v : vars) {
      out << v.second << "*" << v.first << " + ";
    }
    out << expr.getCoeff();

    return out;
  }

  enum ZCondition {
    CMP_LTZ,
    CMP_GTZ,
    CMP_LTEZ,
    CMP_GTEZ,
    CMP_EQZ
  };

  template<typename T>
  class LinearCon {
  public:
    LinearExpr<T> expr;
    ZCondition cond;
  };

  typedef LinearCon<std::string> LinearConstraint;

  static inline
  std::string toString(const ZCondition cond) {
    switch (cond) {
    case CMP_LTZ:
      return "< 0";
    case CMP_GTEZ:
      return ">= 0";
    case CMP_LTEZ:
      return "<= 0";
    case CMP_GTZ:
      return "> 0";
    case CMP_EQZ:
      return "== 0";
      
    default:
      assert(false);
    }
  }

  static inline
  std::ostream& operator<<(std::ostream& out, const LinearConstraint& c) {
    out << c.expr << " " << toString(c.cond);
    return out;
  }

  template<typename T>
  static inline
  LinearExpr<T>
  operator-(const LinearExpr<T> left, const LinearExpr<T> right) {
    return left.sub(right);
  }

  template<typename T>  
  static inline
  LinearExpr<T>
  operator+(const LinearExpr<T> left, const LinearExpr<T> right) {
    return left.add(right);
  }

  template<typename T>  
  static inline
  LinearExpr<T>
  operator+(const LinearExpr<T> left, const int right) {
    return left.add(LinearExpr<T>(right));
  }
  
  template<typename T>  
  static inline
  LinearExpr<T>
  operator*(const LinearExpr<T> left, const int c) {
    return left.scalarMul(c);
  }

  template<typename T>  
  static inline
  LinearExpr<T>
  operator*(const int c, const LinearExpr<T> right) {
    return right.scalarMul(c);
  }
  
  template<typename T> static inline
  LinearCon<T>
  operator>=(const LinearExpr<T> left, const LinearExpr<T> right) {
    return {left - right, CMP_GTEZ};
  }

  template<typename T> static inline
  LinearCon<T>
  operator>=(const LinearExpr<T> left, const int right) {
    return {left - LinearExpr<T>(right), CMP_GTEZ};
  }

  template<typename T> static inline
  LinearCon<T>
  operator>=(const int left, const LinearExpr<T> right) {
    return {LinearExpr<T>(left) - right, CMP_GTEZ};
  }

  template<typename T> static inline
  LinearCon<T>
  operator<=(const LinearExpr<T> left, const LinearExpr<T> right) {
    return {left - right, CMP_LTEZ};
  }

  template<typename T> static inline
  LinearCon<T>
  operator<=(const LinearExpr<T> left, const int right) {
    return {left - LinearExpr<T>(right), CMP_LTEZ};
  }

  template<typename T> static inline
  LinearCon<T>
  operator<=(const int left, const LinearExpr<T> right) {
    return {LinearExpr<T>(left) - right, CMP_LTEZ};
  }

  template<typename T> static inline
  LinearCon<T>
  operator==(const LinearExpr<T> left, const LinearExpr<T> right) {
    return {left - right, CMP_EQZ};
  }

  template<typename T> static inline
  LinearCon<T>
  operator==(const LinearExpr<T> left, const int right) {
    return {left - LinearExpr<T>(right), CMP_EQZ};
  }

  template<typename T> static inline
  LinearCon<T>
  operator==(const int left, const LinearExpr<T> right) {
    return {LinearExpr<T>(left) - right, CMP_EQZ};
  }

  // ---
  template<typename T> static inline
  LinearCon<T>
  operator<(const LinearExpr<T> left, const LinearExpr<T> right) {
    return {left - right, CMP_LTZ};
  }

  template<typename T> static inline
  LinearCon<T>
  operator<(const LinearExpr<T> left, const int right) {
    return {left - LinearExpr<T>(right), CMP_LTZ};
  }

  template<typename T> static inline
  LinearCon<T>
  operator>(const int left, const LinearExpr<T> right) {
    return {LinearExpr<T>(left) - right, CMP_GTZ};
  }

  template<typename T> static inline
  LinearCon<T>
  operator>(const LinearExpr<T> left, const LinearExpr<T> right) {
    return {left - right, CMP_GTZ};
  }

  template<typename T> static inline
  LinearCon<T>
  operator>(const LinearExpr<T> left, const int right) {
    return {left - LinearExpr<T>(right), CMP_GTZ};
  }

  template<typename T> static inline
  LinearCon<T>
  operator<(const int left, const LinearExpr<T> right) {
    return {LinearExpr<T>(left) - right, CMP_LTZ};
  }
  
  class EventTime {
  public:
    ExecutionAction action;
    bool isEnd;
    int offset;

    void replaceInstruction(Instruction* const toReplace,
                            Instruction* const replacement) {
      if (action.isInstruction() && (action.getInstruction() == toReplace)) {
        action.setInstruction(replacement);
        //instr = replacement;
      }
    }

    void replaceAction(ExecutionAction& toReplace,
                       ExecutionAction& replacement) {
      if (action == toReplace) {
        action = replacement;
      }
    }
    
    bool isStart() const {
      return !isEnd;
    }

    ExecutionAction getAction() const {
      return action;
    }
    
    llvm::Instruction* getInstr() const {
      return action.getInstruction();
    }
  };

  typedef EventTime InstructionTime;

  class SchedulingProblem {
  public:
    int blockNo;

    std::map<ExecutionAction, std::vector<std::string> > actionVarNames;
    std::map<llvm::Instruction*, std::vector<std::string> > schedVarNames;
    std::map<llvm::BasicBlock*, std::vector<std::string> > blockVarNames;
    std::map<PipelineSpec, std::string> IInames;

    std::vector<LinearConstraint> constraints;

    HardwareConstraints hdc;
    bool optimize;
    LinearExpression objectiveFunction;

    std::map<llvm::BasicBlock*, std::vector<llvm::BasicBlock*> > controlPredecessors;
    std::set<TaskSpec> taskSpecs;

    SchedulingProblem(const HardwareConstraints& hcs_) :
      blockNo(0), hdc(hcs_), optimize(false) {}

    SchedulingProblem() : optimize(false) {
      blockNo = 0;
    }

    void setObjective(const LinearExpression& expr) {
      objectiveFunction = expr;
      optimize = true;
    }

    std::string getIIName(llvm::BasicBlock* bb) const {
      //std::string val = dbhc::map_find(bb, IInames);
      for (auto p : IInames) {
        if (dbhc::elem(bb, p.first.blks)) {
          return p.second;
        }
      }

      assert(false);
    }

    std::string getIIName(const PipelineSpec& p) const {
      std::string val = dbhc::map_find(p, IInames);
      return val;
    }

    LinearExpression getII(const PipelineSpec& p) const {
      std::string val = getIIName(p);
      return LinearExpression(val);
    }
    
    LinearExpression getII(llvm::BasicBlock* bb) const {
      for (auto p : IInames) {
        if (dbhc::elem(bb, p.first.blks)) {
          return getII(p.first);
        }
      }

      assert(false);
    }
    
    int blockNumber() const {
      return blockNo;
    }

    LinearExpression blockStart(llvm::BasicBlock* bb) {
      return LinearExpression(dbhc::map_find(bb, blockVarNames).front());
    }

    LinearExpression blockEnd(llvm::BasicBlock* bb) {
      return LinearExpression(dbhc::map_find(bb, blockVarNames).back());
    }

    bool hasAction(const ExecutionAction& action) {
      return dbhc::contains_key(action, actionVarNames);
    }

    void addAction(const ExecutionAction& action) {
      // TODO: Eventually use this for all actions including instructions
      assert(!action.isInstruction());
      actionVarNames[action] = {action.getName() + "_start", action.getName() + "_end"};
    }

    LinearExpression actionStart(const ExecutionAction& action) {
      auto lc = LinearExpression(dbhc::map_find(action, actionVarNames).front());
      //cout << "start of " << action << " is " << lc << endl;
      return lc;
    }

    LinearExpression actionEnd(const ExecutionAction& action) {
      return LinearExpression(dbhc::map_find(action, actionVarNames).back());
    }
    
    LinearExpression instrStart(const ExecutionAction& action) {
      assert(action.isInstruction());
      return LinearExpression(dbhc::map_find(action.getInstruction(), schedVarNames).front());
    }

    LinearExpression instrEnd(const ExecutionAction& action) {
      assert(action.isInstruction());      
      return LinearExpression(dbhc::map_find(action.getInstruction(), schedVarNames).back());
    }
    
    LinearExpression instrStart(llvm::Instruction* instr) {
      return LinearExpression(dbhc::map_find(instr, schedVarNames).front());
    }

    LinearExpression instrStage(llvm::Instruction* instr, const int i) {
      return LinearExpression(dbhc::map_find(instr, schedVarNames).at(i));
    }

    LinearExpression instrEnd(llvm::Instruction* instr) {
      return LinearExpression(dbhc::map_find(instr, schedVarNames).back());
    }

    int numStages(llvm::Instruction* instr) {
      return (int) dbhc::map_find(instr, schedVarNames).size();
    }
    
    void addBasicBlock(llvm::BasicBlock* const bb,
                       std::set<PipelineSpec>& toPipeline);
                       //std::set<llvm::BasicBlock*>& toPipeline);

    void addConstraint(const LinearConstraint& constraint) {
      constraints.push_back(constraint);
    }
  };

  static inline   
  InstructionTime operator+(const InstructionTime t, const int offset) {
    return {t.action, t.isEnd, t.offset + offset};
  }

  static inline   
  InstructionTime operator+(const int offset, const InstructionTime t) {
    return {t.action, t.isEnd, t.offset + offset};
  }

  static inline   
  LinearExpression
  toLinearExpression(const InstructionTime& time,
                     SchedulingProblem& p) {
    if (time.action.isInstruction()) {
      return (time.isEnd ? p.instrEnd(time.action) : p.instrStart(time.action)) + time.offset;
    } else if (time.action.isBasicBlock()) {
      return (time.isEnd ? p.blockEnd(time.action.getBasicBlock()) : p.blockStart(time.action.getBasicBlock())) + time.offset;
    } else {
      if (!p.hasAction(time.action)) {
        p.addAction(time.action);
      }
      return (time.isEnd ? p.actionEnd(time.action) : p.actionStart(time.action)) + time.offset;      
    }
  }

  static inline   
  InstructionTime start(BasicBlock* const bb) {
    return {bb, false, 0};
  }

  static inline   
  InstructionTime end(BasicBlock* const bb) {
    return {bb, true, 0};
  }
  
  static inline   
  InstructionTime instrEnd(Instruction* const instr) {
    return {instr, true, 0};
  }

  static inline   
  InstructionTime instrStart(Instruction* const instr) {
    return {instr, false, 0};
  }

  static inline   
  InstructionTime actionStart(ExecutionAction& action) {
    return {action, false, 0};
  }

  static inline   
  InstructionTime actionEnd(ExecutionAction& action) {
    return {action, true, 0};
  }
  
  static inline   
  InstructionTime instrEnd(llvm::Value* const instr) {
    assert(llvm::Instruction::classof(instr));
    return {llvm::dyn_cast<llvm::Instruction>(instr), true, 0};
  }

  static inline 
  InstructionTime instrStart(llvm::Value* const instr) {
    assert(llvm::Instruction::classof(instr));
    return {llvm::dyn_cast<llvm::Instruction>(instr), false, 0};
  }

  static inline
  std::ostream& operator<<(std::ostream& out, const InstructionTime& t) {
    std::string pre = t.isEnd ? "end" : "start";
    out << pre << "(" << t.action << ") + " << t.offset;
    return out;
  }

  enum ExecutionConstraintType {
    CONSTRAINT_TYPE_ORDERED,
    CONSTRAINT_TYPE_STALL,
    CONSTRAINT_TYPE_ILP,        
  };

  class ExecutionConstraint {
  public:
    virtual void addSelfTo(SchedulingProblem& p, Function* f) = 0;
    virtual ExecutionConstraintType type() const = 0;
    virtual void replaceInstruction(Instruction* const toReplace,
                                    Instruction* const replacement) = 0;

    virtual void replaceAction(ExecutionAction& toReplace,
                               ExecutionAction& replacement) = 0;
    
    virtual ExecutionConstraint* clone() const = 0;
    virtual void print(std::ostream& out) const = 0;    
    virtual ~ExecutionConstraint() {}
  };

  static inline
  std::ostream& operator<<(std::ostream& out, const ExecutionConstraint& c) {
    c.print(out);
    return out;
  }

  class WaitUntil : public ExecutionConstraint {
  public:
    Value* value;
    Instruction* mustWait;
  };

  enum OrderRestriction {
    ORDER_RESTRICTION_SIMULTANEOUS,
    ORDER_RESTRICTION_BEFORE,
    ORDER_RESTRICTION_BEFORE_OR_SIMULTANEOUS,
  };

  static inline
  std::string toString(OrderRestriction r) {
    switch(r) {
    case ORDER_RESTRICTION_SIMULTANEOUS:
      return "==";
    case ORDER_RESTRICTION_BEFORE:
      return "<";
    case ORDER_RESTRICTION_BEFORE_OR_SIMULTANEOUS:
      return "<=";
    default:
      assert(false);
    }
  }

  class Ordered : public ExecutionConstraint {
  public:
    InstructionTime before;
    InstructionTime after;

    map<string, int> pipelineOffsets;
    
    OrderRestriction restriction;

    Ordered(const InstructionTime before_,
            const InstructionTime after_,
            const OrderRestriction restriction_) :
      before(before_),
      after(after_),
      restriction(restriction_) {}

    virtual ExecutionConstraintType type() const override {
      return CONSTRAINT_TYPE_ORDERED;
    }

    virtual void print(std::ostream& out) const override {
      out << before << " " << toString(restriction) << " " << after;
    }

    virtual void addSelfTo(SchedulingProblem& p, Function* f) override;
    
    virtual ExecutionConstraint* clone() const override {
      InstructionTime beforeCpy(before);
      InstructionTime afterCpy(after);      
      return new Ordered(beforeCpy, afterCpy, restriction);
    }
    
    virtual void replaceAction(ExecutionAction& toReplace,
                               ExecutionAction& replacement) override {
      before.replaceAction(toReplace, replacement);
      after.replaceAction(toReplace, replacement);      
    }
    
    virtual void replaceInstruction(Instruction* const toReplace,
                                    Instruction* const replacement) override {
      before.replaceInstruction(toReplace, replacement);
      after.replaceInstruction(toReplace, replacement);      
    }
    
  };
  
  static inline
  Ordered* operator<(InstructionTime before, InstructionTime after) {
    return new Ordered(before, after, ORDER_RESTRICTION_BEFORE);
  }

  static inline
  Ordered* operator==(InstructionTime before, InstructionTime after) {
    return new Ordered(before, after, ORDER_RESTRICTION_SIMULTANEOUS);
  }

  static inline
  Ordered* operator<=(InstructionTime before, InstructionTime after) {
    return new Ordered(before, after, ORDER_RESTRICTION_BEFORE_OR_SIMULTANEOUS);
  }

  static inline
  Ordered* operator>(InstructionTime before, InstructionTime after) {
    return after < before;
  }
  
  class ExecutionConstraints {
  public:
    std::vector<ExecutionConstraint*> constraints;
    std::set<PipelineSpec> toPipeline;
    std::set<TaskSpec> tasks;
    map<BasicBlock*, vector<BasicBlock*> > controlPredecessors;
    
    ExecutionConstraints() {}

    ExecutionConstraints(const ExecutionConstraints& other) {
      for (auto c : other.constraints) {
        addConstraint(c->clone());
      }
      toPipeline = other.toPipeline;
      tasks = other.tasks;
      controlPredecessors = other.controlPredecessors;
    }

    std::string getIIName(BasicBlock* const bb);

    void remove(ExecutionConstraint* c) {
      assert(dbhc::elem(c, constraints));

      dbhc::remove(c, constraints);
      delete c;
    }
    
    void addConstraint(ExecutionConstraint* c) {
      constraints.push_back(c);
    }

    void add(ExecutionConstraint* c) {
      addConstraint(c);
    }
    
    void addConstraints(SchedulingProblem& p,
                        Function* f) {
      for (auto c : constraints) {
        c->addSelfTo(p, f);
      }
    }

    ~ExecutionConstraints() {
      for (auto c : constraints) {
        delete c;
      }
    }
  };

  class Schedule {

  public:
    std::map<llvm::Instruction*, std::vector<int> > instrTimes;
    std::map<llvm::BasicBlock*, std::vector<int> > blockTimes;
    std::map<PipelineSpec, int> pipelineSchedules;

    int defaultReturnState;
    bool hasRetDefault;

    std::map<llvm::BasicBlock*, std::vector<llvm::BasicBlock*> > controlPredecessors;

    SchedulingProblem problem;
    
    Schedule() : hasRetDefault(false) {}

    bool hasReturnDefault() const { return hasRetDefault; }

    void setDefaultReturnState(const int st) {
      defaultReturnState = st;
      hasRetDefault = true;
    }

    int getDefaultReturnState() const {
      assert(hasReturnDefault());
      return defaultReturnState;
    }
    
    int startTime(llvm::Instruction* const instr) const {
      return dbhc::map_find(instr, instrTimes).front();
    }

    int clockTicksToFinish() const {
      int maxFinishTime = 0;
      for (auto b : blockTimes) {
        if (b.second.back() > maxFinishTime) {
          maxFinishTime = b.second.back();
        }
      }
      return maxFinishTime;
    }

    int numStates() const {
      return clockTicksToFinish() + 1;
    }

    void print(std::ostream& out) const {
      out << "Block times" << std::endl;
      for (auto i : blockTimes) {
        out << "\t" << i.first << " -> [";
        out << commaListString(i.second);
        out << "]" << std::endl;
      }

      out << "Instruction times" << std::endl;
      for (auto i : instrTimes) {
        out << "\t" << instructionString(i.first) << " -> [";
        out << commaListString(i.second);
        out << "]" << std::endl;
      }


    }

  };

  static inline std::ostream& operator<<(std::ostream& out, const Schedule& s) {
    s.print(out);
    return out;
  }
  
  Schedule scheduleFunction(llvm::Function* f, HardwareConstraints& hdc);
  Schedule scheduleFunction(llvm::Function* f,
                            HardwareConstraints& hdc,
                            std::set<llvm::BasicBlock*>& toPipeline);
  
  Schedule schedulePipeline(llvm::BasicBlock* const bb, HardwareConstraints& hdc);

  // Logical condition used in state transitions

  class Atom {
  public:

    llvm::Value* cond;
    bool negated;
    Atom() : cond(nullptr), negated(false) {}
    Atom(llvm::Value* const cond_) : cond(cond_), negated(false) {}
    Atom(llvm::Value* const cond_, const bool negated_) :
      cond(cond_), negated(negated_) {}    
  };

  static inline std::ostream& operator<<(std::ostream& out, const Atom& c) {
    if (c.cond != nullptr) {

      if (c.negated) {
        out << "!(";
      }

      if (llvm::Instruction::classof(c.cond)) {
        out << instructionString(static_cast<llvm::Instruction* const>(c.cond));
      } else {
        out << valueString(c.cond);
      }

      if (c.negated) {
        out << ")";
      }

    } else {
      out << "True";
    }
    return out;
  }
  
  class Condition {
  public:

    std::vector<std::vector<Atom> > clauses;
    // llvm::Value* cond;
    // bool negated;
    Condition() : clauses({{}}) {}

    Condition(llvm::Value* const cond_) : clauses({{{cond_, false}}}) {}
    Condition(llvm::Value* const cond_, const bool negated_) :
      clauses({{{cond_, negated_}}}) {}      

    Condition(std::vector<std::vector<Atom > > cl) :
      clauses(cl) {}

    bool isTrue() const {
      return (clauses.size() == 1) && (clauses[0].size() == 0);
    }

  };

  static inline std::ostream& operator<<(std::ostream& out, const Condition& c) {
    if ((c.clauses.size() == 1) && (c.clauses[0].size() == 0)) {
      out << "True" << std::endl;
      return out;
    }

    int nCl = 0;
    for (auto cl : c.clauses) {

      int nAt = 0;

      out << "(";
      for (auto atom : cl) {
        out << atom;
        if (nAt < (int) (cl.size() - 1)) {
          out << " ^ ";
        }
        nAt++;
      }
      out << ")";

      if (nCl < (int) (c.clauses.size() - 1)) {
        out << " v ";
      }
      nCl++;
    }

    return out;
  }

  typedef int StateId;

  class StateTransition {
  public:
    StateId dest;
    Condition cond;
  };

  static inline
  std::ostream&
  operator<<(std::ostream& out, const StateTransition& t) {
    out << t.dest << " if " << t.cond << std::endl;
    return out;
  }

  class Pipeline {
    int ii;
    int stateDepth;
    std::set<StateId> states;    

  public:
    Pipeline(const int ii_,
             const int stateDepth_,
             const std::set<StateId>& states_) :
      ii(ii_), stateDepth(stateDepth_), states(states_) {
      assert(II() >= 1);
      assert(depth() >= 1);
      assert(depth() == (int) states.size());
    }

    StateId startState() const { return *std::min_element(std::begin(states), std::end(states)); }
    StateId endState() const { return *std::max_element(std::begin(states), std::end(states)); }

    int II() const { return ii; }
    int depth() const { return stateDepth; }
    const std::set<StateId>& getStates() const { return states; }
  };

  class SuccessorInfo {
  public:
    StateId destState;
  };
  
  class TransitionInfo {
  public:
    std::map<BasicBlock*, std::vector<SuccessorInfo> > successorInfo;
  };
  
  class StateTransitionGraph {
  public:

    Schedule sched;
    std::map<StateId, std::vector<Instruction*> > opStates;
    std::map<StateId, std::vector<StateTransition> > opTransitions;
    std::vector<Pipeline> pipelines;
    std::map<StateId, TransitionInfo> transitions;
    map<BasicBlock*, int> basicBlockNos;
    
    StateTransitionGraph() {}

    llvm::Function* getFunction() const {
      auto& opSt = *(std::begin(opStates));
      return (opSt.second)[0]->getParent()->getParent();
    }

    bool isEmptyState(const StateId id) {
      return !dbhc::contains_key(id, opStates) ||
        (dbhc::map_find(id, opStates).size() == 0);
    }

    std::set<StateId> statesInTask(TaskSpec& t) {
      std::set<StateId> taskStates;
      for (auto blk : t.blks) {
        for (int s = blockStartState(blk); s <= blockEndState(blk); s++) {
          taskStates.insert(s);
        }
      }

      return taskStates;
    }

    TaskSpec task(const StateId id) {
      for (auto t : sched.problem.taskSpecs) {
        for (StateId inTask : statesInTask(t)) {
          if (inTask == id) {
            return t;
          }
        }
      }
      assert(false);
    }

    llvm::Instruction* pickInstructionAt(const StateId id) {
      auto instrs = opStates[id];
      if (instrs.size() == 0) {
        return nullptr;
      } else {
        return instrs.back();
      }
    }
    
    StateTransitionGraph(const StateTransitionGraph& other) {
      std::cout << "Calling stg const ref constructor" << std::endl;
      sched = other.sched;
      opStates = other.opStates;
      opTransitions = other.opTransitions;
      pipelines = other.pipelines;
      transitions = other.transitions;

      assert(sched.numStates() == other.sched.numStates());
      assert(opStates.size() == other.opStates.size());
      assert(opTransitions.size() == other.opTransitions.size());
      assert(pipelines.size() == other.pipelines.size());      
    }

    StateTransitionGraph(Schedule& sched_) : sched(sched_) {}

    StateId instructionStartState(llvm::Instruction* const instr) {
      return dbhc::map_find(instr, sched.instrTimes).front();
    }

    StateId instructionEndState(llvm::Instruction* const instr) {
      return dbhc::map_find(instr, sched.instrTimes).back();
    }

    StateId blockStartState(llvm::BasicBlock* const block) {
      return dbhc::map_find(block, sched.blockTimes).front();
    }

    StateId blockEndState(llvm::BasicBlock* const block) {
      return dbhc::map_find(block, sched.blockTimes).back();
    }
    
    std::vector<Instruction*>
    instructionsFinishingAt(const StateId id) const {
      std::vector<Instruction*> instrs;
      for (auto st : dbhc::map_find(id, opStates)) {
        llvm::Instruction* instr = st; //.instruction;
        if (id == dbhc::map_find(instr, sched.instrTimes).back()) {
          instrs.push_back(st);
        }
      }
      return instrs;
    }

    std::vector<Instruction*>
    instructionsStartingAt(const StateId id) const {
      std::vector<Instruction*> instrs;
      for (auto st : dbhc::map_find(id, opStates)) {
        llvm::Instruction* instr = st; //.instruction;
        if (id == dbhc::map_find(instr, sched.instrTimes).front()) {
          instrs.push_back(st);
        }
      }
      return instrs;
    }
    
    int numControlStates() const {
      return opStates.size();
    }

    bool hasTransition(const StateId a, const StateId b) const {
      if (!dbhc::contains_key(a, opTransitions)) {
        return false;
      }

      auto nextStates = dbhc::map_find(a, opTransitions);
      for (auto t : nextStates) {
        if (t.dest == b) {
          return true;
        }
      }

      return false;
    }

    void print(std::ostream& out);
    
  };

  typedef StateTransitionGraph STG;

  STG buildSTG(Schedule& sched, llvm::Function* const f);
  
  HardwareConstraints standardConstraints();

  void addDataConstraints(llvm::Function* f, ExecutionConstraints& exe);

  class InterfaceFunctions {
  public:
    std::map<llvm::Function*, ExecutionConstraints> constraints;
    std::map<std::string,
             std::function<void(llvm::Function*, ExecutionConstraints&)> >
    functionTemplates;

    void addFunction(Function* const f) {
      constraints[f] = ExecutionConstraints();
    }

    ExecutionConstraints& getConstraints(llvm::Function* const f) {
      assert(dbhc::contains_key(f, constraints));
      return constraints.find(f)->second;
    }

    bool containsFunction(llvm::Function* const f) const {
      return dbhc::contains_key(f, constraints);
    }
    
  };

  void implementRVFifoRead(llvm::Function* readFifo, ExecutionConstraints& exec);
  void implementRVFifoWrite(llvm::Function* writeFifo, ExecutionConstraints& exec);
  void implementRVFifoWriteTemplate(llvm::Function* writeFifo,
                                    ExecutionConstraints& exec);
  void implementRVFifoWriteRef(llvm::Function* writeFifo,
                               ExecutionConstraints& exec);
  void implementRVFifoReadRef(llvm::Function* readFifo, ExecutionConstraints& exec);
  
  void inlineWireCalls(llvm::Function* f,
                       ExecutionConstraints& exec,
                       InterfaceFunctions& interfaces);

  ModuleSpec fifoSpec(int width, int depth);

  void inlineFunctionWithConstraints(llvm::Function* const f,
                                     ExecutionConstraints& exec,
                                     llvm::CallInst* const toInline,
                                     ExecutionConstraints& constraintsToInline);

  ModuleSpec wireSpec(int width);

  void implementWireRead(Function* readFifo);
  void implementWireWrite(Function* writeFifo);

  void implementRVCompoundRead(llvm::Function* readFifo,
                               ExecutionConstraints& exec,
                               const HardwareConstraints& hcs);

  void implementRVCompoundWrite(llvm::Function* writeFifo,
                                ExecutionConstraints& exec,
                                const HardwareConstraints& hcs);
    
  void implementBusGet(llvm::Function* busGet,
                       ExecutionConstraints& exec,
                       const HardwareConstraints& hcs);

  void implementAXIRead(llvm::Function* axiRead,
                        ExecutionConstraints& exec);

  void implementAXIWrite(llvm::Function* axiWrite,
                         ExecutionConstraints& exec);

  ModuleSpec busSpec(llvm::StructType* tp);

  ModuleSpec streamAxiPackedStencilSpec(const int valueWidth, const int nRows, const int nCols);
  ModuleSpec packedStencilSpec(const int valueWidth, const int nRows, const int nCols);
  ModuleSpec axiPackedStencilSpec(const int valueWidth, const int nRows, const int nCols);
  ModuleSpec stencilSpec(const int valueWidth, const int nRows, const int nCols);

  void implementStencilCall(llvm::Function* stencilCall,
                            ExecutionConstraints& exec);

  void implementStencilWrite(llvm::Function* stencilCall,
                             ExecutionConstraints& exec);

  void implementStencilRead(llvm::Function* stencilCall,
                            ExecutionConstraints& exec);

  void implementStencilSet(llvm::Function* stencilCall,
                           ExecutionConstraints& exec);

  void implementStencilGet(llvm::Function* stencilCall,
                           ExecutionConstraints& exec);

  void implementStencilConstructor(llvm::Function* stencilCall,
                                   ExecutionConstraints& exec);

  void implementStencilSetLast(llvm::Function* stencilCall,
                               ExecutionConstraints& exec);

  void implementRawAXIWrite(llvm::Function* axiWrite,
                            ExecutionConstraints& exec);

  void implementRawAXIRead(llvm::Function* axiWrite,
                           ExecutionConstraints& exec);


  ModuleSpec ramSpec(const int width,
                     const int depth,
                     const int numReadPorts,
                     const int numWritePorts);
  
  ModuleSpec ramSpec(const int width, const int depth);

  void implementRAMRead0(Function* ramRead0, ExecutionConstraints& exec);
  void implementRAMRead1(Function* ramRead1, ExecutionConstraints& exec);
  void implementRAMRead2(Function* ramRead1, ExecutionConstraints& exec);
  void implementRAMWrite0(Function* ramWrite0, ExecutionConstraints& exec);

  Schedule scheduleInterface(llvm::Function* f,
                             HardwareConstraints& hcs,
                             InterfaceFunctions& interfaces,
                             std::set<BasicBlock*>& toPipeline,
                             ExecutionConstraints& exec);
  
  Schedule scheduleInterface(llvm::Function* f,
                             HardwareConstraints& hcs,
                             InterfaceFunctions& interfaces,
                             std::set<BasicBlock*>& toPipeline);
  
  Schedule scheduleInterface(llvm::Function* f,
                             HardwareConstraints& hcs,
                             InterfaceFunctions& interfaces,
                             std::set<BasicBlock*>& toPipeline,
                             ExecutionConstraints& exec);
  
  Schedule scheduleInterface(llvm::Function* f,
                             HardwareConstraints& hcs,
                             InterfaceFunctions& interfaces);

  ModuleSpec axiRamSpec(llvm::StructType* tp);

  ModuleSpec fifoSpec32(llvm::StructType* tp);

  ModuleSpec medianFilterSpec();

  void implementRunMedian(llvm::Function* f, ExecutionConstraints& exec);

  bool precedes(Instruction* pred, Instruction* succ, vector<BasicBlock*> blkOrder);
  
  ModuleSpec ipReceiverSpec();
  ModuleSpec counterSpec();

  void implementLBPush(llvm::Function* const func,
                       ExecutionConstraints& exec);
  void implementLBPop(llvm::Function* const func,
                      ExecutionConstraints& exec);
  
  void implementLBHasValidData(llvm::Function* lbM,
                               ExecutionConstraints& exec);
  
  void implementIncrement(llvm::Function* f, ExecutionConstraints& exec);
  void implementGetAddrsRV(llvm::Function* f, ExecutionConstraints& exec);

  ModuleSpec ramSpecFunc(llvm::StructType* tp);
  ModuleSpec ram2SpecFunc(llvm::StructType* tp);
  ModuleSpec ram3SpecFunc(llvm::StructType* tp);

  void addInputPort(map<string, Port>& ports,
                    const int width,
                    const std::string name);

  void addOutputPort(map<string, Port>& ports,
                     const int width,
                     const std::string name);

  void sequentialCalls(llvm::Function* f,
                       ExecutionConstraints& exec);

  ModuleSpec registerModSpec(const int width);

  map<BasicBlock*, map<BasicBlock*, int> >
  topologicalLevelsForBlocks(const StateId state,
                             STG& stg);

  std::map<BasicBlock*, vector<BasicBlock*> >
  buildControlPreds(llvm::Function* f);

  std::vector<BasicBlock*>
  topologicalSortOfBlocks(llvm::Function* f,
                          std::map<BasicBlock*, std::vector<BasicBlock*> >& controlPredecessors);
  

  std::set<llvm::BasicBlock*> inProgressBlocks(const StateId state,
                                               STG& stg);

  std::set<std::pair<BasicBlock*, BasicBlock*> >
  getOutOfStateTransitions(const StateId state,
                           STG& stg);

    
  SchedulingProblem
  createSchedulingProblem(llvm::Function* f,
                          HardwareConstraints& hdc,
                          std::set<PipelineSpec>& toPipeline,
                          map<BasicBlock*,vector<BasicBlock*>>& controlPredecessors);

  Schedule scheduleFunction(llvm::Function* f,
                            HardwareConstraints& hdc,
                            std::set<PipelineSpec>& toPipeline,
                            std::map<Function*, SchedulingProblem>& constraints);

  std::map<llvm::BasicBlock*, int> numberBasicBlocks(llvm::Function* const f);

  class StateActivationRecord {
  public:
    StateId state;
    llvm::BasicBlock* entryBlock;
    llvm::BasicBlock* priorBlock;
  };

  std::ostream& operator<<(std::ostream& out, const StateActivationRecord& r);

  StateActivationRecord
  buildRecord(BasicBlock* const src, BasicBlock* const dst, STG& stg);

  class CFGJump {
  public:
    std::pair<BasicBlock*, BasicBlock*> jmp;
  };

  bool operator<(const CFGJump& x, const CFGJump& y);

  std::set<CFGJump> possibleLastJumps(const StateId state,
                                      STG& stg);
  
  std::set<BasicBlock*> blocksInState(const StateId state,
                                      STG& stg);

  std::set<std::pair<BasicBlock*, BasicBlock*> >
  getInStateTransitions(const StateId state,
                        STG& stg);

  std::set<BasicBlock*> outOfStatePredecessors(const StateId state,
                                               llvm::BasicBlock* blk,
                                               STG& stg);

  Schedule buildFromModel(SchedulingProblem& p);

  // TODO: Add incremental support for building scheduling constraints?
  Schedule scheduleFunction(llvm::Function* f,
                            HardwareConstraints& hdc,
                            std::set<llvm::BasicBlock*>& toPipeline,
                            std::map<llvm::Function*, SchedulingProblem>& constraints);

  SchedulingProblem
  createSchedulingProblem(llvm::Function* f,
                          HardwareConstraints& hdc,
                          std::set<llvm::BasicBlock*>& toPipeline);

  SchedulingProblem
  createSchedulingProblem(llvm::Function* f,
                          HardwareConstraints& hdc,
                          std::set<PipelineSpec>& toPipeline,
                          std::set<TaskSpec>& tasks,                          
                          map<BasicBlock*, vector<BasicBlock*> >& controlPredecessors);

  std::set<CFGJump> getOutOfTaskJumps(TaskSpec& task, STG& stg);

  TaskSpec getTask(llvm::BasicBlock* const blk,
                   STG& stg);

  std::set<TaskSpec> halideTaskSpecs(llvm::Function* f);

  void addStencilCallConstraints(llvm::Function* f,
                                 map<BasicBlock*, vector<BasicBlock*> >& preds,
                                 ExecutionConstraints& exec);

  void populateHalideStencils(Function* f,
                              InterfaceFunctions& interfaces,
                              HardwareConstraints& hcs);

  std::set<llvm::BasicBlock*> activeOnExitBlocks(const StateId state, STG& stg);

  void
  createMemoryConstraints(llvm::Function* f,
                          HardwareConstraints& hdc,
                          ExecutionConstraints& exe);

  bool isMethod(const std::string& className,
                const std::string& methodName,
                Function* const func);

  int stencilTypeWidth(const std::string& name);
  int stencilNumRows(const std::string& name);
  int stencilNumCols(const std::string& name);
  string streamStencilName(const std::string& streamName);

  std::string blkNameString(BasicBlock* const blk);
  int ramDataWidth(const std::string& ramName);
  int ramAddrWidth(const std::string& ramName);
  int ramDepth(const std::string& ramName);

  struct HalideStencilTp {
  public:
    int typeWidth;
    int nRows;
    int nCols;
  };

  static inline
  int bitWidth(HalideStencilTp stencil) {
    return stencil.typeWidth*stencil.nRows*stencil.nCols;
  }
  
  
  class LBSpec {
  public:
    HalideStencilTp in;
    HalideStencilTp out;
    vector<int> imageBounds;
  };

  //StructType* lbType(const int inWidth, const int outWidth);
  StructType* lbType(LBSpec& spec);

  Instruction* readPort(IRBuilder<>& b,
                        Value* const readMod,
                        const int width,
                        const std::string name);

  //void bindUnits(const STG& stg, HardwareConstraints& hcs);
  
}



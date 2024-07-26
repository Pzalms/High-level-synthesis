#pragma once

#include <fstream>
#include <streambuf>
#include <sstream>

#include <iostream>

#include "expression.h"
#include "llvm_codegen.h"
#include "verilog_backend.h"

using namespace dbhc;
using namespace std;
using namespace llvm;

namespace ahaHLS {

  static inline
  bool oneCharToken(const char c) {
    vector<char> chars = {'{', '}', ';', ')', '(', ',', '[', ']', ':', '-', '&', '+', '=', '>', '<', '*', '.', '%'};
    return dbhc::elem(c, chars);
  }

  template<typename T>
  class ParseState {
    std::vector<T> ts;
    int pos;

  public:

    ParseState(const std::vector<T>& toks) : ts(toks), pos(0) {}

    int currentPos() const { return pos; }
    void setPos(const int position) { pos = position; }

    bool nextCharIs(const Token t) const {
    
      return !atEnd() && (peekChar() == t);
    }

    T peekChar(const int offset) const {
      assert(((int) ts.size()) > (pos + offset));
      return ts[pos + offset];
    }
  
    T peekChar() const { return peekChar(0); }

    T parseChar() {
      assert(((int) ts.size()) > pos);

      T next = ts[pos];
      pos++;
      return next;
    }

    bool atEnd() const {
      return pos == ((int) ts.size());
    }

    int remainderSize() const {
      return ((int) ts.size()) - pos;
    }

    std::string remainder() const {
      stringstream ss;
      for (int i = pos; i < ts.size(); i++) {
        ss << ts.at(i) << " ";
      }
      return ss.str();
      //return ts.substr(pos);
    }
  
  };

  typedef ParseState<char> TokenState;

  static inline
  bool isBinop(const Token t) {
    //vector<string> binopStrings{"=", "==", "+", "&", "-", "/", "^", "%", "&&", "||", "<=", ">=", "<", ">", "*"};
    vector<string> binopStrings{"==", "+", "&", "-", "/", "^", "%", "&&", "||", "<=", ">=", "<", ">", "*", "%"};
    return elem(t.getStr(), binopStrings);
  }

  static inline
  bool isWhitespace(const char c) {
    return isspace(c);
  }

  template<typename OutType, typename TokenType, typename Parser>
  std::vector<OutType> many(Parser p, ParseState<TokenType>& tokens) {
    std::vector<OutType> stmts;

    while (true) {
      auto res = p(tokens);
      if (!res.has_value()) {
        break;
      }

      stmts.push_back(res.get_value());
    }

    return stmts;
  }

  template<typename OutType, typename TokenType, typename Parser>
  maybe<OutType> tryParse(Parser p, ParseState<TokenType>& tokens) {
    int lastPos = tokens.currentPos();
    //cout << "lastPos = " << lastPos << endl;
  
    maybe<OutType> val = p(tokens);
    if (val.has_value()) {
      return val;
    }

    tokens.setPos(lastPos);

    //cout << "try failed, now pos = " << tokens.currentPos() << endl;
    return maybe<OutType>();
  }

  template<typename OutType, typename TokenType, typename StatementParser, typename SepParser>
  std::vector<OutType>
  sepBy(StatementParser stmt, SepParser sep, ParseState<TokenType>& tokens) {
    std::vector<OutType> stmts;
  
    while (!tokens.atEnd()) {
      stmts.push_back(stmt(tokens));
      sep(tokens);
    }

    return stmts;
  }

  template<typename OutType, typename SepType, typename TokenType, typename StatementParser, typename SepParser>
  std::vector<OutType>
  sepBtwn(StatementParser stmt, SepParser sep, ParseState<TokenType>& tokens) {
    std::vector<OutType> stmts;
  
    while (true) {
      OutType nextStmt = stmt(tokens);
      stmts.push_back(nextStmt);
      auto nextSep = tryParse<SepType>(sep, tokens);
      if (!nextSep.has_value()) {
        break;
      }
    }

    return stmts;
  }

  template<typename OutType, typename SepType, typename TokenType, typename StatementParser, typename SepParser>
  std::vector<OutType>
  sepBtwn0(StatementParser stmt, SepParser sep, ParseState<TokenType>& tokens) {
    std::vector<OutType> stmts;

    maybe<OutType> nextStmt = stmt(tokens);
    if (!nextStmt.has_value()) {
      return {};
    } else {
      stmts.push_back(nextStmt.get_value());
    }

    auto nextSep = tryParse<SepType>(sep, tokens);
    if (!nextSep.has_value()) {
      return stmts;
    }
  
    while (true) {
      maybe<OutType> nextStmt = stmt(tokens);
      assert(nextStmt.has_value());
      stmts.push_back(nextStmt.get_value());
      auto nextSep = tryParse<SepType>(sep, tokens);
      if (!nextSep.has_value()) {
        break;
      }
    }

    return stmts;
  }

  template<typename F>
  maybe<Token> consumeWhile(TokenState& state, F shouldContinue) {
    string tok = "";
    while (!state.atEnd() && shouldContinue(state.peekChar())) {
      tok += state.parseChar();
    }
    if (tok.size() > 0) {
      return Token(tok);
    } else {
      return maybe<Token>();
    }
  }

  static inline
  bool isUnderscore(const char c) { return c == '_'; }

  static inline
  bool isAlphaNum(const char c) { return isalnum(c); }

  maybe<Token> parseStr(const std::string target, TokenState& chars);
  maybe<Token> parseComment(TokenState& state);
  std::function<maybe<Token>(TokenState& chars)> mkParseStr(const std::string str);

  static inline
  void consumeWhitespace(TokenState& state) {
    while (true) {
      auto commentM = tryParse<Token>(parseComment, state);
      if (!commentM.has_value()) {
        auto ws = consumeWhile(state, isWhitespace);
        if (!ws.has_value()) {
          return;
        }
      }
    }
  }

  static inline
  Token parse_token(TokenState& state) {
    if (isalnum(state.peekChar())) {
      auto res = consumeWhile(state, [](const char c) { return isAlphaNum(c) || isUnderscore(c); });
      assert(res.has_value());
      return res.get_value();
    } else if (oneCharToken(state.peekChar())) {
      maybe<Token> result = tryParse<Token>(mkParseStr("=="), state);
      if (result.has_value()) {
        return result.get_value();
      }

      result = tryParse<Token>(mkParseStr("<="), state);
      if (result.has_value()) {
        return result.get_value();
      }

      result = tryParse<Token>(mkParseStr(">="), state);
      if (result.has_value()) {
        return result.get_value();
      }
      
      result = tryParse<Token>(mkParseStr("->"), state);
      if (result.has_value()) {
        return result.get_value();
      }
    
      char res = state.parseChar();
      string r;
      r += res;
      return Token(r, TOKEN_TYPE_SYMBOL);
    } else {
      cout << "Cannot tokenize " << state.remainder() << endl;
      assert(false);
    }

  }

  static inline
  std::vector<Token> tokenize(const std::string& classCode) {
    vector<char> chars;
    for (int i = 0; i < (int) classCode.size(); i++) {
      chars.push_back(classCode[i]);
    }
    TokenState state(chars);
    vector<Token> tokens;
  
    while (!state.atEnd()) {
      //cout << "Next char = " << state.peekChar() << endl;
      consumeWhitespace(state);

      if (state.atEnd()) {
        break;
      }

      Token t = parse_token(state);
      //cout << "Next char after token = " << state.peekChar() << endl;
      tokens.push_back(t);
    }

    return tokens;
  }

  enum SynthCppTypeKind {
    SYNTH_CPP_TYPE_KIND_STRUCT,
    SYNTH_CPP_TYPE_KIND_VOID,
    SYNTH_CPP_TYPE_KIND_POINTER,
    SYNTH_CPP_TYPE_KIND_LABEL,
    SYNTH_CPP_TYPE_KIND_BITS,
    SYNTH_CPP_TYPE_KIND_ARRAY
  };

  class SynthCppType {
  public:

    virtual SynthCppTypeKind getKind() const { assert(false); }

    virtual ~SynthCppType() {
    }

    virtual void print(std::ostream& out) const = 0;

  };

  static inline
  std::ostream& operator<<(std::ostream& out, const SynthCppType& tp) {
    tp.print(out);
    return out;
  }

  class TemplateType : public SynthCppType {
  public:
    std::vector<Expression*> exprs;

    TemplateType(const std::vector<Expression*>& exprs_) : exprs(exprs_) {}

    ~TemplateType() {
      for (auto e : exprs) {
        delete e;
      }
    }
  };

  class SynthCppStructType : public SynthCppType {
  public:

    Token name;

    SynthCppStructType(const Token name_) : name(name_) {}

    std::string getName() const { return name.getStr(); }

    virtual SynthCppTypeKind getKind() const override { return SYNTH_CPP_TYPE_KIND_STRUCT; }

    static bool classof(const SynthCppType* const tp) { return tp->getKind() == SYNTH_CPP_TYPE_KIND_STRUCT; }

    virtual void print(std::ostream& out) const override {
      out << name;
    }
  
  };

  class SynthCppArrayType : public SynthCppType {
  public:

    SynthCppType* underlying;
    Expression* len;

    SynthCppArrayType(SynthCppType* underlying_, Expression* len_) : underlying(underlying_), len(len_) {}

    virtual SynthCppTypeKind getKind() const override { return SYNTH_CPP_TYPE_KIND_ARRAY; }

    static bool classof(const SynthCppType* const tp) { return tp->getKind() == SYNTH_CPP_TYPE_KIND_ARRAY; }

    virtual void print(std::ostream& out) const override {
      out << *underlying << "[" << *len << "]";
    }
  
  };
  
  class LabelType : public SynthCppType {
  public:
    static bool classof(const SynthCppType* const tp) {
      return tp->getKind() == SYNTH_CPP_TYPE_KIND_LABEL;
    }

    virtual SynthCppTypeKind getKind() const override { return SYNTH_CPP_TYPE_KIND_LABEL; }

    virtual void print(std::ostream& out) const override {
      out << "label_type";
    }
  };

  class SynthCppPointerType : public SynthCppType {
  public:

    SynthCppType* elementType;
    SynthCppPointerType(SynthCppType* elem_) : elementType(elem_) {}

    SynthCppType* getElementType() const { return elementType; }

    static bool classof(const SynthCppType* const tp) { return tp->getKind() == SYNTH_CPP_TYPE_KIND_POINTER; }
  
    virtual SynthCppTypeKind getKind() const override { return SYNTH_CPP_TYPE_KIND_POINTER; }

    virtual void print(std::ostream& out) const override {
      out << *getElementType() << "&";
    }
  
  };

  class SynthCppDataType : public SynthCppType {
  public:

    static bool classof(const SynthCppType* const tp) { return (tp->getKind() == SYNTH_CPP_TYPE_KIND_BITS) ||
        (tp->getKind() == SYNTH_CPP_TYPE_KIND_VOID); }

    virtual void print(std::ostream& out) const override {
      out << "data_type";
    }

  };

  class SynthCppBitsType : public SynthCppDataType {
  public:

    int width;

    SynthCppBitsType(const int width_) : width(width_) {}

    int getWidth() const { return width; }

    static bool classof(const SynthCppType* const tp) { return (tp->getKind() == SYNTH_CPP_TYPE_KIND_BITS); }
  
    virtual SynthCppTypeKind getKind() const override { return SYNTH_CPP_TYPE_KIND_BITS; }

    virtual void print(std::ostream& out) const override {
      out << "bits[" << getWidth() << "]";
    }
  
  };

  class VoidType : public SynthCppDataType {
  public:

    static bool classof(const SynthCppType* const tp) { return tp->getKind() == SYNTH_CPP_TYPE_KIND_VOID; }
  
    virtual SynthCppTypeKind getKind() const override { return SYNTH_CPP_TYPE_KIND_VOID; }

    virtual void print(std::ostream& out) const override {
      out << "void";
    }
  
  };

  enum StatementKind {
    STATEMENT_KIND_CLASS_DECL,
    STATEMENT_KIND_IF,
    STATEMENT_KIND_BLOCK,    
    STATEMENT_KIND_PIPELINE,
    STATEMENT_KIND_FUNCTION_DECL,
    STATEMENT_KIND_HAZARD_DECL,    
    STATEMENT_KIND_ARG_DECL,
    STATEMENT_KIND_STRUCT_DECL,      
    STATEMENT_KIND_ASSIGN,
    STATEMENT_KIND_ASSIGN_DECL,    
    STATEMENT_KIND_FOR,
    STATEMENT_KIND_EXPRESSION,
    STATEMENT_KIND_RETURN,
    STATEMENT_KIND_DO_WHILE,
  };

  class Statement {
  public:
    bool hasL;
    Token label;

    Statement() : hasL(false) {}

    virtual void print(std::ostream& out) const {
      assert(false);
    }

    Token getLabel() const { return label; }

    bool hasLabel() const { return hasL; }

    void setLabel(const Token l) {
      hasL = true;
      label = l;
    }

    virtual StatementKind getKind() const {
      assert(false);
    }

    virtual ~Statement() {
    }

  };

  static inline
  std::ostream& operator<<(std::ostream& out, const Statement& stmt) {
    stmt.print(out);
    return out;
  }

  class ClassDecl : public Statement {
  public:
    Token name;
    std::vector<Statement*> body;

    ClassDecl(Token name_, std::vector<Statement*>& body_) :
      name(name_), body(body_) {}

    static bool classof(const Statement* const stmt) {
      return stmt->getKind() == STATEMENT_KIND_CLASS_DECL;
    }

    virtual StatementKind getKind() const {
      return STATEMENT_KIND_CLASS_DECL;
    }

    virtual void print(std::ostream& out) const {
      out << "class " << name << "{ INSERT_BODY; }";
    }
  
  };

  class AssignStmt : public Statement {
  public:
    //Token var;
    Expression* lhs;
    Expression* expr;

    //AssignStmt(Token lhs_, Expression* expr_) : lhs(lhs_), expr(expr_) {}
    AssignStmt(Expression* lhs_, Expression* expr_) : lhs(lhs_), expr(expr_) {}

    static bool classof(const Statement* const stmt) {
      return stmt->getKind() == STATEMENT_KIND_ASSIGN;
    }

    virtual StatementKind getKind() const {
      return STATEMENT_KIND_ASSIGN;
    }

    virtual void print(std::ostream& out) const {
      out << *lhs << " = " << *expr;
    }
  
  };

  class AssignDeclStmt : public Statement {
  public:
    //Token var;
    SynthCppType* tp;
    Expression* lhs;
    Expression* expr;

    //AssignStmt(Token lhs_, Expression* expr_) : lhs(lhs_), expr(expr_) {}
    AssignDeclStmt(SynthCppType* tp_, Expression* lhs_, Expression* expr_) : tp(tp_), lhs(lhs_), expr(expr_) {}

    static bool classof(const Statement* const stmt) {
      return stmt->getKind() == STATEMENT_KIND_ASSIGN_DECL;
    }

    virtual StatementKind getKind() const {
      return STATEMENT_KIND_ASSIGN_DECL;
    }

    virtual void print(std::ostream& out) const {
      out << *tp << " " << *lhs << " = " << *expr << ";";
    }
  
  };
  
  class BlockStmt : public Statement {
  public:
    vector<Statement*> body;

    BlockStmt(const std::vector<Statement*>& body_) : body(body_) {}

    static bool classof(const Statement* const stmt) {
      return stmt->getKind() == STATEMENT_KIND_BLOCK;
    }

    virtual StatementKind getKind() const {
      return STATEMENT_KIND_BLOCK;
    }

    virtual void print(std::ostream& out) const {
      out << "{" << endl;
      for (auto stmt : body) {
        out << tab(1) << *stmt << endl;
      }
      out << "}" << endl;
    }

  };
  
  class IfStmt : public Statement {
  public:
    Expression* test;
    Statement* body;
    Statement* elseClause;

    IfStmt(Expression* const test_, Statement* const body_, Statement* const elseClause_) : test(test_), body(body_), elseClause(elseClause_) {}

    static bool classof(const Statement* const stmt) {
      return stmt->getKind() == STATEMENT_KIND_IF;
    }

    virtual StatementKind getKind() const {
      return STATEMENT_KIND_IF;
    }

    virtual void print(std::ostream& out) const {
      out << "IF (INSERT) { INSERT; }";
    }

  };
  
  class ForStmt : public Statement {
  public:
    Statement* init;
    Expression* exitTest;
    Statement* update;
    std::vector<Statement*> stmts;

    ForStmt(Statement* init_,
            Expression* exitTest_,
            Statement* update_,
            std::vector<Statement*>& stmts_) :
      init(init_), exitTest(exitTest_), update(update_), stmts(stmts_) {}

    static bool classof(const Statement* const stmt) {
      return stmt->getKind() == STATEMENT_KIND_FOR;
    }

    virtual StatementKind getKind() const {
      return STATEMENT_KIND_FOR;
    }

    virtual void print(std::ostream& out) const {
      out << "for (INSERT) { INSERT; }";
    }
  
  };

  class ExpressionStmt : public Statement {
  public:
    Expression* expr;

    ExpressionStmt(Expression* expr_) : expr(expr_) {}

    static bool classof(const Statement* const stmt) {
      return stmt->getKind() == STATEMENT_KIND_EXPRESSION;
    }

    virtual StatementKind getKind() const {
      return STATEMENT_KIND_EXPRESSION;
    }

    virtual void print(std::ostream& out) const {
      out << *expr;
    }
  };

  maybe<Statement*> parseStatement(ParseState<Token>& tokens);

  class ArgumentDecl : public Statement {
  public:

    SynthCppType* tp;
    Token name;
    Expression* arraySize;

    ArgumentDecl(SynthCppType* tp_, Token name_) : tp(tp_), name(name_), arraySize(nullptr) {}
    ArgumentDecl(SynthCppType* tp_, Token name_, Expression* arraySize_) :
      tp(tp_), name(name_), arraySize(arraySize_) {}

    virtual void print(std::ostream& out) const {
      out << "ARGDECL " << name;
    }
  
    static bool classof(const Statement* const stmt) {
      return stmt->getKind() == STATEMENT_KIND_ARG_DECL;
    }

    virtual StatementKind getKind() const {
      return STATEMENT_KIND_ARG_DECL;
    }
  
  };

  class ReturnStmt : public Statement {
  public:
    Expression* returnVal;

    ReturnStmt() : returnVal(nullptr) {}
    ReturnStmt(Expression* expr) : returnVal(expr) {}

    static bool classof(const Statement* const stmt) {
      return stmt->getKind() == STATEMENT_KIND_RETURN;
    }

    virtual StatementKind getKind() const {
      return STATEMENT_KIND_RETURN;
    }

    virtual void print(std::ostream& out) const {
      out << "return " << *returnVal;
    }
  
  };


  class StructDecl : public Statement {
  public:
    Token name;
    std::vector<ArgumentDecl*> fields;

    StructDecl(Token name_,
               std::vector<ArgumentDecl*>& fields_) :
      name(name_),
      fields(fields_) {}

    std::string getName() const { return name.getStr(); }

    static bool classof(const Statement* const stmt) {
      return stmt->getKind() == STATEMENT_KIND_STRUCT_DECL;
    }
  
    virtual StatementKind getKind() const {
      return STATEMENT_KIND_STRUCT_DECL;
    }
  };
  
  class FunctionDecl : public Statement {
  public:
    SynthCppType* returnType;
    Token name;
    std::vector<ArgumentDecl*> args;
    std::vector<Statement*> body;

    FunctionDecl(SynthCppType* returnType_,
                 Token name_,
                 std::vector<ArgumentDecl*>& args_,
                 std::vector<Statement*>& body_) :
      returnType(returnType_),
      name(name_),
      args(args_),
      body(body_) {}

    std::string getName() const { return name.getStr(); }

    static bool classof(const Statement* const stmt) {
      return stmt->getKind() == STATEMENT_KIND_FUNCTION_DECL;
    }
  
    virtual StatementKind getKind() const {
      return STATEMENT_KIND_FUNCTION_DECL;
    }
  

  };

  class HazardDecl : public Statement {
  public:
    std::vector<ArgumentDecl*> args;
    std::vector<Statement*> body;

    HazardDecl(std::vector<ArgumentDecl*>& args_,
               std::vector<Statement*>& body_) :
      args(args_),
      body(body_) {}

    static bool classof(const Statement* const stmt) {
      return stmt->getKind() == STATEMENT_KIND_HAZARD_DECL;
    }
  
    virtual StatementKind getKind() const {
      return STATEMENT_KIND_HAZARD_DECL;
    }
  

  };
  
  class ParserModule {
    std::vector<Statement*> stmts;
  
  public:

    ParserModule(const std::vector<Statement*>& stmts_) : stmts(stmts_) {}

    std::vector<Statement*> getStatements() const { return stmts; }

    ~ParserModule() {
      for (auto stmt : stmts) {
        delete stmt;
      }
    }
  };

  static inline
  std::ostream& operator<<(std::ostream& out, const ParserModule& mod) {
    return out;
  }

  static inline
  void parseStmtEnd(ParseState<Token>& tokens) {
    //cout << "Statement end token == " << tokens.peekChar() << endl;
    assert(tokens.parseChar() == Token(";"));
  }

  static inline
  maybe<Token> parseComma(ParseState<Token>& tokens) {
    Token t = tokens.parseChar();
    if (t.getStr() == ",") {
      return t;
    }

    return maybe<Token>();
  }


  static inline
  maybe<Token> parseSemicolon(ParseState<Token>& tokens) {
    Token t = tokens.parseChar();
    if (t.getStr() == ";") {
      return t;
    }

    return maybe<Token>();
  }

  static inline
  maybe<Identifier*> parseId(ParseState<Token>& tokens) {
    Token t = tokens.parseChar();
    if (t.isId()) {
      return new Identifier(t);
    }

    return maybe<Identifier*>();
  }

  maybe<FunctionCall*> parseFunctionCall(ParseState<Token>& tokens);

  maybe<Expression*> parseFieldAccess(ParseState<Token>& tokens);
  
  static inline
  maybe<Expression*> parseMethodCall(ParseState<Token>& tokens) {
    Token t = tokens.parseChar();
    if (!t.isId()) {
      return maybe<Expression*>();
    }

    Token delim = tokens.parseChar();
    if ((delim != Token("->")) &&
        (delim != Token("."))) {
      return maybe<Expression*>();
    }

    //cout << "-- In method call, parsing function call " << tokens.remainder() << endl;
    maybe<FunctionCall*> fCall = parseFunctionCall(tokens);
    if (fCall.has_value()) {
      return new MethodCall(t, fCall.get_value());
    }

    return maybe<Expression*>();
  }

  Expression* parseExpression(ParseState<Token>& tokens);
  maybe<Expression*> parseExpressionMaybe(ParseState<Token>& tokens);

  static inline
  maybe<Expression*> parsePrimitiveExpressionMaybe(ParseState<Token>& tokens) {
    //cout << "-- Parsing primitive expression " << tokens.remainder() << endl;

    if (tokens.atEnd()) {
      return maybe<Expression*>();
    }
  
    if (tokens.nextCharIs(Token("("))) {
      tokens.parseChar();

      //cout << "Inside parens " << tokens.remainder() << endl;
      auto inner = parseExpressionMaybe(tokens);
      if (inner.has_value()) {
        if (tokens.nextCharIs(Token(")"))) {
          tokens.parseChar();
          return inner;
        }
      }
      return maybe<Expression*>();
    }

  
    auto fCall = tryParse<FunctionCall*>(parseFunctionCall, tokens);
    if (fCall.has_value()) {
      return fCall.get_value();
    }

    //cout << "---- Trying to parse method call " << tokens.remainder() << endl;
    auto mCall = tryParse<Expression*>(parseMethodCall, tokens);
    if (mCall.has_value()) {
      return mCall;
    }

    auto faccess = tryParse<Expression*>(parseFieldAccess, tokens);
    if (faccess.has_value()) {
      return faccess;
    }
    
    // Try parsing a function call
    // If that does not work try to parse an identifier
    // If that does not work try parsing a parenthesis
    auto id = tryParse<Identifier*>(parseId, tokens);
    if (id.has_value()) {
      return id.get_value();
    }

    //cout << "Expressions = " << tokens.remainder() << endl;
    if (!tokens.atEnd() && tokens.peekChar().isNum()) {
      return new IntegerExpr(tokens.parseChar().getStr());
    }

    return maybe<Expression*>();
  }

  static inline
  int precedence(Token op) {
    map<string, int> prec{{"+", 100}, {"==", 99}, {"-", 100}, {"*", 100}, {"<", 99}, {">", 99}, {"<=", 99}, {">=", 99}, {"%", 100}};
    assert(contains_key(op.getStr(), prec));
    return map_find(op.getStr(), prec);
  }

  static inline
  Expression* popOperand(vector<Expression*>& postfixString) {
    assert(postfixString.size() > 0);

    Expression* top = postfixString.back();
    postfixString.pop_back();
  
    auto idM = extractM<Identifier>(top);
    if (idM.has_value() && isBinop(idM.get_value()->getName())) {
      auto rhs = popOperand(postfixString);
      auto lhs = popOperand(postfixString);
      return new BinopExpr(lhs, idM.get_value()->getName(), rhs);
    }

    return top;
  }

  static inline
  int getDataWidth(const std::string& name) {
    if (hasPrefix(name, "bit_")) {
      return stoi(name.substr(4));
    } else if (hasPrefix(name, "sint_")) {
      return stoi(name.substr(5));
    } else {
      assert(hasPrefix(name, "uint_"));
      return stoi(name.substr(5)); 
    }
  }

  static inline
  maybe<SynthCppType*> parseBaseType(ParseState<Token>& tokens) {
    if (tokens.peekChar().isId()) {
    
      //cout << tokens.peekChar() << " is id" << endl;    

      Token tpName = tokens.parseChar();

      if (tokens.atEnd() || (tokens.peekChar() != Token("<"))) {
        string name = tpName.getStr();
        SynthCppType* base = nullptr;
        if (hasPrefix(name, "bit_") || hasPrefix(name, "sint_") || hasPrefix(name, "uint_")) {
          int width = getDataWidth(name);
          base = new SynthCppBitsType(width);
        } else {
          base = new SynthCppStructType(tpName);          
        }

        assert(base != nullptr);

        if (tokens.nextCharIs(Token("["))) {
          cout << "Got array start at " << tokens.remainder() << endl;
          tokens.parseChar();
          auto arraySize = parseExpressionMaybe(tokens);
          if (!arraySize.has_value() || !tokens.nextCharIs(Token("]"))) {
            cout << "No value for array size?" << endl;
            return maybe<SynthCppType*>();
          } else {
            tokens.parseChar();
            return new SynthCppArrayType(base, arraySize.get_value());
          }
        } else {
          return base; //new SynthCppStructType(tpName);
        }
      }

    }

    if (tokens.peekChar() == Token("void")) {
      tokens.parseChar();
      return new VoidType();
    }

    return maybe<SynthCppType*>();
  }

  static inline
  maybe<SynthCppType*> parseType(ParseState<Token>& tokens) {
    auto tp = parseBaseType(tokens);

    // Check if its a pointer
    if (!tokens.atEnd() && (tokens.peekChar() == Token("&"))) {
      tokens.parseChar();
      return new SynthCppPointerType(tp.get_value());
    }

    return tp;
  }

  static inline
  maybe<ArgumentDecl*> parseArgDeclMaybe(ParseState<Token>& tokens) {
    // cout << "Parsing arg declaration = " << tokens.remainder() << endl;
    // cout << "Remaining tokens = " << tokens.remainderSize() << endl;
    maybe<SynthCppType*> tp = parseType(tokens);

    if (!tp.has_value()) {
      return maybe<ArgumentDecl*>();
    }


    //cout << "After parsing type = " << tokens.remainder() << endl;

    Token argName = tokens.parseChar();

    if (!argName.isId()) {
      return maybe<ArgumentDecl*>();
    }

    //cout << "After parsing expression = " << tokens.remainder() << endl;
    
    if (tokens.peekChar() == Token("[")) {
      tokens.parseChar();

      auto e = parseExpressionMaybe(tokens);

      //cout << "After parsing expression = " << tokens.remainder() << endl;
      if (!e.has_value()) {
        return maybe<ArgumentDecl*>();
      }

      if (tokens.peekChar() == Token("]")) {
        tokens.parseChar();

        return new ArgumentDecl(tp.get_value(), argName, e.get_value());
      } else {
        return maybe<ArgumentDecl*>();      
      }
    
    }

    return new ArgumentDecl(tp.get_value(), argName);
  }

  static inline
  ArgumentDecl* parseArgDecl(ParseState<Token>& tokens) {
    auto d = parseArgDeclMaybe(tokens);
    if (d.has_value()) {
      return d.get_value();
    }

    assert(false);
  }

  static inline
  maybe<Statement*> parseFuncDecl(ParseState<Token>& tokens) {
    maybe<SynthCppType*> tp = tryParse<SynthCppType*>(parseType, tokens);
    if (!tp.has_value()) {
      return maybe<Statement*>();
    }

    // Create function declaration
    Token funcName = tokens.parseChar();

    if (!funcName.isId()) {
      return maybe<Statement*>();
    }

    if (tokens.peekChar() == Token("(")) {
      assert(tokens.parseChar() == Token("("));

      vector<ArgumentDecl*> classStmts =
        sepBtwn0<ArgumentDecl*, Token>(parseArgDeclMaybe, parseComma, tokens);

      assert(tokens.parseChar() == Token(")"));

      assert(tokens.parseChar() == Token("{"));

      vector<Statement*> funcStmts =
        many<Statement*>(parseStatement, tokens);

      assert(tokens.parseChar() == Token("}"));
    
      return new FunctionDecl(tp.get_value(), funcName, classStmts, funcStmts);
    }

    return maybe<Statement*>();

  }

  static inline
  maybe<Token> parseLabel(ParseState<Token>& tokens) {
    if (tokens.atEnd()) {
      return maybe<Token>();
    }

    Token t = tokens.parseChar();
    if (!t.isId()) {
      return maybe<Token>();
    }

    if (tokens.atEnd()) {
      return maybe<Token>();
    }
  
    Token semi = tokens.parseChar();
    if (semi != Token(":")) {
      return maybe<Token>();
    }

    return t;
  }

  static inline
  maybe<Statement*> parseFunctionCallStmt(ParseState<Token>& tokens) {
    maybe<FunctionCall*> p = parseFunctionCall(tokens);
    if (!p.has_value()) {
      return maybe<Statement*>();
    }

    Token semi = tokens.parseChar();
    if (semi != Token(";")) {
      return maybe<Statement*>();
    }

    return new ExpressionStmt(static_cast<FunctionCall*>(p.get_value()));
  }

  static inline
  maybe<Statement*> parseAssignStmt(ParseState<Token>& tokens) {
    //cout << "Starting parse assign \" " << tokens.remainder() << "\"" << endl;

    if (tokens.atEnd()) {
      return maybe<Statement*>();
    }

    maybe<Expression*> lhs = parseExpressionMaybe(tokens);
    if (!lhs.has_value()) {
      return maybe<Statement*>();
    }
    // Token id = tokens.peekChar();
    // if (!id.isId()) {
    //   return maybe<Statement*>();
    // }
    // tokens.parseChar();

    //cout << "After name remainder is \"" << tokens.remainder() << "\"" << endl;

    if (!tokens.nextCharIs(Token("="))) {
      return maybe<Statement*>();
    }
    tokens.parseChar();

    //cout << "Remaining after eq is " << tokens.remainder() << endl;
    auto r = parseExpressionMaybe(tokens);
    if (!r.has_value()) {
      cout << "No expr" << endl;
      return maybe<Statement*>();
    }

    //return new AssignStmt(id, r.get_value());
    return new AssignStmt(lhs.get_value(), r.get_value());

  }

  class PipelineBlock : public Statement {
  public:
    Expression* ii;
    vector<Statement*> body;

    PipelineBlock(Expression* e_,
                  std::vector<Statement*>& stmts_) : ii(e_), body(stmts_) {}

    static bool classof(const Statement* const stmt) {
      return stmt->getKind() == STATEMENT_KIND_PIPELINE;
    }
  
    virtual StatementKind getKind() const {
      return STATEMENT_KIND_PIPELINE;
    }
    
  };
    
  class DoWhileLoop : public Statement {
  public:

    Expression* test;
    vector<Statement*> body;
  
    DoWhileLoop(Expression* e_,
                std::vector<Statement*>& stmts_) : test(e_), body(stmts_) {}
  
    static bool classof(const Statement* const stmt) {
      return stmt->getKind() == STATEMENT_KIND_DO_WHILE;
    }
  
    virtual StatementKind getKind() const {
      return STATEMENT_KIND_DO_WHILE;
    }

  };

  static inline
  maybe<Statement*> parseDoWhileLoop(ParseState<Token>& tokens) {
    //cout << "Parsing do = " << tokens.remainder() << endl;
  
    if (tokens.nextCharIs(Token("do"))) {
      tokens.parseChar();

      if (!tokens.nextCharIs(Token("{"))) {
        return maybe<Statement*>();
      }
      tokens.parseChar();

      vector<Statement*> stmts =
        many<Statement*>(parseStatement, tokens);

      if (!tokens.nextCharIs(Token("}"))) {
        return maybe<Statement*>();
      }
      tokens.parseChar();

      if (!tokens.nextCharIs(Token("while"))) {
        return maybe<Statement*>();
      }
      tokens.parseChar();

      maybe<Expression*> expr = parseExpression(tokens);
      if (expr.has_value()) {

        if (!tokens.nextCharIs(Token(";"))) {
          return maybe<Statement*>();
        }
        tokens.parseChar();
      
        return new DoWhileLoop(expr.get_value(), stmts);
      }

      return maybe<Statement*>();
    
    }

    return maybe<Statement*>();
  }

  static inline
  ParserModule parse(const std::vector<Token>& tokens) {
    ParseState<Token> pm(tokens);
    vector<Statement*> stmts =
      many<Statement*>(parseStatement, pm);

    if (!pm.atEnd()) {
      cout << "Error: Not at end of module after parsing, remainder = " << endl;
      cout << pm.remainder() << endl;
    }
    assert(pm.atEnd());
    
    ParserModule m(stmts);
  
    return m;
  }


  // llvm::Type* llvmPointerFor(SynthCppType* const tp) {
  //   //return llvmTypeFor(tp)->getPointerTo();
  //   // if (SynthCppStructType::classof(tp)) {
  //   //   return structType(static_cast<SynthCppStructType* const>(tp)->getName())->getPointerTo();
  //   // } else {
  //   //   cout << "Getting llvm pointer for " << *tp << endl;
  //   //   assert(false);
  //   // }
  // }

  static inline
  bool isPrimitiveStruct(SynthCppStructType*  const st) {
    string name = st->getName();
    return hasPrefix(name, "bit_") || hasPrefix(name, "sint_") || hasPrefix(name, "uint_");
  }

  static inline
  int getWidth(SynthCppStructType* tp) {
    assert(isPrimitiveStruct(tp));
    string name = tp->getName();
    if (hasPrefix(name, "bit_")) {
      return stoi(name.substr(4));
    } else if (hasPrefix(name, "sint_")) {
      return stoi(name.substr(5));
    } else {
      assert(hasPrefix(name, "uint_"));
      return stoi(name.substr(5));    
    }
  }

  class SynthCppFunction {
  public:
    Token nameToken;
    llvm::Function* func;
    std::map<std::string, SynthCppType*> symtab;
    SynthCppType* retType;
    ExecutionConstraints* constraints;
    set<PipelineSpec> pipelines;

    bool hasReturnType() {
      return retType != nullptr;
    }

    bool hasReturnValue() {
      return (retType != nullptr) && (!VoidType::classof(retType));
    }
  
    SynthCppType* returnType() {
      assert(retType != nullptr);
      return retType;
    }
  
    llvm::Function* llvmFunction() { return func; }

    std::string getName() const {
      return nameToken.getStr();
    }
  };

  static inline
  std::ostream& operator<<(std::ostream& out, const SynthCppFunction& f) {
    out << " " << f.getName() << "(ARGS) {" << endl;
    out << "}" << endl;

    return out;
  }

  class SynthCppClass {
  public:

    Token name;
    std::map<std::string, SynthCppType*> memberVars;
    std::map<std::string, SynthCppFunction*> methods;
    vector<HazardSpec> hazards;
    StructType* llvmTp;
    std::vector<std::string> fieldOrder;

    StructType* llvmStructType() { return llvmTp; }

    std::string getName() const { return name.getStr(); }

    SynthCppType* getType() { return new SynthCppStructType(name); }
    
    SynthCppFunction* getMethod(const Token name) {
      cout << "Getting method for " << name << endl;
      return map_find(name.getStr(), methods);
    }

    int getPortWidth(Token vName) {
      for (auto v : memberVars) {
        cout << "Checking member var " << v.first << endl;
        if (v.first == vName.getStr()) {
          cout << "Found member variable " << v.first << " with type " << *(v.second) << endl;
          assert(SynthCppStructType::classof(v.second));
          string name = sc<SynthCppStructType>(v.second)->getName();
          assert(hasPrefix(name, "input_") || (hasPrefix(name, "output_")));
          if (hasPrefix(name, "input_")) {
            return stoi(name.substr(string("input_").size()));
          } else {
            return stoi(name.substr(string("output_").size()));          
          }
        }
      }

      cout << "Error: No member named " << vName << " in " << name << endl;
      assert(false);
    }
  };

  static inline
  std::ostream& operator<<(std::ostream& out, const SynthCppClass& mod) {
    out << "class " << mod.getName() << "{" << endl;
    for (auto v : mod.methods) {
      out << tab(1) << v.first << " : " << *(v.second) << ";" << endl;
    }
    out << "};" << endl;

    return out;
  }

  static inline
  bool isInputPort(SynthCppType* const tp) {
    auto ps = extractM<SynthCppStructType>(tp);
    if (!ps.has_value()) {
      return false;
    }

    string name = ps.get_value()->getName();
    if (hasPrefix(name, "input_")) {
      return true;
    }

    return false;
  }

  static inline
  bool isOutputPort(SynthCppType* const tp) {
    auto ps = extractM<SynthCppStructType>(tp);
    if (!ps.has_value()) {
      return false;
    }

    string name = ps.get_value()->getName();
    if (hasPrefix(name, "output_")) {
      return true;
    }

    return false;
  }

  static inline
  bool isPortType(SynthCppType* const tp) {
    return isOutputPort(tp) || isInputPort(tp);
  }

  static inline
  int portWidth(SynthCppType* const tp) {
    assert(isPortType(tp));
    auto portStruct = extract<SynthCppStructType>(tp);

    string name = portStruct->getName();
    if (isInputPort(tp)) {
      string prefix = "input_";
      string width = name.substr(prefix.size());
      return stoi(width);
    } else {
      assert(isOutputPort(tp));

      string prefix = "output_";
      string width = name.substr(prefix.size());
      return stoi(width);
    }
  }

  static inline
  SynthCppStructType* extractBaseStructType(SynthCppType* tp) {
    if (SynthCppStructType::classof(tp)) {
      return sc<SynthCppStructType>(tp);
    }

    assert(SynthCppPointerType::classof(tp));
    auto pTp = sc<SynthCppPointerType>(tp);
    auto underlying = pTp->getElementType();

    assert(SynthCppStructType::classof(underlying));

    return sc<SynthCppStructType>(underlying);
  }

  class SymbolTable {
  public:
    std::vector<std::map<std::string, SynthCppType*>* > tableStack;

    void pushTable(std::map<std::string, SynthCppType*>* table) {
      tableStack.push_back(table);
    }

    void popTable() {
      tableStack.pop_back();
    }

    void setType(const std::string& name, SynthCppType* tp) {
      tableStack.back()->insert({name, tp});
    }

    void print(std::ostream& out) {
      for (auto ts : tableStack) {
        out << tab(1) << "Table stack" << endl;
        for (auto str : *ts) {
          out << tab(2) << "symbol = " << str.first << endl;
        }
      }
    }

    SynthCppType* getType(const std::string& str) {
      int depth = ((int) tableStack.size()) - 1;

      for (int i = depth; i >= 0; i--) {
        if (contains_key(str, *(tableStack.at(i)))) {
          return map_find(str, *(tableStack.at(i)));
        }
      }

      cout << "Error: Cannot find type for " << str << endl;
      print(cout);
      assert(false);
    }

  
  };


  class CodeGenState {
  public:

    SynthCppClass* activeClass;
    SynthCppFunction* activeFunction;
    BasicBlock* activeBlock;
    int globalNum;
    std::map<std::string, SynthCppType*> globalVars;

    SymbolTable symtab;

    bool inPipeline;
    PipelineSpec activePipeline;

    CodeGenState() : activeBlock(nullptr), inPipeline(false) {
      symtab.pushTable(&globalVars);
    }

    bool inTopLevel() const { return activeBlock == nullptr; }
    
    BasicBlock& getActiveBlock() const { return *activeBlock; }
    void setActiveBlock(BasicBlock* blk) {
      activeBlock = blk;
    }

    IRBuilder<> builder() { return IRBuilder<>(activeBlock); }

    void pushClassContext(SynthCppClass* ac) {
      activeClass = ac;
    }

    void popClassContext() {
      activeClass = nullptr;
    }

    SynthCppClass* getActiveClass() {
      return activeClass;
    }
  };

  pair<string, int> extractDefault(Statement* stmt);
  
  class SynthCppModule {

    HardwareConstraints hcs;
    InterfaceFunctions interfaces;
  
  public:

    llvm::LLVMContext context;
    unique_ptr<llvm::Module> mod;
    std::vector<SynthCppClass*> classes;
    std::vector<SynthCppFunction*> functions;
    std::set<BasicBlock*> blocksToPipeline;

    CodeGenState cgs;
    SynthCppFunction* activeFunction;
    int globalNum;
    std::map<Token, Instruction*> labelsToInstructions;
    std::map<Token, BasicBlock*> labelsToBlockStarts;
    std::map<Token, BasicBlock*> labelsToBlockEnds;
    std::map<llvm::Type*, StructDecl*> structDefs;
    std::map<llvm::GlobalVariable*, int> globalVarValues;
    
    std::string uniqueNumString() {
      auto s = std::to_string(globalNum);
      globalNum++;
      return s;
    }

    void genLLVMCopyTo(llvm::Value* receiver,
                       llvm::Value* source) {
    
      cgs.builder().CreateCall(mkFunc({receiver->getType(), source->getType()}, voidType(), "copy_" + typeString(receiver->getType())), {receiver, source});
    }

    llvm::Value* bvCastTo(Value* value, Type* const destTp) {
      int vWidth = getTypeBitWidth(value->getType());
      int rWidth = getTypeBitWidth(destTp);

      if (vWidth == rWidth) {
        return value;
      }

      if (vWidth < rWidth) {
        return cgs.builder().CreateSExt(value, destTp);
      }

      assert(vWidth > rWidth);
    
      return cgs.builder().CreateTrunc(value, destTp);
    }

    int constantLength(Expression* expr);

    llvm::Type* llvmTypeFor(SynthCppType* const tp) {
      if (SynthCppPointerType::classof(tp)) {
        return llvmTypeFor(sc<SynthCppPointerType>(tp)->getElementType())->getPointerTo();
        //return llvmPointerFor(static_cast<SynthCppPointerType* const>(tp)->getElementType());
      } else if (VoidType::classof(tp)) {
        return voidType();
      } else if (SynthCppBitsType::classof(tp)) {
        return intType(extract<SynthCppBitsType>(tp)->getWidth());
      } else if (SynthCppArrayType::classof(tp)) {
        auto* arrTp = sc<SynthCppArrayType>(tp);

        int len = constantLength(arrTp->len);
        return ArrayType::get(llvmTypeFor(arrTp->underlying), len);
      } else {
        assert(SynthCppStructType::classof(tp));
        auto st = static_cast<SynthCppStructType* const>(tp);

        for (auto c : getClasses()) {
          if (c->getName() == st->getName()) {
            return c->llvmStructType();
          }
        }

        cout << "Could not get llvm type for SynthCppStructType = " << *st << endl;

        Type* argTp = structType(st->getName());
        return argTp;

      }
    }

    
    // argNum could be 1 if the function being synthesized is actually a
    // method
    void setArgumentSymbols(IRBuilder<>& b,
                            map<string, SynthCppType*>& symtab,
                            vector<ArgumentDecl*>& args,
                            Function* f,
                            int argOffset) {

      cout << "f = " << endl;
      cout << valueString(f) << endl;
      cout << "argOffset = " << argOffset << endl;
      //int argNum = argOffset;
      int argNum = 0;    
      for (auto argDecl : args) {
        cout << "\targ = " << argDecl->name << endl;

        // Create local copy if argument is passed by value
        // For arguments originally passed by pointer:
        // - Add the underlying type to the value map (setValue)
        // For arguments originally passed by value
        // - Create internal copy
        symtab[argDecl->name.getStr()] = argDecl->tp;
        cout << "Setting " << argDecl->name.getStr() << " in symbol table" << endl;
        if (!SynthCppPointerType::classof(argDecl->tp)) {
          auto val = cgs.builder().CreateAlloca(llvmTypeFor(argDecl->tp));
          cgs.builder().CreateStore(getArg(f, argNum + argOffset), val);
          setValue(argDecl->name, val);
        } else {
          setValue(argDecl->name, getArg(f, argNum + argOffset));
        }

        argNum++;
      }

    }


    llvm::Value* structFieldPtr(llvm::Value* baseVal, const std::string& fieldName);
    
    llvm::Value* genFieldAccess(FieldAccess* const e);    
    void addHazard(HazardDecl* hazard, ModuleSpec& cSpec);
    void addMethodDecl(FunctionDecl* decl, ModuleSpec& cSpec);

    void addAssignDecl(AssignDeclStmt* const stmt);
    void addStructDecl(StructDecl* const stmt);    

    vector<Type*> functionInputs(FunctionDecl* fd) {
      vector<Type*> inputTypes;
      for (auto argDecl : fd->args) {
        cout << "\targ = " << argDecl->name << endl;
        Type* argTp = llvmTypeFor(argDecl->tp);

        if (!SynthCppPointerType::classof(argDecl->tp)) {
          assert(SynthCppDataType::classof(argDecl->tp));
          inputTypes.push_back(argTp);
        } else {
          inputTypes.push_back(argTp);
        }
      }

      return inputTypes;
    }

    
    SynthCppModule(ParserModule& parseRes) {
      activeFunction = nullptr;

      globalNum = 0;
      hcs = standardConstraints();
    
      mod = llvm::make_unique<Module>("synth_cpp", context);
      setGlobalLLVMContext(&context);
      setGlobalLLVMModule(mod.get());

      for (auto stmt : parseRes.getStatements()) {

        if (ClassDecl::classof(stmt)) {
          ClassDecl* decl = static_cast<ClassDecl* const>(stmt);
          ModuleSpec cSpec;

          SynthCppClass* c = new SynthCppClass();
          c->name = decl->name;
          cgs.symtab.pushTable(&(c->memberVars));
          cgs.pushClassContext(c);

          vector<Type*> fields;
        
          for (auto subStmt: decl->body) {
            if (ArgumentDecl::classof(subStmt)) {
              auto decl = sc<ArgumentDecl>(subStmt);
              c->memberVars[decl->name.getStr()] = decl->tp;
              c->fieldOrder.push_back(decl->name.getStr());
              
            } else if (FunctionDecl::classof(subStmt)) {
              auto methodFuncDecl = sc<FunctionDecl>(subStmt);
              addMethodDecl(methodFuncDecl, cSpec);
            } else if (HazardDecl::classof(subStmt)) {
              cout << "Found hazard" << endl;
              auto haz = sc<HazardDecl>(subStmt);
              addHazard(haz, cSpec);
            } else {
              assert(subStmt != nullptr);
              cout << "Unsupported statement in class, stmt kind: "  << subStmt->getKind() << endl;
              cout << *stmt << endl;
              assert(false);
            }
          }
          classes.push_back(c);

          // TODO: Wrap this up in to a function
          // Add interface class module spec to hardware constraints
          bool notIClass = true;
          for (auto vTp : c->memberVars) {
            auto tp = vTp.second;
            if (isPortType(tp)) {
              notIClass = false;
              if (isOutputPort(tp)) {
                addOutputPort(cSpec.ports, portWidth(tp), vTp.first);
              } else {
                assert(isInputPort(tp));

                // Set non-default values to be insensitive
                string portName = vTp.first;
                if (!contains_key(portName, cSpec.defaultValues)) {
                  cSpec.insensitivePorts.insert(portName);
                }
                addInputPort(cSpec.ports, portWidth(tp), vTp.first);
              }
            }
          }


          // If this is not an interface class then do not make it an opaque struct
          if (notIClass) {
            for (auto vTp : c->memberVars) {
              fields.push_back(llvmTypeFor(vTp.second));
            }

            c->llvmTp = StructType::create(fields, c->getName(), true);

            cSpec = registerModSpec(getTypeBitWidth(c->llvmTp));
          } else {

            cSpec.name = c->getName();
            cSpec.hasClock = true;
            cSpec.hasRst = true;

            
            c->llvmTp = structType(c->getName());
          }
          

          cout << "class has name " << c->getName() << endl;
          hcs.typeSpecs[c->getName()] = [cSpec](StructType* tp) { return cSpec; };
          hcs.hasTypeSpec(c->getName());



          cgs.symtab.popTable();
          cgs.popClassContext();
        
        } else if (FunctionDecl::classof(stmt)) {
          auto fd = static_cast<FunctionDecl*>(stmt);
          vector<Type*> inputTypes = functionInputs(fd);
          auto sf = new SynthCppFunction();
          sf->nameToken = fd->name;
          //activeSymtab = &(sf->symtab);
          cgs.symtab.pushTable(&(sf->symtab));

          // Add return type as first argument
          llvm::Function* f = mkFunc(inputTypes, llvmTypeFor(fd->returnType), sf->getName());

          // Add this function to the interface functions
          interfaces.addFunction(f);
          sf->constraints = &interfaces.getConstraints(f);

          auto bb = addBB("entry_block", f);
          cgs.setActiveBlock(bb);
        
          //IRBuilder<> b(bb);

          //bool hasReturn = sf->hasReturnValue();
          auto bd = cgs.builder();
          setArgumentSymbols(bd, sf->symtab, fd->args, f, 0);

          // Now need to iterate over all statements in the body creating labels
          // that map to starts and ends of statements
          // and also adding code for each statement to the resulting function, f.
          sf->func = f;        
          activeFunction = sf;
          cout << "# of statements = " << fd->body.size() << endl;
          for (auto stmt : fd->body) {
            cout << "Statement" << endl;
            genLLVM(stmt);
          }

          BasicBlock* activeBlock = &(cgs.getActiveBlock());
          //&(activeFunction->llvmFunction()->getEntryBlock());
          if (activeBlock->getTerminator() == nullptr) {
            cout << "Basic block " << valueString(activeBlock) << "has no terminator" << endl;
            cgs.builder().CreateRet(nullptr);
          }

          // Set all calls to be sequential by default
          //sequentialCalls(f, interfaces.getConstraints(f));

          functions.push_back(sf);
          activeFunction = nullptr;

          sanityCheck(f);

          cgs.symtab.popTable();
        } else if (StructDecl::classof(stmt)) {
          addStructDecl(sc<StructDecl>(stmt));
        } else if (AssignDeclStmt::classof(stmt)) {
          addAssignDecl(sc<AssignDeclStmt>(stmt));
        } else {
          cout << "Error: Unsupported statement kind " << stmt->getKind() << " in module" << endl;
          assert(false);
        }
      }

    }

    map<std::string, llvm::Value*> valueMap;

    bool hasValue(Token t) const {
      return contains_key(t.getStr(), valueMap);
    }

    llvm::Value* getValueFor(Token t) {
      cout << "Getting value for " << t.getStr() << endl;
      assert(contains_key(t.getStr(), valueMap));
      return map_find(t.getStr(), valueMap);
    }

    void setValue(Token t, llvm::Value* v) {
      valueMap[t.getStr()] = v;
      assert(contains_key(t.getStr(), valueMap));    
    }

    EventTime eventFromLabel(string arg0Name, const string& labelName) {
      if (contains_key(Token(arg0Name), labelsToInstructions)) {
        Instruction* instrLabel = map_find(Token(arg0Name), labelsToInstructions);
        cout << "Instruction labeled = " << valueString(instrLabel) << endl;
      
        if (labelName == "start") {
          return {ExecutionAction(instrLabel), false, 0};
        } else {
          assert(labelName == "end");
          return {ExecutionAction(instrLabel), true, 0};
        }
      } else {
        assert(contains_key(Token(arg0Name), labelsToBlockStarts));
        assert(contains_key(Token(arg0Name), labelsToBlockEnds));

        if (labelName == "start") {
          BasicBlock* blkLabel = map_find(Token(arg0Name), labelsToBlockStarts);
          return {ExecutionAction(blkLabel), false, 0};        
        } else {
          assert(labelName == "end");

          BasicBlock* blkLabel = map_find(Token(arg0Name), labelsToBlockEnds);
          return {ExecutionAction(blkLabel), true, 0};
        }
      }
    }

    EventTime parseEventTime(Expression* const e) {
      auto mBop = extractM<BinopExpr>(e);
      if (mBop.has_value()) {
        auto bop = mBop.get_value();
        Token op = bop->op;
        EventTime tm = parseEventTime(bop->lhs);
        cout << "Integer rhs type = " << bop->rhs->getKind() << endl;
        IntegerExpr* val = extract<IntegerExpr>(bop->rhs);

        return tm + val->getInt();
      } else {
        cout << "Call type = " << e->getKind() << endl;      
        auto labelCall = extract<FunctionCall>(e);
        string name = labelCall->funcName.getStr();
        cout << "name = " << name << endl;
        assert((name == "start") || (name == "end"));

        Expression* arg0 = labelCall->args.at(0);
        cout << "arg0Kind = " << arg0->getKind() << endl;
        auto arg0Id = extract<Identifier>(arg0);
        string arg0Name = arg0Id->getName();

        cout << "got arg0 name = " << arg0Name << endl;

        return eventFromLabel(arg0Name, name);
      }
    }

    ExecutionConstraint* parseConstraint(Expression* const e) {
      BinopExpr* bop = extract<BinopExpr>(e);
      Token op = bop->op;
      cout << "Parsing binop = " << op << endl;
      assert(isComparator(op));

      Expression* lhs = bop->lhs;
      EventTime lhsTime = parseEventTime(lhs);
      Expression* rhs = bop->rhs;
      EventTime rhsTime = parseEventTime(rhs);

      if (op.getStr() == "==") {
        return lhsTime == rhsTime;
      } else if (op.getStr() == "<") {
        return lhsTime < rhsTime;
      } else {
        assert(false);
      }
    }

    bool isData(Type* const tp) {
      if (IntegerType::classof(tp)) {
        return true;
      }

      if (StructType::classof(tp)) {
        StructType* stp = dyn_cast<StructType>(tp);
        return stp->elements().size() > 0;
      }

      return false;
    }

    llvm::Value* genLLVM(Expression* const e);

    SynthCppType* getTypeForId(const Token id) {
      if (id.getStr() == "this") {
        assert(cgs.getActiveClass() != nullptr);
        return new SynthCppPointerType(cgs.getActiveClass()->getType());
      }
      return cgs.symtab.getType(id.getStr());
    }

    SynthCppClass* getClass(const std::string id) {
      for (auto c : classes) {
        if (c->getName() == id) {
          return c;
        }
      }

      cout << "Error: Cannot find class with name " << id << endl;
      assert(false);
    }
    
    SynthCppClass* getClassByName(const Token id) {
      for (auto c : classes) {
        if (c->getName() == id.getStr()) {
          return c;
        }
      }

      cout << "Error: Cannot find class with name " << id << endl;
      assert(false);
    }

    SynthCppClass* getClass(StructType* const classType) {
      for (auto c : classes) {
        cout << "Candidate class = " << c->getName() << endl;
        if (c->getName() == classType->getName()) {
          return c;
        }
      }

      // If token has the type of the class we are currently
      // parsing
      if (cgs.getActiveClass()->getName() == classType->getName()) {
        return cgs.getActiveClass();
      }

      assert(false);
    }

    SynthCppClass* getClass(const Token id) {
      string idName = id.getStr();
      cout << "Getting class for " << idName << endl;
      SynthCppType* tp = getTypeForId(id);

      cout << "Got type for " << idName << endl;

      SynthCppStructType* classType = extractBaseStructType(tp);

      assert(StructType::classof(llvmTypeFor(classType)));
      return getClass(dyn_cast<StructType>(llvmTypeFor(classType)));

      // cout << "Got class type = " << *classType << endl;
      
      // for (auto c : classes) {
      //   cout << "Candidate class = " << c->name << endl;
      //   if (c->name == classType->name) {
      //     return c;
      //   }
      // }

      // // If token has the type of the class we are currently
      // // parsing
      // if (cgs.getActiveClass()->name == classType->name) {
      //   return cgs.getActiveClass();
      // }

      // assert(false);
    }

    BasicBlock*
    addBB(const std::string& name, llvm::Function* const f);

    void pushPipeline(const int II);
    void popPipeline();

    void genLLVM(BlockStmt* const blk);    
    void genLLVM(ForStmt* const stmt) {
      Statement* init = stmt->init;
      Expression* exitTest = stmt->exitTest;
      Statement* update = stmt->update;
      auto stmts = stmt->stmts;
    
      auto bd = cgs.builder();

      // Need entry block to check
      auto loopTestBlock = addBB( "for_blk_init_test_" + uniqueNumString(), activeFunction->llvmFunction());
      auto loopBodyBlock = addBB( "for_body_" + uniqueNumString(), activeFunction->llvmFunction());
      auto nextBlock = addBB( "for_body_" + uniqueNumString(), activeFunction->llvmFunction());    

      genLLVM(init);
      bd.CreateBr(loopTestBlock);

      cgs.setActiveBlock(loopTestBlock);
      auto testCond = genLLVM(exitTest);
      cgs.builder().CreateCondBr(testCond, loopBodyBlock, nextBlock);

      cgs.setActiveBlock(loopBodyBlock);
      for (auto stmt : stmts) {
        genLLVM(stmt);
      }
      genLLVM(update);
      cgs.builder().CreateBr(loopTestBlock);

      // Need major work block

      // Need exit block
      cgs.setActiveBlock(nextBlock);
    }
  
    void genLLVM(ArgumentDecl* const decl) {
      auto bd = cgs.builder();
    
      string valName = decl->name.getStr();
      Type* tp = llvmTypeFor(decl->tp);
      // TODO: add to name map?
      cgs.symtab.setType(valName, decl->tp);
      auto n = bd.CreateAlloca(tp, nullptr, valName);
      // Add to constraints?
      int width = getTypeBitWidth(tp);
      setMemSpec(n, getHardwareConstraints(), {0, 0, 1, 1, width, 1, false, {{{"width", std::to_string(width)}}, "register"}});
      setValue(decl->name, n);
    }

    void genLLVM(AssignStmt* const stmt) {

      // Note: Should really be an expression
      //Token t = stmt->var;
      Expression* lhs = stmt->lhs;
      Expression* newVal = stmt->expr;

      Value* v = genLLVM(newVal);

      if (stmt->hasLabel()) {
        Token l = stmt->label;
        cout << "Label on assign = " << stmt->label << endl;

        // TODO: Remove this hack and replace with something more general
        //BasicBlock* blk = &(activeFunction->llvmFunction()->getEntryBlock());
        BasicBlock* blk = &(cgs.getActiveBlock());
        cout << "Block for label = " << valueString(blk) << endl;
        Instruction* last = &(blk->back());

        cout << "Last instruction" << valueString(last) << endl;
        labelsToInstructions[stmt->label] = last;
      }

      Value* tV = pointerToLocation(lhs); //getValueFor(t);

      genSetCode(tV, v);
    }

    llvm::Value* pointerToLocation(Expression* const e);

    void genLLVM(ReturnStmt* const stmt) {
      auto bd = cgs.builder();

      Expression* retVal = stmt->returnVal;
      Value* v = nullptr;
      if (retVal != nullptr) {
        v = genLLVM(retVal);
      }
      bd.CreateRet(v);
    
      if (stmt->hasLabel()) {
        Token l = stmt->label;
        cout << "Label on assign = " << stmt->label << endl;
        BasicBlock* blk = &(cgs.getActiveBlock());
        cout << "Block for label = " << valueString(blk) << endl;
        Instruction* last = &(blk->back());

        cout << "Last instruction" << valueString(last) << endl;
        labelsToInstructions[stmt->label] = last;
      }

    }
  
    void genSetCode(Value* receiver, Value* value) {
      auto bd = cgs.builder();
      // Check types?
    
      Type* rType = receiver->getType();
      Type* vType = value->getType();

      cout << "receiver = " << valueString(receiver) << endl;
      cout << "value    = " << valueString(value) << endl;    
      cout << "rType = " << typeString(rType) << endl;
      cout << "vType = " << typeString(vType) << endl;

      if (rType == vType->getPointerTo()) {
        bd.CreateStore(value, receiver);
      } else {
        // Cast values of different lengths
        assert(PointerType::classof(rType));
        auto underlying = dyn_cast<PointerType>(rType)->getElementType();
        if (isData(underlying) &&
            isData(vType)) {

          auto valueCast = bvCastTo(value, underlying);
          assert(rType == valueCast->getType()->getPointerTo());
          bd.CreateStore(valueCast, receiver);
        } else {
          cout << "Error: Incompatible types in assignment, receiver "
               << valueString(receiver) << ", value " << valueString(value) << endl;
        }
      }
    }

    void genLLVM(DoWhileLoop* stmt) {
      cout << "Do while loop" << endl;
      Expression* test = stmt->test;
      vector<Statement*> stmts = stmt->body;

      auto lastBlock = cgs.builder();
      auto loopBlock =
        addBB("while_loop_" + uniqueNumString(), activeFunction->llvmFunction());
      lastBlock.CreateBr(loopBlock);

      cgs.setActiveBlock(loopBlock);

      // Create exit block
      auto nextBlock =
        addBB("after_while_" + uniqueNumString(), activeFunction->llvmFunction());

      for (auto stmt : stmts) {
        genLLVM(stmt);
      }

      // End of loop
      auto exitCond = genLLVM(test);
      cgs.builder().CreateCondBr(exitCond, loopBlock, nextBlock);
    
      cgs.setActiveBlock(nextBlock);

      if (stmt->hasLabel()) {
        Token label = stmt->getLabel();
        labelsToBlockStarts[label] = loopBlock;
        labelsToBlockEnds[label] = loopBlock;
      }
    }

    void genLLVM(PipelineBlock* const stmt);
    void genLLVM(IfStmt* const stmt);
    
    void genLLVM(Statement* const stmt) {
      assert(stmt != nullptr);
      
      auto bd = cgs.builder();
    
      if (ExpressionStmt::classof(stmt)) {
        auto es = static_cast<ExpressionStmt* const>(stmt);
        Expression* e = es->expr;
        genLLVM(e);


        if (stmt->hasLabel()) {
          Token l = stmt->label;
          cout << "Label = " << stmt->label << endl;
          //BasicBlock* blk = &(activeFunction->llvmFunction()->getEntryBlock());
          BasicBlock* blk = &(cgs.getActiveBlock());        
          cout << "Block for label = " << valueString(blk) << endl;
          Instruction* last = &(blk->back());

          cout << "Last instruction" << valueString(last) << endl;
          labelsToInstructions[stmt->label] = last;
        }
      
      } else if (ArgumentDecl::classof(stmt)) {
        auto decl = static_cast<ArgumentDecl* const>(stmt);
        genLLVM(decl);
      } else if (ForStmt::classof(stmt)) {
        auto loop = static_cast<ForStmt* const>(stmt);
        genLLVM(loop);
      } else if (AssignStmt::classof(stmt)) {
        auto asg = static_cast<AssignStmt* const>(stmt);
        genLLVM(asg);
      
      } else if (ReturnStmt::classof(stmt)) {
        genLLVM(sc<ReturnStmt>(stmt));
      } else if (DoWhileLoop::classof(stmt)) {
        genLLVM(sc<DoWhileLoop>(stmt));
      } else if (PipelineBlock::classof(stmt)) {
        genLLVM(sc<PipelineBlock>(stmt));
      } else if (IfStmt::classof(stmt)) {
        genLLVM(sc<IfStmt>(stmt));
      } else if (BlockStmt::classof(stmt)) {
        genLLVM(sc<BlockStmt>(stmt));
      } else {
        // Add support for variable declarations, assignments, and for loops
        cout << "No support for code generation for statement" << endl;
        assert(false);
      }

      if (stmt->hasLabel()) {
        cout << "Label = " << stmt->label << endl;
        // How do I tag statement starts and ends with a label?
        // Crude: Track the active block in CodeGenState and just attach
        // the start and end to the last instruction?
      }
    
    }

    SynthCppFunction* builtinStub(std::string name, std::vector<llvm::Type*>& args, SynthCppType* retType) {
      SynthCppFunction* stub = new SynthCppFunction();
      stub->nameToken = Token(name);
      stub->retType = retType;
      stub->func = mkFunc(args, llvmTypeFor(retType), name);
  
      return stub;
    }

    
    SynthCppFunction* getFunction(const std::string& name) {
      cout << "Getting function for " << name << endl;

      // TODO: Generalize to work for any width input
      if (name == "set_port") {
        vector<Type*> inputs =
          {intType(32)->getPointerTo(), intType(32)->getPointerTo()};
        SynthCppFunction* stb = builtinStub("set_port", inputs, new VoidType());
        return stb;
      }

      if (name == "add_constraint") {
        vector<Type*> inputs =
          {intType(32)->getPointerTo()};
        SynthCppFunction* stb = builtinStub("add_constraint", inputs, new VoidType());
      
        return stb;
      }


      if (name == "start") {
        vector<Type*> inputs =
          {intType(32)->getPointerTo()};
        SynthCppFunction* stb = builtinStub("start", inputs, new SynthCppStructType(Token("bit_32")));
        return stb;
      }

      if (name == "end") {
        vector<Type*> inputs =
          {intType(32)->getPointerTo()};
        SynthCppFunction* stb = builtinStub("end", inputs, new SynthCppStructType(Token("bit_32")));
        return stb;
      }

      if (name == "read_port") {
        vector<Type*> inputs =
          {intType(32)->getPointerTo()};
        SynthCppFunction* stb = builtinStub("read_port", inputs, new SynthCppStructType(Token("bit_32")));
        return stb;
      }
    
      for (auto f : functions) {
        cout << "f name = " << f->getName() << endl;
        cout << "name   = " << name << endl;
        if (f->getName() == name) {
          return f;
        }
      }
      assert(false);
    }

    HardwareConstraints& getHardwareConstraints() {
      return hcs;
    }

    InterfaceFunctions& getInterfaceFunctions() {
      return interfaces;
    }

    std::set<BasicBlock*>& getBlocksToPipeline() {
      return blocksToPipeline;
    }
  
    std::vector<SynthCppClass*> getClasses() const {
      return classes;
    }

    std::vector<SynthCppFunction*> getFunctions() const {
      return functions;
    }

  };

  MicroArchitecture
  synthesizeVerilog(SynthCppModule& scppMod, const std::string& funcName);

  MicroArchitecture
  synthesizeVerilog(SynthCppModule& scppMod,
                    const std::string& funcName,
                    VerilogDebugInfo& info);
  
  STG buildSTGFor(SynthCppModule& scppMod, SynthCppFunction* const f);  
  STG buildSTGFor(SynthCppModule& mod, const std::string& funcName);
  STG scheduleBanzai(SynthCppModule& mod, const std::string& funcName);

  void optimizeStores(llvm::Function* f);
  void optimizeModuleLLVM(llvm::Module& mod);

  void clearExecutionConstraints(llvm::Function* const f,
                                 ExecutionConstraints& exec);
  
}

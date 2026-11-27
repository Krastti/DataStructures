#ifndef DATASTRUCTURES_AST_VISITORS_H
#define DATASTRUCTURES_AST_VISITORS_H

namespace ast {

struct NumberLiteral;
struct Identifier;
struct StringLiteral;
struct Binary;
struct Remember;
struct Print;
struct When;
struct RepeatWhile;
struct For;
struct Call;
struct Function;
struct Update;
struct UpdateStatement;

struct ExprVisitor {
  virtual ~ExprVisitor() = default;

  virtual void visit(const NumberLiteral& node) = 0;
  virtual void visit(const Identifier& node) = 0;
  virtual void visit(const StringLiteral& node) = 0;
  virtual void visit(const Call& node) = 0;
  virtual void visit(const Binary& node) = 0;
  virtual void visit(const Update& node) = 0;
};

struct StatementVisitor {
  virtual ~StatementVisitor() = default;

  virtual void visit(const Remember& node) = 0;
  virtual void visit(const Print& node) = 0;
  virtual void visit(const When& node) = 0;
  virtual void visit(const RepeatWhile& node) = 0;
  virtual void visit(const For& node) = 0;
  virtual void visit(const Function& node) = 0;
  virtual void visit(const UpdateStatement& node) = 0;
};

}

#endif //DATASTRUCTURES_AST_VISITORS_H

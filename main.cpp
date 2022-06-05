#include "llvm/ADT/APFloat.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Type.h"
#include "llvm/IR/Verifier.h"
#include <algorithm>
#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <map>
#include <memory>
#include <string>
#include <vector>

#include "spooky.h"
#include "hashtable.h"
#include "dirent_for_windows.h"

using namespace llvm;
//My Lexer

const char* Keywords[] = { "array"
,"and"
,"as"
,"ast"
,"atom"
,"become"
,"big"
,"biop"
,"boxed"
,"break"
,"byte"
,"cast"
,"call"
,"case"
,"class"
,"cut"
,"declare"
,"delete"
,"default"
,"do"
,"continue"
,"constant"
,"continuation"
,"ctree"
,"else"
,"elseif"
,"endfunction"
,"endgenerator"
,"endswitch"
,"endif"
,"endwhile"
,"function"
,"generator"
,"if"
,"index"
,"integer"
,"interface"
,"list"
,"logical"
,"match"
,"maybe"
,"mod"
,"new"
,"nil"
,"no"
,"not"
,"object"
,"of"
,"or"
,"one"
,"pointer"
,"post"
,"pre"
,"real"
,"record"
,"returning"
,"send"
,"sizeof"
,"string"
,"super"
,"switch"
,"table"
,"then"
,"to"
,"unify"
,"until"
,"value"
,"var"
,"where"
,"whether"
,"while"
,"bxor"
,"yes"
,"-"
,"*"
,"&"
,"!"
,"~"
,"++"
,"--"
,"="
,"+="
,"-="
, "*="
,"/="
,"mod="
,"<<="
,">>="
,"band="
,"bor="
,"bxor="
,"^"
,"<="
,">="
,"<"
,">"
,"=="
,"not="
,"<=>"
,".."
,"."
,"|"
,">>"
,"<<"
,"+"
,"/"
,"?="
,"?"
,"#"
,"#|"
,"`"
,","
,"::"
,"("
,")"
,"["
,"]"
,"{"
,"}"
,"'"
,":"
,";"
,nullptr
};

SolidAsciiTokenizer Tokenizer(false, Keywords);

//===----------------------------------------------------------------------===//
// Lexer
//===----------------------------------------------------------------------===//
#ifdef OH_OH_NO
// The lexer returns tokens [0-255] if it is an unknown character, otherwise one
// of these for known things.
enum Token {
    tok_eof = -1,

    // commands
    tok_def = -2,
    tok_extern = -3,

    // primary
    tok_identifier = -4,
    tok_number = -5
};

static std::string IdentifierStr; // Filled in if tok_identifier
static double NumVal;             // Filled in if tok_number

/// gettok - Return the next token from standard input.
static int gettok() {
    static int LastChar = ' ';

    // Skip any whitespace.
    while (isspace(LastChar))
        LastChar = getchar();

    if (isalpha(LastChar)) { // identifier: [a-zA-Z][a-zA-Z0-9]*
        IdentifierStr = LastChar;
        while (isalnum((LastChar = getchar())))
            IdentifierStr += LastChar;

        if (IdentifierStr == "def")
            return tok_def;
        if (IdentifierStr == "extern")
            return tok_extern;
        return tok_identifier;
    }

    if (isdigit(LastChar) || LastChar == '.') { // Number: [0-9.]+
        std::string NumStr;
        do {
            NumStr += LastChar;
            LastChar = getchar();
        } while (isdigit(LastChar) || LastChar == '.');

        NumVal = strtod(NumStr.c_str(), nullptr);
        return tok_number;
    }

    if (LastChar == '#') {
        // Comment until end of line.
        do
            LastChar = getchar();
        while (LastChar != EOF && LastChar != '\n' && LastChar != '\r');

        if (LastChar != EOF)
            return gettok();
    }

    // Check for end of file.  Don't eat the EOF.
    if (LastChar == EOF)
        return tok_eof;

    // Otherwise, just return the character as its ascii value.
    int ThisChar = LastChar;
    LastChar = getchar();
    return ThisChar;
}
#endif
//===----------------------------------------------------------------------===//
// Abstract Syntax Tree (aka Parse Tree)
//===----------------------------------------------------------------------===//

namespace {

    /// ExprAST - Base class for all expression nodes.
    class ExprAST {
    public:
        virtual ~ExprAST() = default;

        virtual Value* codegen() = 0;
    };

    /// NumberExprAST - Expression class for numeric literals like "1.0".
    class NumberExprAST : public ExprAST {
        double Val;

    public:
        NumberExprAST(double Val) : Val(Val) {}

        Value* codegen() override;
    };

    /// VariableExprAST - Expression class for referencing a variable, like "a".
    class VariableExprAST : public ExprAST {
        std::string Name;

    public:
        VariableExprAST(const std::string& Name) : Name(Name) {}

        Value* codegen() override;
    };

    /// BinaryExprAST - Expression class for a binary operator.
    class BinaryExprAST : public ExprAST {
        TOKENS Op;
        std::unique_ptr<ExprAST> LHS, RHS;

    public:
        BinaryExprAST(TOKENS Op, std::unique_ptr<ExprAST> LHS,
            std::unique_ptr<ExprAST> RHS)
            : Op(Op), LHS(std::move(LHS)), RHS(std::move(RHS)) {}

        Value* codegen() override;
    };

    /// CallExprAST - Expression class for function calls.
    class CallExprAST : public ExprAST {
        std::string Callee;
        std::vector<std::unique_ptr<ExprAST>> Args;

    public:
        CallExprAST(const std::string& Callee,
            std::vector<std::unique_ptr<ExprAST>> Args)
            : Callee(Callee), Args(std::move(Args)) {}

        Value* codegen() override;
    };

    /// PrototypeAST - This class represents the "prototype" for a function,
    /// which captures its name, and its argument names (thus implicitly the number
    /// of arguments the function takes).
    class PrototypeAST {
        std::string Name;
        std::vector<std::string> Args;

    public:
        PrototypeAST(const std::string& Name, std::vector<std::string> Args)
            : Name(Name), Args(std::move(Args)) {}

        Function* codegen();
        const std::string& getName() const { return Name; }
    };

    /// FunctionAST - This class represents a function definition itself.
    class FunctionAST {
        std::unique_ptr<PrototypeAST> Proto;
        std::unique_ptr<ExprAST> Body;

    public:
        FunctionAST(std::unique_ptr<PrototypeAST> Proto,
            std::unique_ptr<ExprAST> Body)
            : Proto(std::move(Proto)), Body(std::move(Body)) {}

        Function* codegen();
    };

} // end anonymous namespace

//===----------------------------------------------------------------------===//
// Parser
//===----------------------------------------------------------------------===//

/// CurTok/getNextToken - Provide a simple token buffer.  CurTok is the current
/// token the parser is looking at.  getNextToken reads another token from the
/// lexer and updates CurTok with its results.
//static int CurTok;
//static int getNextToken() { return CurTok = gettok(); }

/// BinopPrecedence - This holds the precedence for each binary operator that is
/// defined.
static int BiopPrecedence[TK_NUM_TOKENS];
static int PreopPrecedence[TK_NUM_TOKENS];
static int PostopPrecedence[TK_NUM_TOKENS];
enum class Associativity {
    Left    =   1,
    Right   =   2,
    NonAssoc=   3,
    BiopMask=   3,
    Pre     =   4,
    Post    =   8,
    NotOperator=16
};
static enum class Associativity OpAssoc[TK_NUM_TOKENS];

/// GetTokPrecedence - Get the precedence of the pending binary operator token.
static int GetTokPrecedence() {
    if (((int)OpAssoc[Tokenizer.cur_token().token_number] & (int)Associativity::BiopMask) == 0) return -1;
    return BiopPrecedence[Tokenizer.cur_token().token_number];
 }

/// LogError* - These are little helper functions for error handling.
std::unique_ptr<ExprAST> LogError(const char* Str) {
    fprintf(stderr, "Error: %s\n", Str);
    return nullptr;
}

std::unique_ptr<PrototypeAST> LogErrorP(const char* Str) {
    LogError(Str);
    return nullptr;
}

static std::unique_ptr<ExprAST> ParseExpression();

/// numberexpr ::= number
static std::unique_ptr<ExprAST> ParseNumberExpr() {
    auto Result = std::make_unique<NumberExprAST>(Tokenizer.cur_token().as_double());
    Tokenizer.tokenize(); // consume the number
    return std::move(Result);
}

/// parenexpr ::= '(' expression ')'
static std::unique_ptr<ExprAST> ParseParenExpr() {
    Tokenizer.tokenize(); // eat (.
    auto V = ParseExpression();
    if (!V)
        return nullptr;

    if (Tokenizer.cur_token().token_number!= TK_RP)
        return LogError("expected ')'");
    Tokenizer.tokenize(); // eat ).
    return V;
}

/// identifierexpr
///   ::= identifier
///   ::= identifier '(' expression* ')'
static std::unique_ptr<ExprAST> ParseIdentifierExpr() {
    char buf[100];
    Tokenizer.cur_token().get_text(buf, 100);
    std::string IdName = buf;

    Tokenizer.tokenize(); // eat identifier.

    if (Tokenizer.cur_token().token_number != TK_LP) // Simple variable ref.
        return std::make_unique<VariableExprAST>(IdName);

    // Call.
    Tokenizer.tokenize(); // eat (
    std::vector<std::unique_ptr<ExprAST>> Args;
    if (Tokenizer.cur_token().token_number != TK_RP) {
        while (true) {
            if (auto Arg = ParseExpression())
                Args.push_back(std::move(Arg));
            else
                return nullptr;

            if (Tokenizer.cur_token().token_number != TK_RP)
                break;

            if (Tokenizer.cur_token().token_number != TK_COMMA)
                return LogError("Expected ')' or ',' in argument list");
            Tokenizer.tokenize();
        }
    }

    // Eat the ')'.
    Tokenizer.tokenize();

    return std::make_unique<CallExprAST>(IdName, std::move(Args));
}

/// primary
///   ::= identifierexpr
///   ::= numberexpr
///   ::= parenexpr
static std::unique_ptr<ExprAST> ParsePrimary() {
    switch (Tokenizer.cur_token().token_number) {
    default:
        return LogError("unknown token when expecting an expression");
    case TK_IDENT:
        return ParseIdentifierExpr();
    case TK_INTEGER_CONST:
    case TK_REAL_CONST:
        return ParseNumberExpr();
    case TK_LP:
        return ParseParenExpr();
    }
}

/// binoprhs
///   ::= ('+' primary)*
static std::unique_ptr<ExprAST> ParseBinOpRHS(int ExprPrec,
    std::unique_ptr<ExprAST> LHS) {
    // If this is a binop, find its precedence.
    while (true) {
        int TokPrec = GetTokPrecedence();

        // If this is a binop that binds at least as tightly as the current binop,
        // consume it, otherwise we are done.
        if (TokPrec < ExprPrec)
            return LHS;

        // Okay, we know this is a binop.
        TOKENS BinOp =(TOKENS) Tokenizer.cur_token().token_number;
        Tokenizer.tokenize(); // eat binop

        // Parse the primary expression after the binary operator.
        auto RHS = ParsePrimary();
        if (!RHS)
            return nullptr;

        // If BinOp binds less tightly with RHS than the operator after RHS, let
        // the pending operator take RHS as its LHS.
        int NextPrec = GetTokPrecedence();
        if (TokPrec < NextPrec) {
            RHS = ParseBinOpRHS(TokPrec + 1, std::move(RHS));
            if (!RHS)
                return nullptr;
        }

        // Merge LHS/RHS.
        LHS =
            std::make_unique<BinaryExprAST>(BinOp, std::move(LHS), std::move(RHS));
    }
}

/// expression
///   ::= primary binoprhs
///
static std::unique_ptr<ExprAST> ParseExpression() {
    auto LHS = ParsePrimary();
    if (!LHS)
        return nullptr;

    return ParseBinOpRHS(0, std::move(LHS));
}

/// prototype
///   ::= id '(' id* ')'
static std::unique_ptr<PrototypeAST> ParsePrototype() {
    if (Tokenizer.cur_token().token_number != TK_IDENT)
        return LogErrorP("Expected function name in prototype");
    char buf[100];
    Tokenizer.cur_token().get_text(buf, 100);
    std::string FnName = buf;
    Tokenizer.tokenize();

    if (Tokenizer.cur_token().token_number != TK_LP)
        return LogErrorP("Expected '(' in prototype");

    std::vector<std::string> ArgNames;
    while (Tokenizer.tokenize() && Tokenizer.cur_token().token_number == TK_IDENT) {
        Tokenizer.cur_token().get_text(buf, 100);
        ArgNames.push_back(buf);
    }
        
    if (Tokenizer.cur_token().token_number != TK_RP)
        return LogErrorP("Expected ')' in prototype");

    // success.
    Tokenizer.tokenize(); // eat ')'.

    return std::make_unique<PrototypeAST>(FnName, std::move(ArgNames));
}

/// definition ::= 'def' prototype expression
static std::unique_ptr<FunctionAST> ParseDefinition() {
    Tokenizer.tokenize(); // eat def.
    auto Proto = ParsePrototype();
    if (!Proto)
        return nullptr;

    if (auto E = ParseExpression())
        return std::make_unique<FunctionAST>(std::move(Proto), std::move(E));
    return nullptr;
}

/// toplevelexpr ::= expression
static std::unique_ptr<FunctionAST> ParseTopLevelExpr() {
    if (auto E = ParseExpression()) {
        // Make an anonymous proto.
        auto Proto = std::make_unique<PrototypeAST>("__anon_expr",
            std::vector<std::string>());
        return std::make_unique<FunctionAST>(std::move(Proto), std::move(E));
    }
    return nullptr;
}

/// external ::= 'extern' prototype
static std::unique_ptr<PrototypeAST> ParseExtern() {
    Tokenizer.tokenize(); // eat extern.
    return ParsePrototype();
}

//===----------------------------------------------------------------------===//
// Code Generation
//===----------------------------------------------------------------------===//

static std::unique_ptr<LLVMContext> TheContext;
static std::unique_ptr<Module> TheModule;
static std::unique_ptr<IRBuilder<>> Builder;
static std::map<std::string, Value*> NamedValues;

Value* LogErrorV(const char* Str) {
    LogError(Str);
    return nullptr;
}

Value* NumberExprAST::codegen() {
    return ConstantFP::get(*TheContext, APFloat(Val));
}

Value* VariableExprAST::codegen() {
    // Look this variable up in the function.
    Value* V = NamedValues[Name];
    if (!V)
        return LogErrorV("Unknown variable name");
    return V;
}

Value* BinaryExprAST::codegen() {
    Value* L = LHS->codegen();
    Value* R = RHS->codegen();
    if (!L || !R)
        return nullptr;

    switch (Op) {
    case TK_PLUS:
        return Builder->CreateFAdd(L, R, "addtmp");
    case TK_MINUS:
        return Builder->CreateFSub(L, R, "subtmp");
    case TK_ASTERIX:
        return Builder->CreateFMul(L, R, "multmp");
    case TK_LT:
        L = Builder->CreateFCmpULT(L, R, "cmptmp");
        // Convert bool 0/1 to double 0.0 or 1.0
        return Builder->CreateUIToFP(L, Type::getDoubleTy(*TheContext), "booltmp");
    default:
        return LogErrorV("invalid binary operator");
    }
}

Value* CallExprAST::codegen() {
    // Look up the name in the global module table.
    Function* CalleeF = TheModule->getFunction(Callee);
    if (!CalleeF)
        return LogErrorV("Unknown function referenced");

    // If argument mismatch error.
    if (CalleeF->arg_size() != Args.size())
        return LogErrorV("Incorrect # arguments passed");

    std::vector<Value*> ArgsV;
    for (unsigned i = 0, e = Args.size(); i != e; ++i) {
        ArgsV.push_back(Args[i]->codegen());
        if (!ArgsV.back())
            return nullptr;
    }

    return Builder->CreateCall(CalleeF, ArgsV, "calltmp");
}

Function* PrototypeAST::codegen() {
    // Make the function type:  double(double,double) etc.
    std::vector<Type*> Doubles(Args.size(), Type::getDoubleTy(*TheContext));
    FunctionType* FT =
        FunctionType::get(Type::getDoubleTy(*TheContext), Doubles, false);

    Function* F =
        Function::Create(FT, Function::ExternalLinkage, Name, TheModule.get());

    // Set names for all arguments.
    unsigned Idx = 0;
    for (auto& Arg : F->args())
        Arg.setName(Args[Idx++]);

    return F;
}

Function* FunctionAST::codegen() {
    // First, check for an existing function from a previous 'extern' declaration.
    Function* TheFunction = TheModule->getFunction(Proto->getName());

    if (!TheFunction)
        TheFunction = Proto->codegen();

    if (!TheFunction)
        return nullptr;

    // Create a new basic block to start insertion into.
    BasicBlock* BB = BasicBlock::Create(*TheContext, "entry", TheFunction);
    Builder->SetInsertPoint(BB);

    // Record the function arguments in the NamedValues map.
    NamedValues.clear();
    for (auto& Arg : TheFunction->args())
        NamedValues[std::string(Arg.getName())] = &Arg;

    if (Value* RetVal = Body->codegen()) {
        // Finish off the function.
        Builder->CreateRet(RetVal);

        // Validate the generated code, checking for consistency.
        verifyFunction(*TheFunction);

        return TheFunction;
    }

    // Error reading body, remove function.
    TheFunction->eraseFromParent();
    return nullptr;
}

//===----------------------------------------------------------------------===//
// Top-Level parsing and JIT Driver
//===----------------------------------------------------------------------===//

static void InitializeModule() {
    // Open a new context and module.
    TheContext = std::make_unique<LLVMContext>();
    TheModule = std::make_unique<Module>("my cool jit", *TheContext);

    // Create a new builder for the module.
    Builder = std::make_unique<IRBuilder<>>(*TheContext);
}

static void HandleDefinition() {
    if (auto FnAST = ParseDefinition()) {
        if (auto* FnIR = FnAST->codegen()) {
            fprintf(stderr, "Read function definition:");
            FnIR->print(errs());
            fprintf(stderr, "\n");
        }
    }
    else {
        // Skip token for error recovery.
        Tokenizer.tokenize();
    }
}

static void HandleExtern() {
    if (auto ProtoAST = ParseExtern()) {
        if (auto* FnIR = ProtoAST->codegen()) {
            fprintf(stderr, "Read extern: ");
            FnIR->print(errs());
            fprintf(stderr, "\n");
        }
    }
    else {
        // Skip token for error recovery.
        Tokenizer.tokenize();
    }
}

static void HandleTopLevelExpression() {
    // Evaluate a top-level expression into an anonymous function.
    if (auto FnAST = ParseTopLevelExpr()) {
        if (auto* FnIR = FnAST->codegen()) {
            fprintf(stderr, "Read top-level expression:");
            FnIR->print(errs());
            fprintf(stderr, "\n");

            // Remove the anonymous expression.
            FnIR->eraseFromParent();
        }
    }
    else {
        // Skip token for error recovery.
        Tokenizer.tokenize();
    }
}

/// top ::= definition | external | expression | ';'
static void MainLoop() {
    while (true) {
        fprintf(stderr, "ready> ");
        switch (Tokenizer.cur_token().token_number) {
        case TK_EOF:
            return;
        case TK_SEMICOLON: // ignore top-level semicolons.
            Tokenizer.tokenize();
            break;
        case TK_FUNCTION://instead of define
            HandleDefinition();
            break;
        case TK_DECLARE://instead of extern
            HandleExtern();
            break;
        default:
            HandleTopLevelExpression();
            break;
        }
    }
}

//===----------------------------------------------------------------------===//
// Main driver code.
//===----------------------------------------------------------------------===//

int main() {
    for (int i = 0; i < TK_NUM_TOKENS; ++i) {
        BiopPrecedence[i]=0;
        PreopPrecedence[i]=0;
        PostopPrecedence[i]=0;
        OpAssoc[i]= Associativity::NotOperator;
    }
    // Install standard binary operators.
    // 1 is lowest precedence.
/*
* 
* 
1	()   []   ->   .   ::	Function call, scope, array/member access
2	   ~   -   +   *   &   sizeof   type cast   ++   --  	(most) unary operators, sizeof and type casts (right to left)
3	*   /   % MOD	Multiplication, division, modulo
4	+   -	Addition and subtraction
5	<<   >>	Bitwise shift left and right
6	&	Bitwise AND
6	^	Bitwise exclusive OR (XOR)
6	|	Bitwise inclusive (normal) OR
7	<   <=   >   >=	Comparisons: less-than and greater-than
7	==   !=	Comparisons: equal and not equal

8	and	
8	or
9   not
10	? :	Conditional expression (ternary)
11	=   +=   -=   *=   /=   %=   &=   |=   ^=   <<=   >>=	Assignment operators (right to left)
12	,
12  ;
    
    TK_AND,              and
        TK_NOT,          not
        TK_OR,           or 
        TK_TO,
        TK_BXOR,
        TK_MINUS,
        TK_ASTERIX,
        TK_AMPERSAND,
        TK_EXCLAMATION,
        TK_TILDE,
        TK_PLUSPLUS,
        TK_MINUSMINUS,
        TK_EQUAL,
        TK_PLUSEQ,
        TK_MINUSEQ,
        TK_ASTERIXEQ,
        TK_SLASHEQ,
        TK_MODEQ,
        TK_LTLTEQ,
        TK_GTGTEQ,
        TK_BANDEQ,
        TK_BOREQ,
        TK_BXOREQ,
        TK_CAROT,
        TK_LE,
        TK_GE,
        TK_LT,
        TK_GT,
        TK_EQEQ,
        TK_NOTEQ,
        TK_LTEQGT,
        TK_DOTDOT,
        TK_PERIOD,
        TK_PIPE,
        TK_GTGT,
        TK_LTLT,
        TK_PLUS,
        TK_SLASH,
        TK_QMARKEQ,
        TK_QMARK,
        TK_BSLASH,
        TK_HASH,
        TK_HASHPIPE,

static int BiopPrecedence[TK_NUM_TOKENS];
static int PreopPrecedence[TK_NUM_TOKENS];
static int PostopPrecedence[TK_NUM_TOKENS];
enum class Associativity {
    Left    =   1,
    Right   =   2,
    NonAssoc=   3,
    BiopMask=   3,
    Pre     =   4,
    Post    =   8,
    NotOperator=16
};
static enum class Associativity OpAssoc[TK_NUM_TOKENS];
        */
    BiopPrecedence[TK_LT] = 10;
    OpAssoc[TK_LT] = Associativity::Left;
    BiopPrecedence[TK_PLUS] = 20;
    OpAssoc[TK_PLUS] = Associativity::Left;
    BiopPrecedence[TK_MINUS] = 20;
    OpAssoc[TK_MINUS] = Associativity::Left;
    BiopPrecedence[TK_ASTERIX] = 40; // highest.
    OpAssoc[TK_ASTERIX] = Associativity::Left;

    // Prime the first token.
    fprintf(stderr, "ready> ");
    Tokenizer.tokenize();

    // Make the module, which holds all the code.
    InitializeModule();

    // Run the main "interpreter loop" now.
    MainLoop();

    // Print out all of the generated code.
    TheModule->print(errs(), nullptr);

    return 0;
}

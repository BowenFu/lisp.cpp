#include "lisp/evaluator.h"
#include "lisp/parser.h"
#include <numeric>
#include <fstream>
#include <filesystem>
namespace fs = std::filesystem;

#define DEBUG 0

auto consOp = [](std::vector<std::shared_ptr<Expr>> const& args)
{
    ASSERT(args.size() == 2);
    return ExprPtr{new Cons{args.at(0), args.at(1)}}; 
};

auto printOp = [](std::vector<std::shared_ptr<Expr>> const& args)
{
    for (auto const& e: args)
    {
        std::cout << e->toString();
    }
    std::cout << std::endl;
    return nil(); 
};

auto errorOp = [](std::vector<std::shared_ptr<Expr>> const& args)
{
    std::stringstream o;
    for (auto const& e: args)
    {
        o << e->toString() << " ";
    }
    o << std::endl;
    throw std::runtime_error{o.str()};
    return nil(); 
};

auto consPred = [](std::vector<std::shared_ptr<Expr>> const& args)
{
    ASSERT(args.size() == 1);
    if (dynamic_cast<Cons const*>(args.front().get()))
    {
        return true_();
    }
    return false_();
};

auto carOp = [](std::vector<std::shared_ptr<Expr>> const& args)
{
    ASSERT(args.size() == 1);
    auto cons_ = dynamic_cast<Cons*>(args.at(0).get());
    ASSERT(cons_);
    return cons_->car(); 
};

auto cdrOp = [](std::vector<std::shared_ptr<Expr>> const& args)
{
    ASSERT(args.size() == 1);
    auto cons_ = dynamic_cast<Cons&>(*args.at(0));
    return cons_.cdr(); 
};

auto isNullOp = [](std::vector<std::shared_ptr<Expr>> const& args)
{
    ASSERT(args.size() == 1);
    auto result = (args.at(0) == nil());
    return result ? true_() : false_(); 
};

auto isPairOp = [](std::vector<std::shared_ptr<Expr>> const& args)
{
    ASSERT(args.size() == 1);
    auto result = (dynamic_cast<Cons*>(args.at(0).get()) != nullptr);
    return result ? true_() : false_(); 
};

auto isEqOp = [](std::vector<std::shared_ptr<Expr>> const& args)
{
    ASSERT(args.size() == 2);
    return (args.at(0) == args.at(1) || (args.at(0)->equalTo(args.at(1)))) ? true_() : false_(); 
};

auto mulOp = [](std::vector<std::shared_ptr<Expr>> const& args)
{
    auto result = std::accumulate(args.begin(), args.end(), 1.0, [](auto p, std::shared_ptr<Expr> const& arg)
    {
        auto num = dynamic_cast<Number&>(*arg);
        return p * num.get();
    }
    );
    return std::shared_ptr<Expr>(new Number(result)); 
};

auto addOp = [](std::vector<std::shared_ptr<Expr>> const& args)
{
    auto result = std::accumulate(args.begin(), args.end(), 0.0, [](auto p, std::shared_ptr<Expr> const& arg)
    {
        auto num = dynamic_cast<Number&>(*arg);
        return p + num.get();
    }
    );
    return std::shared_ptr<Expr>(new Number(result)); 
};

auto divOp = [](std::vector<std::shared_ptr<Expr>> const& args)
{
    ASSERT(args.size() == 2);
    auto num1 = dynamic_cast<Number&>(*args.at(0));
    auto num2 = dynamic_cast<Number&>(*args.at(1));
    return std::shared_ptr<Expr>(new Number(num1.get() / num2.get())); 
};

auto ltOp = [](std::vector<std::shared_ptr<Expr>> const& args)
{
    ASSERT(args.size() == 2);
    auto num1 = dynamic_cast<Number&>(*args.at(0));
    auto num2 = dynamic_cast<Number&>(*args.at(1));
    return std::shared_ptr<Expr>(new Bool(num1.get() < num2.get())); 
};

auto setUpEnvironment()
{
    auto emptyEnv = std::make_shared<Env>();
    auto primitiveProcedureNames = std::vector<std::string>{};
    auto primitiveProcedureObjects = std::vector<ExprPtr>{};
    auto initialEnv = emptyEnv->extend(Params{std::make_pair(primitiveProcedureNames, false)}, primitiveProcedureObjects);

    initialEnv->defineVariable("cons", ExprPtr{new PrimitiveProcedure{consOp}});
    initialEnv->defineVariable("cons?", ExprPtr{new PrimitiveProcedure{consPred}});
    initialEnv->defineVariable("print", ExprPtr{new PrimitiveProcedure{printOp}});
    initialEnv->defineVariable("error", ExprPtr{new PrimitiveProcedure{errorOp}});
    initialEnv->defineVariable("car", ExprPtr{new PrimitiveProcedure{carOp}});
    initialEnv->defineVariable("cdr", ExprPtr{new PrimitiveProcedure{cdrOp}});
    initialEnv->defineVariable("null?", ExprPtr{new PrimitiveProcedure{isNullOp}});
    initialEnv->defineVariable("pair?", ExprPtr{new PrimitiveProcedure{isPairOp}});
    initialEnv->defineVariable("eq?", ExprPtr{new PrimitiveProcedure{isEqOp}});
    initialEnv->defineVariable("=", ExprPtr{new PrimitiveProcedure{isEqOp}});
    initialEnv->defineVariable("<", ExprPtr{new PrimitiveProcedure{ltOp}});
    initialEnv->defineVariable("+", ExprPtr{new PrimitiveProcedure{addOp}});
    initialEnv->defineVariable("*", ExprPtr{new PrimitiveProcedure{mulOp}});
    initialEnv->defineVariable("/", ExprPtr{new PrimitiveProcedure{divOp}});

    initialEnv->defineVariable("true", true_());
    initialEnv->defineVariable("false", false_());
    initialEnv->defineVariable("nil", nil());
    return initialEnv;
}

auto& globalEnvironment()
{
    static auto env = setUpEnvironment();
    return env;
}

constexpr auto inputPrompt = ";;; M-Eval input:";
constexpr auto outputPrompt = ";;; M-Eval value:";

void promptForInput(std::string const& string)
{
    std::cout << "\n\n" << string << std::endl;
}

void announceOutput(std::string const& string)
{
    std::cout << "\n" << string << std::endl;
}

auto eval(std::string const& input, std::shared_ptr<Env> const& env)
{
    Lexer lex(input);
    MetaParser p(lex);
    std::string result;
    auto macroEnv = env;
    do
    {
        auto me = p.sexpr();
#if DEBUG
        std::cout << "me ## " << me->toString() << std::endl; 
#endif // DEBUG
        auto ee = expandMacros(me, macroEnv);
#if DEBUG
        std::cout << "ee ## " << ee->toString() << std::endl; 
#endif // DEBUG
        auto e = parse(ee);
#if DEBUG
        std::cout << "e ## " << e->toString() << std::endl; 
#endif // DEBUG
        result = e->eval(env)->toString();
    } while (!p.eof());
    return result;
}

void preEval()
{
    auto const path1 = "core.lisp";
    auto const path2 = std::string("../../") + path1;
    auto const path = fs::exists(path1) ? path1 : path2;
    std::ifstream ifs(path);
    std::string content((std::istreambuf_iterator<char>(ifs)),
                        (std::istreambuf_iterator<char>()));
    auto output = eval(content, globalEnvironment());
    (void)output;
}

#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Winfinite-recursion"
#endif
void driverLoop()
{
    promptForInput(inputPrompt);
    std::string input;
    std::string allInput;
    while (getline(std::cin, input) && !input.empty())
    {
        allInput += input;
    }
    auto output = eval(allInput, globalEnvironment());
    announceOutput(outputPrompt);
    std::cout << output << std::endl;
    driverLoop();
}
#if defined(__clang__)
#pragma clang diagnostic pop
#endif

bool hasEnding(std::string const &fullString, std::string const &ending)
{
    if (fullString.length() >= ending.length())
    {
        return (0 == fullString.compare(fullString.length() - ending.length(), ending.length(), ending));
    }
    else
    {
        return false;
    }
}

int32_t main(int n, char** args)
{
    preEval();
    if (n == 1)
    {
        driverLoop();
    }
    else
    {
        ASSERT(n == 2);
        std::string input = args[1];
        if (hasEnding(input, ".lisp"))
        {
            std::ifstream ifs(input);
            std::string content((std::istreambuf_iterator<char>(ifs)),
                                (std::istreambuf_iterator<char>()));
            input = std::move(content);
        }
        auto output = eval(input, globalEnvironment());
        std::cout << output << std::endl;
    }
    return 0;
}

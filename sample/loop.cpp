#include "lisp/parser.h"

auto consOp = [](std::vector<std::shared_ptr<Expr>> const& args)
{
    ASSERT(args.size() == 2);
    return ExprPtr{new Cons{args.at(0), args.at(1)}}; 
};

auto carOp = [](std::vector<std::shared_ptr<Expr>> const& args)
{
    ASSERT(args.size() == 1);
    auto cons_ = dynamic_cast<Cons&>(*args.at(0));
    return cons_.car(); 
};

auto cdrOp = [](std::vector<std::shared_ptr<Expr>> const& args)
{
    ASSERT(args.size() == 1);
    auto cons_ = dynamic_cast<Cons&>(*args.at(0));
    return cons_.cdr(); 
};

auto listOp = [](std::vector<std::shared_ptr<Expr>> const& args)
{
    auto result = nil();
    for (auto i = args.rbegin(); i != args.rend(); ++i)
    {
        result = ExprPtr{new Cons{*i, result}};
    }
    return result;
};

auto setUpEnvironment()
{
    auto emptyEnv = std::make_shared<Env>();
    auto primitiveProcedureNames = std::vector<std::string>{};
    auto primitiveProcedureObjects = std::vector<ExprPtr>{};
    auto initialEnv = emptyEnv->extend(primitiveProcedureNames, primitiveProcedureObjects);

    initialEnv->defineVariable("cons", ExprPtr{new PrimitiveProcedure{consOp}});
    initialEnv->defineVariable("car", ExprPtr{new PrimitiveProcedure{carOp}});
    initialEnv->defineVariable("cdr", ExprPtr{new PrimitiveProcedure{cdrOp}});
    initialEnv->defineVariable("list", ExprPtr{new PrimitiveProcedure{listOp}});

    initialEnv->defineVariable("true", true_());
    initialEnv->defineVariable("false", false_());
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
    Parser p(lex);
    std::string result;
    do
    {
        auto e = p.sexpr();
        result = e->eval(env)->toString();
    } while (!p.eof());
    return result;
}

#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Winfinite-recursion"
#endif
void driverLoop()
{
    promptForInput(inputPrompt);
    std::string input;
    getline(std::cin, input);
    auto output = eval(input, globalEnvironment());
    announceOutput(outputPrompt);
    std::cout << output << std::endl;
    driverLoop();
}
#if defined(__clang__)
#pragma clang diagnostic pop
#endif

int32_t main()
{
    driverLoop();
    return 0;
}
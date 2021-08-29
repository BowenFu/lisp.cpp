#include "lisp/evaluator.h"
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
    auto initialEnv = emptyEnv->extend(primitiveProcedureNames, primitiveProcedureObjects);

    initialEnv->defineVariable("cons", ExprPtr{new PrimitiveProcedure{consOp}});
    initialEnv->defineVariable("car", ExprPtr{new PrimitiveProcedure{carOp}});
    initialEnv->defineVariable("cdr", ExprPtr{new PrimitiveProcedure{cdrOp}});
    initialEnv->defineVariable("list", ExprPtr{new PrimitiveProcedure{listOp}});
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
    initialEnv->defineVariable("#t", true_());
    initialEnv->defineVariable("#f", false_());
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
    do
    {
        auto me = p.sexpr();
        std::cout << "me\t<" << me->toString() << ">" << std::endl;
        auto e = parse(me);
        std::cout << "e\t<" << e->toString() << ">" << std::endl;
        result = e->eval(env)->toString();
        std::cout << "result\t<" << result << ">" << std::endl;
    } while (!p.eof());
    return result;
}

void preEval()
{
    auto defAtom = "(define atom?"
                        "(lambda (x) "
                        "(and (not (pair? x)) (not (null? x)))))";
    eval(defAtom ,globalEnvironment());
    auto defNot = "(define not"
                        "(lambda (x) "
                        "(if x false true)))";
    eval(defNot ,globalEnvironment());
    auto defSub = "(define -"
                        "(lambda (x y) "
                        "(+ x (* -1 y))))";
    eval(defSub ,globalEnvironment());
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
        auto input = args[1];
        auto output = eval(input, globalEnvironment());
        std::cout << output << std::endl;
    }
    return 0;
}

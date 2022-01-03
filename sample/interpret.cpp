#include "lisp/evaluator.h"
#include "lisp/parser.h"
#include <numeric>
#include <fstream>
#include <filesystem>
#include <cmath>
namespace fs = std::filesystem;

#define DEBUG 0

std::shared_ptr<Env> setUpEnvironment();

auto& globalEnvironment()
{
    static auto env = setUpEnvironment();
    return env;
}

auto& globalMacroEnvironment()
{
    static auto macroEnv = globalEnvironment()->extend(Params{}, {});
    return macroEnv;
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

auto eval(std::string const& input, std::shared_ptr<Env> const& env, std::shared_ptr<Env> const& macroEnv)
{
    Lexer lex(input);
    MetaParser p(lex);
    std::string result;
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
    auto output = eval(content, globalEnvironment(), globalMacroEnvironment());
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
    auto output = eval(allInput, globalEnvironment(), globalMacroEnvironment());
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
        auto output = eval(input, globalEnvironment(), globalMacroEnvironment());
        std::cout << output << std::endl;
    }
    return 0;
}

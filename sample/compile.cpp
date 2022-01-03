#include "lisp/evaluator.h"
#include "lisp/parser.h"
#include "lisp/compiler.h"
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

void promptForInput(std::string const& string)
{
    std::cout << "\n\n" << string << std::endl;
}

void announceOutput(std::string const& string)
{
    std::cout << "\n" << string << std::endl;
}

auto compile(Compiler& c, std::string const& input)
{
    Lexer lex(input);
    MetaParser p(lex);
    do
    {
        auto me = p.sexpr();
#if DEBUG
        std::cout << "me ## " << me->toString() << std::endl;
#endif // DEBUG
        auto ee = expandMacros(me, globalMacroEnvironment());
#if DEBUG
        std::cout << "ee ## " << ee->toString() << std::endl;
#endif // DEBUG
        auto e = parse(ee);
#if DEBUG
        std::cout << "e ## " << e->toString() << std::endl;
#endif // DEBUG
        e->eval(globalEnvironment());
        c.compile(e);
    } while (!p.eof());
    return c.code();
}

void preCompile(Compiler& c)
{
    auto const path1 = "core.lisp";
    auto const path2 = std::string("../../") + path1;
    auto const path = fs::exists(path1) ? path1 : path2;
    std::ifstream ifs(path);
    std::string content((std::istreambuf_iterator<char>(ifs)),
                        (std::istreambuf_iterator<char>()));
    auto code = compile(c, content);
    (void)code;
}

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
    Compiler c{};
    preCompile(c);
    ASSERT(n == 2);
    std::string input = args[1];
    if (hasEnding(input, ".lisp"))
    {
        std::ifstream ifs(input);
        std::string content((std::istreambuf_iterator<char>(ifs)),
                            (std::istreambuf_iterator<char>()));
        input = std::move(content);
    }
    auto code = compile(c, input);
    code.instructions.push_back(vm::kPRINT);
    vm::VM vm{code};
    vm.run();
    return 0;
}

//
// Created by Mpho Mbotho on 2021-06-05.
//

#include "suil/utils/args.hpp"
#include "suil/utils/exception.hpp"

#include <iostream>
#include <sstream>

namespace suil::args {

    Command::Command(std::string name, std::string desc, bool help)
        : mName{std::move(name)},
          mDesc{std::move(desc)}
    {
        if (help) {
            Ego(Arg{"help", "Show this command's help", 'h', true, false});
        }
    }

    Command& Command::operator()(Handler handler)
    {
        Ego.mHandler = std::move(handler);
        return Ego;
    }

    Command& Command::operator()(Arg&& arg)
    {
        if (arg.Lf.empty()) {
            throw InvalidArguments("Command line argument missing long format (--arg) option");
        }

        for (auto& a: mArgs) {
            if (a.check(arg.Sf, arg.Lf)) {
                throw InvalidArguments("Command line '", a.Lf, "|", arg.Sf, "' duplicated");
            }
        }

        mRequired = mRequired || arg.Required;
        mTakesArgs = mTakesArgs || !arg.Option;
        auto len = arg.Option? arg.Lf.size(): arg.Lf.size() + 5;
        mLongest = std::max(mLongest, len);
        arg.Global = false;
        mArgs.emplace_back(arg);
        return Ego;
    }

    void Command::showHelp(const std::string_view& app, std::ostream& os, bool isHelp) const
    {
        if (isHelp) {
            os << mDesc << "\n\n";
        }
        os << "Usage:\n"
           << "  " << app << " " << mName;
        if (!mArgs.empty()) {
            os << " [flags...]\n"
               << "\n"
               << "Flags:\n";

            for (auto& arg: mArgs) {
                if (arg.Global) {
                    continue;
                }

                size_t remaining{mLongest+1};
                os << "    ";
                if (arg.Sf != NOSF) {
                    os << '-' << arg.Sf << ", ";
                } else {
                    os << "    ";
                }

                os << "--" << arg.Lf;
                os << (!arg.Prompt.empty()? '?': (arg.Required? '*': ' '));
                os << (arg.Option? (mTakesArgs? "     ": ""): " arg0");

                remaining -= arg.Lf.size();
                std::fill_n(std::ostream_iterator<char>(os), remaining, ' ');
                os << arg.Help << "\n";
            }
        }
        else {
            os << "\n";
        }
    }

   const Arg& Command::check(const std::string_view& lf, char sf) const
    {
        const Arg *arg{nullptr};
        if (check(&arg, lf, sf)) {
            return *arg;
        }
        else {
            if (!lf.empty()) {
                throw InvalidArguments("error: command argument '--", lf, "' is not recognized");
            }
            else {
                throw InvalidArguments("error: command argument '-", sf, "' is not recognized");
            }
        }
    }

    bool Command::check(const Arg** found, const std::string_view& lf, char sf) const
    {
        for (auto& arg: mArgs) {
            if (arg.check(sf, lf)) {
                *found = &arg;
                return true;
            }
        }
        return false;
    }

    bool Command::parse(int argc, char** argv, bool debug)
    {
        int pos{0};
        std::string zArg{};
        bool isHelp{false};

        while (pos < argc) {
            auto cArg = argv[pos];
            auto cVal = strchr(cArg, '=');
            if (cVal != nullptr) {
                // argument is --arg=val
                *cVal = '\0';
            }
            if (cArg[0] != '-') {
                // positional argument
                Ego.mPositionals.emplace_back(cArg);
                pos++;
                continue;
            }

            int dashes = isValid(cArg);
            if (dashes == 0) {
                throw InvalidArguments("error: Unsupported argument syntax '", cArg, "'");
            }

            if (dashes == 2) {
                // argument is parsed in Lf
                auto& arg = Ego.check(&cArg[2], NOSF);
                if (!arg.Prompt.empty()) {
                    throw InvalidArguments("error: command argument '", arg.Lf, "' is supported through prompt");
                }

                if (arg.Sf == 'h') {
                    isHelp = true;
                    break;
                }

                if (mParsed.find(arg.Lf) != mParsed.end()) {
                    throw InvalidArguments("error: command argument '", arg.Lf, "' was already passed");
                }

                std::string val{"1"};
                if (!arg.Option) {
                    if (cVal == nullptr) {
                        pos++;
                        if (pos >= argc) {
                            throw InvalidArguments("error: command argument '", arg.Lf, "' expects a value");
                        }
                        cVal = argv[pos];
                    }
                    val = std::string{cVal};
                }
                else if (cVal != nullptr) {
                    throw InvalidArguments("error: command argument '", arg.Lf, "' does not take a value");
                }
                mParsed.emplace(arg.Lf, std::move(val));
            }
            else {
                size_t nOpts = strlen(cArg);
                size_t oPos{1};
                while (oPos < nOpts) {
                    auto& arg = Ego.check({}, cArg[oPos++]);
                    if (arg.Sf == 'h') {
                        isHelp = true;
                        break;
                    }

                    std::string val{"1"};
                    if (!arg.Option) {
                        if (oPos < nOpts) {
                            throw InvalidArguments("error: command argument '", arg.Lf, "' expects a value");
                        }

                        if (cVal == nullptr) {
                            pos++;
                            if (pos >= argc) {
                                throw InvalidArguments("error: command argument '", arg.Lf, "' expects a value");
                            }
                            cVal = argv[pos];
                        }

                        val = std::string{cVal};
                    }
                    else if (cVal != nullptr) {
                        throw InvalidArguments("error: command argument '", arg.Lf,
                                               "' assigned a value but it's an option");
                    }
                    mParsed.emplace(arg.Lf, std::move(val));
                }

                if (isHelp) {
                    break;
                }
            }
            pos++;
        }

        if (!isHelp) {
            std::stringstream ss;
            ss << "error: missing required arguments:";
            bool missing{false};
            for (auto& arg: mArgs) {
                if (!arg.Required or (mParsed.find(arg.Lf) != mParsed.end())) {
                    continue;
                }
                if (!arg.Prompt.empty()) {
                    // Command argument is interactive, request value from console
                    requestValue(arg);
                }
                else {
                    ss << (missing? ", '" : " '") << arg.Lf << '\'';
                    missing = true;
                }
            }

            if (missing) {
                throw InvalidArguments(ss.str());
            }
        }

        if (debug) {
            // dump all arguments to console
            for (auto& [key, value]: mParsed) {
                std::cout << "--" << key << " = " << value << "\n";
            }
        }

        return isHelp;
    }

    void Command::requestValue(Arg& arg)
    {
        Status<std::string> status;
        if (arg.Hidden) {
            status = readPasswd(arg.Prompt);
        }
        else {
            status = readParam(arg.Prompt, arg.Default);
        }
        if (status.error and arg.Required) {
            throw InvalidArguments("error: required interactive argument '", arg.Lf, "' not provided");
        }

        mParsed.emplace(arg.Lf, std::move(status.result));
    }

    std::string_view Command::at(int index, const std::string_view& errMsg) const
    {
        if (mPositionals.size() <= index) {
            if (!errMsg.empty()) {
                throw InvalidArguments(errMsg);
            }
            return {};
        }
        return mPositionals[index];
    }

    std::string_view Command::operator[](const std::string_view& lf) const
    {
        auto it = mParsed.find(lf);
        if (it != mParsed.end()) {
            return it->second;
        }
        return {};
    }

    int Command::isValid(const std::string_view& flag)
    {
        if (flag[0] != '-' || (flag.size() == 1)) {
            return 0;
        }

        if (flag[1] != '-') {
            return (flag[1] != '\0' and ::isalpha(flag[1]))? 1: 0;
        }

        if (flag.size() == 2) {
            return 0;
        }

        return (flag[2] != '\0' and isalpha(flag[2]))? 2: 0;
    }

    Parser::Parser(std::string app, std::string version, std::string descript)
        : mAppName{std::move(app)},
          mAppVersion{std::move(version)},
          mDescription{std::move(descript)}
    {
        // Add a global flag
        Ego(Arg{"help", "Show help for the active command for application", 'h'});

        // add version command
        Command ver{"version", "Show the application version"};
        ver([&](Command& cmd) {
            std::cout << mAppName << " v" << mAppVersion << "\n";
            if (!Ego.mDescription.empty()) {
                std::cout << Ego.mDescription << "\n";
            }
        });
        ver.mInternal = true;

        // add help command
        Command help{"help", "Display this application help"};
        help([&](Command& cmd) {
            // show application help
            Ego.showHelp();
        });
        help.mInternal = true;

        Ego.add(std::move(ver), std::move(help));
    }

    Command* Parser::find(const std::string_view& name)
    {
        Command *cmd{nullptr};
        for (auto& c: mCommands) {
            if (c.mName == name) {
                cmd = &c;
                break;
            }
        }

        return cmd;
    }

    Arg* Parser::findArg(const std::string_view& name, char sf)
    {
        for (auto& a: mGlobals) {
            if (a.Lf == name || (sf != NOSF and sf == a.Sf)) {
                return &a;
            }
        }
        return nullptr;
    }

    Parser& Parser::add(Arg &&arg)
    {
        if (Ego.findArg(arg.Lf) == nullptr) {
            arg.Global = true;
            size_t len = (arg.Option? arg.Lf.size() : arg.Lf.size() + 5);
            mTakesArgs = mTakesArgs || !arg.Option;
            mLongestFlag = std::max(mLongestFlag, len);
            mGlobals.push_back(std::move(arg));
        }
        else {
            throw InvalidArguments(
                    "duplicate global argument '", arg.Lf, "' already registered");
        }
        return Ego;
    }

    Parser& Parser::add(Command &&cmd)
    {
        if (Ego.find(cmd.mName) == nullptr) {
            // add copies of global arguments
            for (auto& ga: mGlobals) {
                const Arg* _;
                if (!cmd.check(&_, ga.Lf, ga.Sf)) {
                    cmd.mHasGlobalFlags = true;
                    cmd.mArgs.push_back(ga);
                }
            }

            // accommodate interactive
            mInter = mInter || cmd.mInteractive;
            size_t len = Ego.mInter? (cmd.mName.size()+14) : cmd.mName.size();
            mLongestCmd = std::max(mLongestCmd, len);
            // add a new command
            mCommands.emplace_back(std::move(cmd));
        }
        else {
            throw InvalidArguments(
                    "command with name '", cmd.mName, " already registered");
        }
        return Ego;
    }

    void Parser::getHelp(std::ostream& os, const char *prefix)
    {
        if (prefix != nullptr) {
            os << prefix << '\n';
        }
        else {
            if (!mDescription.empty()) {
                // show description
                os << mDescription << '\n';
            }
            else {
                // make up description
                os << mAppName;
                if (!mAppVersion.empty()) {
                    os << " v" << mAppVersion;
                }
                os << '\n';
            }
        }
        os << '\n'
           << "Usage:"
           << "    " << mAppName << " [command ...]\n"
           << '\n';

        // append commands help
        if (!mCommands.empty()) {
            os << "Available Commands:\n";
            for (auto& cmd: mCommands) {
                size_t remaining{mLongestCmd - cmd.mName.size()};
                os << "  " << cmd.mName << ' ';
                if (cmd.mInteractive) {
                    os << "(interactive) ";
                    remaining -= 14;
                }
                if (remaining > 0) {
                    std::fill_n(std::ostream_iterator<char>(os), remaining, ' ');
                }
                os << cmd.mDesc << '\n';
            }
        }

        // append global arguments
        if (!mGlobals.empty()) {
            os << "Flags:\n";
            for (auto& ga: mGlobals) {
                size_t remaining{mLongestFlag - ga.Lf.size()};
                os << "    ";
                if (ga.Sf != NOSF) {
                    os << '-' << ga.Sf;
                    if (!ga.Lf.empty())
                        os << ", ";
                } else {
                    os << "    ";
                }

                os << "--" << ga.Lf;
                os << (!ga.Prompt.empty()? '?': (ga.Required? '*': ' '));
                os << (ga.Option? (mTakesArgs? "     ": ""): " arg0");

                std::fill_n(std::ostream_iterator<char>(os), remaining, ' ');
                os << " " << ga.Help << "\n";
            }
        }
        os << '\n'
           << "Use \"" << mAppName
           << "\" [command] --help for more information about a command"
           << "\n\n";
    }

    void Parser::showHelp(const char *prefix) {
        getHelp(std::cout, prefix);
    }

    void Parser::getCommandHelp(std::ostream& os, Command& cmd, bool isHelp)
    {
        cmd.showHelp(mAppName, os, isHelp);
        // append global arguments
        if (cmd.mHasGlobalFlags) {
            os << "\nGlobal Flags:\n";
            for (auto& ga: cmd.mArgs) {
                if (!ga.Global) {
                    continue;
                }

                size_t remaining{mLongestFlag - ga.Lf.size()};
                os << "    ";
                if (ga.Sf != NOSF) {
                    os << '-' << ga.Sf;
                    if (!ga.Lf.empty())
                        os << ", ";
                } else {
                    os << "    ";
                }

                os << "--" <<  ga.Lf;
                os << (!ga.Prompt.empty()? '?': (ga.Required? '*': ' '));
                os << (ga.Option? (mTakesArgs? "     ": ""): " arg0");

                std::fill_n(std::ostream_iterator<char>(os), remaining, ' ');
                os << " " << ga.Help << "\n";
            }
        }
        os << "\n";
    }

    void Parser::parse(int argc, char **argv)
    {
        if (argc <= 1) {
            // show application help
            Ego.showHelp();
            exit(EXIT_FAILURE);
        }

        if (argv[1][0] == '-') {
            int tr = Command::isValid(argv[1]);
            if (!tr) {
                fprintf(stderr, "error: bad flag syntax: %s\n", argv[1]);
                exit(EXIT_FAILURE);
            }
            fprintf(stderr, "error: free floating flags are not supported\n");
            exit(EXIT_FAILURE);
        }

        std::string_view cmdstr{argv[1]};
        Command *cmd = Ego.find(cmdstr);
        if (cmd == nullptr) {
            fprintf(stderr, "error: unknown command \"%s\" for \"%.*s\"\n",
                    argv[1], int(mAppName.size()), mAppName.data());
            exit(EXIT_FAILURE);
        }

        bool showhelp[2] ={false, true};
        try {
            // parse command line (appname command)
            int nargs{argc-2};
            char  **args = nargs? &argv[2] : &argv[1];
            showhelp[0] = cmd->parse(argc-2, args);
        }
        catch (Exception& ser) {
            showhelp[0] = true;
            showhelp[1] = false;
            std::cerr << ser.what() << "\n";
        }

        if (showhelp[0]) {
            Ego.getCommandHelp((showhelp[1]? std::cout : std::cerr), *cmd, showhelp[1]);
            exit(showhelp[1]? EXIT_SUCCESS : EXIT_FAILURE);
        }

        // save passed command
        mParsed = cmd;

        if (cmd->mInternal) {
            // execute internal commands and exit
            Ego.handle();
            exit(EXIT_SUCCESS);
        }
    }

    void Parser::handle() {
        if (mParsed && mParsed->mHandler) {
            mParsed->mHandler(*mParsed);
            return;
        }
        throw UnsupportedOperation("parser::parse should be "
                                "invoked before invoking handle");
    }

    Status<std::string> readParam(const std::string_view& display, const std::string& def)
    {
        write(STDIN_FILENO, display.data(), display.size());
        sync();

        std::string result;
        if (std::getline(std::cin, result)) {
            return Ok(std::move(result));
        }
        else if (!def.empty()) {
            return Ok(def);
        }

        return {-1};
    }

    Status<std::string> readPasswd(const std::string_view& display)
    {
        auto pass = getpass(display.data());
        return Ok(std::string{pass});
    }
}

#ifdef SUIL_UNITTEST
#include <catch2/catch.hpp>
#define ARGV_SIZE(argv) sizeof(argv)/sizeof(const char *)

using suil::args::Arg;
using suil::args::Command;
using suil::args::Parser;
using suil::args::Arguments;

SCENARIO("Using args::Parser")
{
    WHEN("Building a command") {
        Command cmd("one", "Simple one command", false);
        REQUIRE(cmd.mName == "one");
        REQUIRE(cmd.mDesc == "Simple one command");
        cmd({.Lf = "help", .Help = "Show shelp", .Sf = 'h'});
        REQUIRE(cmd.mArgs.size() == 1);
    }

    WHEN("Showing command help") {
        Command cmd("one", "Simple one command", false);
        cmd({.Lf = "help", .Help = "Show shelp", .Sf = 'h'})
           ({.Lf = "one", .Help = "First argument", .Sf = 'o', .Option = false})
           ({.Lf = "two", .Help = "Second argument", .Required = true})
           ({.Lf = "three", .Help = "Third argument", .Required = true, .Global = true})
           ({.Lf =  "four", .Help = "Fourth argument", .Required = true, .Prompt = "Enter 4:"})
           ;

        std::stringstream help;
        cmd.showHelp("demo", help, false);
        std::string expected =
                "Usage:\n"
                "  demo one [flags...]\n"
                "\n"
                "Flags:\n"
                "    -h, --help           Show shelp\n"
                "    -o, --one  arg0      First argument\n"
                "        --two*           Second argument\n"
                "        --three*         Third argument\n"
                "        --four?          Fourth argument\n"
                ;
        REQUIRE(help.str() == expected);
    }

    WHEN("Using Command::parse") {
        Command cmd("doit", "Simple one command", false);
        cmd({.Lf = "help", .Help = "Show shelp", .Sf = 'h'})
                ({.Lf = "one", .Help = "First argument", .Sf = 'o', .Option = false})
                ({.Lf = "two", .Help = "Second argument", .Required = true})
                ({.Lf = "three", .Help = "Third argument", .Sf = 't', .Required = true, .Global = true});

        {
            Arguments args{"--help"};
            REQUIRE(cmd.parse(args.Argc, args.Argv, false));
            cmd.mParsed.clear();
        }

        {
            Arguments args{"--one", "one", "--two", "-t"};
            REQUIRE_FALSE(cmd.parse(args, false));
            cmd.mParsed.clear();
        }

        {
            Arguments args{"-o", "1", "--two", "-t"};
            REQUIRE_FALSE(cmd.parse(args, false));
            cmd.mParsed.clear();
        }

        {
            Arguments args{"--one", "--two", "-t"};
            REQUIRE_THROWS(cmd.parse(args, false));
            cmd.mParsed.clear();
        }

        {
            Arguments args{"--one", "1" "--two"}; // missing required option
            REQUIRE_THROWS(cmd.parse(args, false));
            cmd.mParsed.clear();
        }

        {
            Arguments args{"--", "1" "--two"}; // invalid syntax
            REQUIRE_THROWS(cmd.parse(args, false));
            cmd.mParsed.clear();
        }
    }

    WHEN("Building a Parser") {
        Parser parser("demo", "0.1.0", "Test application");
        parser(Command{"add", "Add two numbers", false});
        {
            REQUIRE_THROWS(parser(Command{"add", "Add two numbers"})); // Duplicate command names not allowed
        }

        parser(Arg{"verbose", "Show all commands output", 'v'});
        {
            REQUIRE_THROWS(parser(Arg{"verbose", "Show all commands output", 'v'}));
        }
    }

    WHEN("Showing parser help") {
        Parser parser("demo", "0.1.0", "Test application");
        parser(
            Arg{"verbose", "Show verbose output", 'v', true},
            Command("add", "Add some numbers", false)
              ({.Lf ="start", .Help = "first addend", .Sf = 'a', .Required = true})
              ({.Lf = "next", .Help = "second addend", .Sf = 'b', .Required = true})(),
            Command("sub", "Subtract some numbers from the minuend", false)
             ({"minuend", "The number to subtract from", 'm'})()
        );

        {
            std::stringstream help;
            parser.getHelp(help);
            std::string expected = "Test application\n"
                                    "\n"
                                    "Usage:    demo [command ...]\n"
                                    "\n"
                                    "Available Commands:\n"
                                    "  version Show the application version\n"
                                    "  help    Display this application help\n"
                                    "  add     Add some numbers\n"
                                    "  sub     Subtract some numbers from the minuend\n"
                                    "Flags:\n"
                                    "    -h, --help     Show help for the active command for application\n"
                                    "    -v, --verbose  Show verbose output\n"
                                    "\n"
                                    "Use \"demo\" [command] --help for more information about a command\n"
                                    "\n";
            REQUIRE(help.str() == expected);
        }
        {
            auto& cmd = parser.mCommands[2];
            std::stringstream out;
            parser.getCommandHelp(out, cmd, false);
            std::string expected =  "Usage:\n"
                                    "  demo add [flags...]\n"
                                    "\n"
                                    "Flags:\n"
                                    "    -a, --start* first addend\n"
                                    "    -b, --next*  second addend\n"
                                    "\n"
                                    "Global Flags:\n"
                                    "    -h, --help     Show help for the active command for application\n"
                                    "    -v, --verbose  Show verbose output\n"
                                    "\n";
            REQUIRE(out.str() == expected);
        }
    }

    WHEN("Parsing command line arguments") {
        Parser parser("demo", "0.1.0", "Test application");
        parser(
                Command("add", "Add some numbers", false)
                        ({"start", "first addend", 'a', false, true})
                        ({"next", "second addend", 'b', false, true})(),
                Command("sub", "Subtract some numbers from the minuend", false)
                        ({"minuend", "The number to subtract from", 'm', false, true})(),
                Arg{"verbose", "Show verbose output", 'v', true}
        );

        {
            Arguments arguments{"demo", "add", "--start", "100", "--next", "10", "13"};
            REQUIRE_NOTHROW(parser.parse(arguments));
            REQUIRE(parser.mParsed != nullptr);

            auto& cmd = *parser.mParsed;
            REQUIRE(cmd.value("start", int(0)) == 100);
            REQUIRE(cmd.value("next", int(0)) == 10);
            REQUIRE(cmd.at<int>(0) == 13);
        }
    }
}

#endif
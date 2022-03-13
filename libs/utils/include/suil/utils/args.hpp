//
// Created by Mpho Mbotho on 2021-06-05.
//

#pragma once

#include <suil/utils/exception.hpp>
#include <string>

namespace suil::args {

    constexpr const char NOSF{'\0'};

    struct Arg {
        std::string      Lf{};
        std::string      Help{};
        char             Sf{NOSF};
        bool             Option{true};
        bool             Required{false};
        bool             Global{false};
        std::string      Prompt{};
        bool             Hidden{false};
        std::string      Default{};

    private:
        friend struct Command;

        inline bool check(const std::string_view& arg) const {
            return arg == Lf;
        }

        inline bool check(char c) const { return c == Sf; }

        inline bool check(char c, const std::string_view& l) {
            return (c != NOSF && check(c)) || check(l);
        }
    };

    struct Arguments {
        template <typename... A>
        Arguments(A... argv)
                : Argv{new char*[sizeof...(A)]},
                  Argc{sizeof...(A)}
        {
            add(argv...);
        }

        Arguments(const char** argv, int argc)
            : Argv{new char*[argc]},
              Argc{argc}
        {
            for (int i = 0; i < argc; i++) {
                Argv[i] = strdup(argv[i]);
            }
        }

        Arguments(const Arguments&) = delete;
        Arguments& operator=(const Arguments&) = delete;
        Arguments(Arguments&&) = delete;
        Arguments& operator=(Arguments&&) = delete;

        ~Arguments() {
            if (Argv != nullptr) {
                for (int i = 0; i < Argc; i++) {
                    free(Argv[i]);
                }
                delete[] Argv;
                Argv = nullptr;
            }
        }
        char **Argv{nullptr};
        int Argc{0};

    private:
        template <typename... A>
        void add(const char *a, A... argv) {
            Argv[Argc - sizeof...(A) - 1] = strdup(a);
            if constexpr(sizeof...(A) > 0) {
                add(argv...);
            }
        }
    };

    struct Command {
        using Handler = std::function<void(Command&)>;

        Command(std::string name, std::string desc = {}, bool help = false);

        Command& operator()(Arg&& arg);
        Command& operator()(Handler handler);
        Command  operator()() { return std::move(Ego); }
        void showHelp(const std::string_view& app, std::ostream& os, bool isHelp = false) const;
        bool parse(int argc, char *argv[], bool debug = false);
        bool parse(Arguments& arguments, bool debug = false) {
            return parse(arguments.Argc, arguments.Argv, debug);
        }

        std::string_view at(int index, const std::string_view& errMsg = {});

        std::string_view operator[](char sf) {
            auto& arg = Ego.check({}, sf);
            return Ego[arg.Lf];
        }

        std::string_view operator[](const std::string_view& lf);

        std::string_view operator[](int index) {
            return Ego.at(index);
        }

        template <typename V>
        V at(int index, const std::string_view& errMsg = {}) {
            auto value = Ego.at(index, errMsg);
            V tmp{};
            setValue(tmp, value);
            return tmp;
        }

        template <typename V>
        V value(std::string_view lf, V def)
        {
            Arg* _;
            if (!check(_, lf, NOSF)) {
                throw InvalidArguments("passed parameter '", lf, "' is not an argument");
            }

            auto zStr = Ego[lf];
            if constexpr(std::is_same_v<V, std::string>) {
                if (!zStr.empty()) {
                    return zStr;
                }
                else {
                    return std::move(def);
                }
            }
            else {
                V tmp{std::move(def)};
                if (!zStr.empty()) {
                    setValue(tmp, zStr);
                }
                return std::move(tmp);
            }
        }

    private suil_ut:
        friend class Parser;

        inline void setValue(std::string& out, const std::string_view& from) {
            out = std::string{from};
        }

        template <typename V>
        inline void setValue(V& out, const std::string_view& from) {
            suil::cast(suil::trim(from), out);
        }

        template <typename V>
        inline void setValue(std::vector<V>& out, const std::string_view& from) {
            auto parts = suil::parts(out, ",");
            for (auto& part: parts) {
                V val{};
                setValue(val, part);
                out.push_back(std::move(val));
            }
        }

        void requestValue(Arg& arg);
        Arg& check(const std::string_view& lf, char sf);
        bool check(Arg*& found, const std::string_view& lf, char sf);

        static int isValid(const std::string_view&  flag);

        std::string mName{};
        std::string mDesc{};
        std::vector<Arg> mArgs{};
        std::vector<std::string> mPositionals{};
        size_t mLongest{0};
        bool mRequired{false};
        bool mInternal{false};
        bool mInteractive{false};
        bool mHasGlobalFlags{false};
        bool mTakesArgs{false};
        Handler mHandler{nullptr};
        std::unordered_map<std::string_view, std::string, std::hash<std::string_view>, std_string_case_eq> mParsed{};
    };

    class Parser {
    public:
        Parser(std::string app, std::string version, std::string descript = nullptr);
        template <typename... T>
        Parser& operator()(T... obj) {
            Ego.add(std::forward<T>(obj)...);
            return Ego;
        }

        void  parse(int argc, char *argv[]);
        void  parse(Arguments& arguments) { parse(arguments.Argc, arguments.Argv); }
        void  handle();
        void  getCommandHelp(std::ostream& os, Command& cmd, bool isHelp);
        const Command* getCommand() const {
            return mParsed;
        }

        template <typename T>
        std::string operator[](char sf) {
            std::string _{nullptr};
            Arg *arg = findArg(_, sf);
            if (arg) {
                return Ego.getValue(arg->Lf, arg);
            }
            return {};
        }

        std::string operator[](const std::string_view& lf)
        {
            return getValue(lf, nullptr);
        }

    private suil_ut:
        template <typename... Commands>
        Parser& add(Command&& cmd, Commands&&... cmds)
        {
            add(std::move(cmd));
            if constexpr(sizeof...(cmds) > 0) {
                add(std::forward<Commands>(cmds)...);
            }
            return Ego;
        }

        template <typename... Args>
        Parser& add(Arg&& arg, Args&&... as)
        {
            add(std::move(arg));
            if constexpr(sizeof...(Args) > 0) {
                add(std::forward<Args>(as)...);
            }
            return Ego;
        }

        Parser& add(Command&& cmd);
        Parser& add(Arg&& arg);
        template <typename... T>
        Parser& add(std::function<void(Parser&)> fn) {
            fn(Ego);
            return Ego;
        }

        std::string     getValue(const std::string_view&, Arg* arg);
        void            showHelp(const char *prefix = nullptr);
        void            getHelp(std::ostream& os, const char *prefix = nullptr);
        Command*        find(const std::string_view& name);
        Arg* findArg(const std::string_view& name, char sf=NOSF);
        std::vector<Command>  mCommands;
        std::vector<Arg> mGlobals;
        // this is the command that successfully passed
        Command   *mParsed{nullptr};
        std::string    mAppName;
        std::string    mDescription;
        std::string    mAppVersion;
        size_t    mLongestCmd{0};
        size_t    mLongestFlag{0};
        bool      mTakesArgs{false};
        bool      mInter{false};
    };

    Status<std::string> readParam(const std::string_view& display, const std::string& def);
    Status<std::string> readPasswd(const std::string_view& display);
}
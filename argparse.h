#include <string>
#include <variant>
#include <vector>
#include <unordered_map>
#include <limits>
#include <sstream>

namespace argparse
{
    static constexpr int NARGS_AT_LEAST_ZERO = std::numeric_limits<int>::max();
    static constexpr int NARGS_AT_LEAST_ONE = std::numeric_limits<int>::max() - 1;

    // Specification for a command line argument
    class Argument
    {
    public:
        std::string name; // determine whether this is positional vs optional based on -- / -
        std::string shortName; // defaults to empty string
        unsigned int nargs; // default to 1
        bool required; // default false for optionals, true for positionals
        std::variant<std::string, std::vector<std::string>> defaultValue;
        std::string help; // defaults to empty string
        std::string metavar; // defaults to NAME for optionals, name for positionals
        
        Argument(const std::string& name);
        bool isPositional() const;
        bool isDefaultValueSet() const;
    };

    // A parser for command line arguments.
    // Usage: Specify arguments with addArgument, then call parse, then retrieve argument values
    //        with the get functions.
    class ArgumentParser
    {
    public:
        void addArgument(const Argument& arg);

        // Parses the command line arguments. If -h / --help is found, then prints the help info
        // and then calls exit(0).
        void parse(int argc, char** argv);
        void parse(const std::vector<std::string>& args);

        template <typename T>
        T get(const std::string& argName)
        {
            if (values.count(argName) == 0)
            {
                throw std::invalid_argument("ArgumentParser: Argument not found: " + argName);
            }
            if (values[argName].size() > 1)
            {
                throw std::invalid_argument("ArgumentParser: Argument has more than 1 value: " + argName);
            }
            return convertFromString<T>(values[argName][0], argName);
        }

        template <typename T>
        void get(const std::string& argName, std::vector<T>& outValues)
        {
            if (values.count(argName) == 0)
            {
                throw std::invalid_argument("ArgumentParser: Argument not found: " + argName);
            }
            for (const std::string& str: values[argName])
            {
                outValues.push_back(convertFromString<T>(str, argName));
            }
        }

        void printUsage(const std::string& programName);
        void printHelp(const std::string& programName);

    private:
        // The optional arguments, in the order given to us by addArgument
        std::vector<Argument> optionals;

        // Maps optional arg name to index in the optionals vector
        std::unordered_map<std::string, int> optIndex;

        // Maps optional arg shortName to name. Only add to this when shortName is specified (not "")
        std::unordered_map<std::string, std::string> optNames;

        // Positional arguments, in the order given to us by addArgument (and thus in the order
        // we expect to see them when parsing)
        std::vector<Argument> positionals;

        // Values parsed from the arguments. If a non-required argument wasn't found, put the
        // Argument's default value here. Keyed by Argument name.
        std::unordered_map<std::string, std::vector<std::string>> values;

        template <typename T>
        static T convertFromString(const std::string& str, const std::string& argName)
        {
            std::stringstream ss(str);
            T val;
            ss >> val;
            if (ss.fail())
            {
                throw std::invalid_argument("ArgumentParser: Argument value "+str+" could not be converted to type for argument "+argName);
            }
            else
            {
                return val;
            }
        }

        template <>
        static bool convertFromString(const std::string& str, const std::string& argName)
        {
            std::stringstream ss(str);
            bool val;
            ss >> std::boolalpha >> val;
            if (ss.fail())
            {
                throw std::invalid_argument("ArgumentParser: Argument value "+str+" could not be converted to type bool for argument "+argName);
            }
            else
            {
                return val;
            }
        }

        template <>
        static std::string convertFromString(const std::string& str, const std::string& argName)
        {
            return str;
        }
    };
}

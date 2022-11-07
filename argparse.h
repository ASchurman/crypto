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
        std::string shortName; // Must be '-' followed by 1 char. Ignored for positionals. Defaults to empty string.
        unsigned int nargs; // default to 1
        bool required; // default false for optionals, true for positionals
        std::variant<std::string, std::vector<std::string>> defaultValue;
        std::string help; // defaults to empty string
        std::string metavar; // defaults to NAME for optionals, name for positionals
        std::vector<std::string> choices; // Valid values for this option. If empty, all values are valid.
        
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
        // Adds an Argument specification to the parser. Each specification must be added to the parser
        // before calling the parse function.
        void addArgument(const Argument& arg);

        // Adds a group of mutually exclusive optional Arguments. Each argument must be an option,
        // and none of them can be required options.
        // If the parameter required == true, then exactly 1 of these mutually exclusive arguments
        // must be present.
        // Throws if any of the args are positional, or if any are marked required, or if only 1 arg
        // is provided.
        // Once an Argument has been added with this function, do not add it with addArgument. Use
        // either this function or addArgument to add an Argument.
        void addMutuallyExclusiveArguments(const std::vector<Argument>& args, bool required=false);

        // Parses the command line arguments. (If -h / --help is found, then prints the help info
        // and then calls exit(0).)
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

    private:
        std::string programName;

        // The optional arguments, in the order given to us by addArgument
        std::vector<Argument> optionals;

        // Maps optional arg name to index in the optionals vector
        std::unordered_map<std::string, int> optIndex;

        // Maps optional arg shortName to name. Only add to this when shortName is specified (not "").
        // Strip initial '-' before adding the shortName.
        std::unordered_map<std::string, std::string> optNames;

        // Positional arguments, in the order given to us by addArgument (and thus in the order
        // we expect to see them when parsing)
        std::vector<Argument> positionals;

        // Values parsed from the arguments. If a non-required argument wasn't found, put the
        // Argument's default value here. Keyed by Argument name.
        std::unordered_map<std::string, std::vector<std::string>> values;

        struct ExclusiveSet
        {
        public:
            int firstIndex; // First index in optionals vector of the exclusive set
            int lastIndex;  // Last index in optionals vector of the exclusive set
            bool required;  // Is user required to provide exactly 1 opt from this set?
        };
        std::vector<ExclusiveSet> exclusiveSets;

        // Map argument name to index in the exclusiveSets vector
        std::unordered_map<std::string, int> exclusiveIndices;

        void validateValues(const std::vector<Argument>& arguments);
        std::string getArgumentUsage(const Argument& arg);
        std::vector<std::string> expandShortArgs(const std::vector<std::string>& args);
        int getNArgs(const std::vector<std::string>& args,
                     int argsIndex,
                     int nargs,
                     const std::string& argName,
                     std::vector<std::string>& outArgs,
                     bool ignoreFlags);
        
        void printUsage();
        void printHelp();

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

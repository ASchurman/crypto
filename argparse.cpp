#include "argparse.h"
#include <stdexcept>
#include <algorithm>
#include <cassert>
#include <iostream>

#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__)
#include <Windows.h>
#elif defined(__linux__) || defined(__MACH__)
#include <sys/ioctl.h>
#include <unistd.h>
#endif

using namespace argparse;
using std::string;
using std::vector;
using std::unordered_map;
using std::variant;
using std::cout;

////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////// Argument Functions ////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////

Argument::Argument(const string& name)
{
    if (name.empty())
    {
        throw std::invalid_argument("Argument: name cannot be empty");
    }
    this->name = name;
    this->shortName = "";
    this->nargs = 1;
    this->help = "";
    if (isPositional())
    {
        this->metavar = name;
        this->required = true;
    }
    else // this is optional
    {
        std::size_t first = name.find_first_not_of("-");
        this->metavar = name.substr(first, name.length());
        std::transform(this->metavar.begin(), this->metavar.end(), this->metavar.begin(), ::toupper);

        this->required = false;
    }
    this->defaultValue = "";
}

bool Argument::isPositional() const
{
    if (name.empty())
    {
        throw std::invalid_argument("Argument: name cannot be empty");
    }
    return name.substr(0, 1) != "-";
}

bool Argument::isDefaultValueSet() const
{
    // defaultValue defaults to empty string. So first check if defaultValue is a string.
    // Then check whether this string of defaultValue is empty.
    // If it's empty, then defaultValue is unset.
    return !(std::holds_alternative<string>(defaultValue) &&
             std::get<string>(defaultValue).empty());
}

/////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////// ArgumentParser Functions//////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////
/////////// Adding and Validating Argument Specifications ///////////////
/////////////////////////////////////////////////////////////////////////

static void validateArgument(const Argument& arg)
{
    // Validate name
    if (arg.name.empty())
    {
        throw std::invalid_argument("ArgumentParser: argument name cannot be empty");
    }

    // Validate shortName
    if (!arg.shortName.empty() &&
        (arg.shortName.length() != 2 || arg.shortName[0] != '-'))
    {
        throw std::invalid_argument("ArgumentParser: shortName must be '-' followed by a single letter insted of: "+arg.shortName);
    }

    // Validate nargs
    if (arg.nargs == 0)
    {
        if (arg.isPositional())
        {
            throw std::invalid_argument("ArgumentParser: nargs=0 requires an optional argument: " + arg.name);
        }
    }
    
    // Validate defaultValue
    if (arg.isDefaultValueSet())
    {
        if (arg.nargs > 1)
        {
            if (!std::holds_alternative<vector<string>>(arg.defaultValue))
            {
                throw std::invalid_argument("ArgumentParser: nargs > 1 requires defaultValue to be a vector for argument: " + arg.name);
            }
            else
            {
                // defaultValue holds a vector for nargs>1. But is it the right size of vector?
                vector<string> v = std::get<vector<string>>(arg.defaultValue);
                if (arg.nargs == NARGS_AT_LEAST_ONE && v.empty())
                {
                    throw std::invalid_argument("ArgumentParser: defaultValue and nargs aren't the same size for argument: " + arg.name);
                }
                else if (arg.nargs != NARGS_AT_LEAST_ZERO && arg.nargs != v.size())
                {
                    throw std::invalid_argument("ArgumentParser: defaultValue and nargs aren't the same size for argument: " + arg.name);
                }

                // Validate that the defaultValue vector has only values from choices, if choices was set
                if (!arg.choices.empty())
                {
                    for (const string& s: v)
                    {
                        if (std::find(arg.choices.begin(), arg.choices.end(), s) == arg.choices.end())
                        {
                            throw std::invalid_argument("ArgumentParser: defaultValue contains invalid value "+s+" for argument: "+arg.name);
                        }
                    }
                }
            }
        }
        else if (!std::holds_alternative<string>(arg.defaultValue))
        {
            throw std::invalid_argument("ArgumentParser: nargs<=1 requires defaultValue to be a single value for argument:" + arg.name);
        }
        else if (!arg.choices.empty())
        {
            // Validate that the single string defaultValue is in choices
            string s = std::get<string>(arg.defaultValue);
            if (std::find(arg.choices.begin(), arg.choices.end(), s) == arg.choices.end())
            {
                throw std::invalid_argument("ArgumentParser: defaultValue is invalid value "+s+" for argument: "+arg.name);
            }
        }
    }
}

void ArgumentParser::addArgument(const Argument& arg)
{
    validateArgument(arg); // throws if arg is malformed

    if (arg.isPositional())
    {
        positionals.push_back(arg);
    }
    else
    {
        optionals.push_back(arg);
        if (arg.nargs == 0)
        {
            optionals.back().defaultValue = "false";
        }
        optIndex.insert({arg.name, optionals.size()-1});
        if (!arg.shortName.empty())
        {
            if (arg.shortName[0] != '-')
            {
                throw std::invalid_argument("ArgumentParser: shortName for an option must begin with '-'");
            }
            optNames.insert({arg.shortName.substr(1), arg.name});
        }
    }
}

void ArgumentParser::addMutuallyExclusiveArguments(const vector<Argument>& args, bool required)
{
    ExclusiveSet set;
    set.required = required;
    set.firstIndex = optionals.size();
    set.lastIndex = optionals.size() + args.size() - 1;
    exclusiveSets.push_back(set);

    for (int i = 0; i < args.size(); i++)
    {
        if (args[i].isPositional())
        {
            throw std::invalid_argument("ArgumentParser: Mutually exclusive argument cannot be positional: "+args[i].name);
        }
        else if (args[i].required)
        {
            throw std::invalid_argument("ArgumentParser: Mutually exclusive option cannot be required: "+args[i].name);
        }

        exclusiveIndices.insert({args[i].name, exclusiveSets.size()-1});
        addArgument(args[i]);
    }
}

/////////////////////////////////////////////////////////////////////////
//////////////////// Parsing Command Line Arguments /////////////////////
/////////////////////////////////////////////////////////////////////////

void ArgumentParser::parse(int argc, char** argv)
{
    vector<string> args(argv, argv + argc);
    parse(args);
}

void ArgumentParser::parse(const vector<string>& args)
{
    // Expand short args into their long names (including expanding -ab into --aName --bName)
    if (optNames.count("h") == 0) optNames.insert({"h", "--help"});
    vector<string> expArgs = expandShortArgs(args);
    programName = expArgs[0];
    int argsIndex = 1; // index in expArgs of the next index we're parsing

    // Start with parsing optionals, then positionals.
    // Becomes true when we find our first positional argument or the end-of-options delimiter
    bool readingPositionals = false;

    // Have we found the end-of-options delimiter "--"? (This needs to be distinct from
    // readingPositionals in order not to throw when we see arg starting with '-' after "--".)
    bool foundEndOfOptions = false;

    // Index in positionals vector of the next positional argument to find.
    int posIndex = 0;

    while (argsIndex < expArgs.size())
    {
        string baseArg = expArgs[argsIndex];
        argsIndex++;

        if ((baseArg[0] == '-') && !foundEndOfOptions) // Is this an option or the end-of-options delimiter?
        {
            if (readingPositionals)
            {
                printUsage();
                throw std::invalid_argument("ArgumentParser: Optional argument found after positional argument: " + baseArg);
            }
            else if (baseArg == "--")
            {
                // "--" ends the options. Any following arguments are to be interpretted as positionals,
                // even if they begin with '-'.
                readingPositionals = true;
                foundEndOfOptions = true;
            }
            else if (baseArg == "--help")
            {
                printHelp();
                exit(0);
            }
            else if (optIndex.count(baseArg) == 0)
            {
                printUsage();
                throw std::invalid_argument("ArgumentParser: Invalid option: " + baseArg);
            }
            else // This is an optional argument for which we have an Argument specification!
            {
                Argument& argSpec = optionals[optIndex[baseArg]];
                if (values.count(argSpec.name) > 0)
                {
                    printUsage();
                    throw std::invalid_argument("ArgumentParser: Option found more than once: " + baseArg);
                }
                else if (argSpec.nargs == 0)
                {
                    // nargs == 0 means that this is a boolean flag. The user provided it, so its value is true.
                    values.insert({argSpec.name, vector<string>{"true"}});
                }
                else
                {
                    // Get the option arguments for this baseArg.
                    // How many we try to get depends on this baseArg's nargs value.
                    vector<string> optArgs;
                    assert(foundEndOfOptions == false);
                    argsIndex = getNArgs(expArgs, argsIndex, argSpec.nargs, argSpec.name, optArgs, foundEndOfOptions);

                    // Now that we've gotten the option arguments for this baseArg, add them to the values map
                    values.insert({argSpec.name, optArgs});
                }
            }
        }
        else // This is a positional argument.
        {
            if (!readingPositionals) readingPositionals = true;

            if (posIndex < positionals.size())
            {
                // Get the Argument specification
                Argument& argSpec = positionals[posIndex];
                posIndex++;

                assert(values.count(argSpec.name) == 0); // if this value has already been written, I've messed up
                assert(argSpec.nargs != 0); // this should have already been validated before parsing

                // Interpret baseArgs as the first argument arg,
                // then get the rest of the args based on the specification's nargs value
                vector<string> argArgs({baseArg});
                if (argSpec.nargs == NARGS_AT_LEAST_ONE || argSpec.nargs == NARGS_AT_LEAST_ZERO)
                {
                    // we've already gotten 1, so we need at least 0 more
                    argsIndex = getNArgs(expArgs, argsIndex, NARGS_AT_LEAST_ZERO, argSpec.name, argArgs, foundEndOfOptions);
                }
                else
                {
                    argsIndex = getNArgs(expArgs, argsIndex, argSpec.nargs-1, argSpec.name, argArgs, foundEndOfOptions);
                }
                values.insert({argSpec.name, argArgs}); // then add them to the values map
            }
            else
            {
                printUsage();
                throw std::invalid_argument("ArgumentParser: Too many positional arguments.");
            }
        }
    }
    validateValues(optionals);
    validateValues(positionals);
}

// Creates a new argument vector by iterating through args and replacing any short named options
// with a long name option. If multiple short options are combined behind a single '-', expands
// these into separate arguments
vector<string> ArgumentParser::expandShortArgs(const vector<string>& args)
{
    vector<string> expandedArgs;
    bool foundDoubleDash = false;
    for (const string& str: args)
    {
        if (str.length() > 1 && str[0] == '-' && str[1] != '-' && !foundDoubleDash)
        {
            // This is a short option. Expand it if necessary
            for (int i = 1; i < str.length(); i++)
            {
                string opt(1, str[i]);
                if (optNames.count(opt) == 1)
                {
                    expandedArgs.push_back(optNames.at(opt));
                }
                else
                {
                    printUsage();
                    throw std::invalid_argument("ArgumentParser: Invalid argument: "+str);
                }
            }
        }
        else
        {
            if (str == "--") foundDoubleDash = true;
            expandedArgs.push_back(str);
        }
    }
    return expandedArgs;
}

// Gets a span of nargs option arguments from args, starting at index argsIndex, putting them in outArgs.
// Returns the index in args directly after the last optArg taken, so that args can continue to be parsed
// starting from the returned index.
// If ignoreFlags==true, then don't interpret arguments starting with '-' as optional arguments (i.e.,
// "--" was previously parsed).
int ArgumentParser::getNArgs(const vector<string>& args,
                             int argsIndex,
                             int nargs,
                             const string& argName,
                             vector<string>& outArgs,
                             bool ignoreFlags)
{
    if (nargs == 0)
    {
        return argsIndex;
    }
    if (nargs == NARGS_AT_LEAST_ONE || nargs == NARGS_AT_LEAST_ZERO)
    {
        int numArgs = 0;
        while (argsIndex < args.size() && (args[argsIndex][0] != '-' || ignoreFlags))
        {
            outArgs.push_back(args[argsIndex]);
            numArgs++;
            argsIndex++;
        }
        if (nargs == NARGS_AT_LEAST_ONE && numArgs == 0)
        {
            printUsage();
            throw std::invalid_argument("ArgumentParser: Not enough arguments provided for option: "+argName);
        }
    }
    else
    {
        for (int n = 0; n < nargs; n++)
        {
            if (argsIndex < args.size() && (args[argsIndex][0] != '-' || ignoreFlags))
            {
                outArgs.push_back(args[argsIndex]);
                argsIndex++;
            }
            else
            {
                printUsage();
                throw std::invalid_argument("ArgumentParser: Not enough arguments provided for option: " + argName);
            }
        }
    }
    return argsIndex;
}

// Validate argument values (e.g., required args were found, mutually exclusive args weren't found) and
// record default values of arguments not found
void ArgumentParser::validateValues(const vector<Argument>& arguments)
{
    // Validate mutually exclusive sets
    if (!arguments.front().isPositional())
    {
        for (const ExclusiveSet& set: exclusiveSets)
        {
            bool foundArg = false;
            for (int i = set.firstIndex; i <= set.lastIndex; i++)
            {
                if (values.count(arguments[i].name) > 0)
                {
                    if (foundArg)
                    {
                        printUsage();
                        throw std::invalid_argument("ArgumentParser: Found mutually exclusive arguments: "+arguments[i].name);
                    }
                    foundArg = true;
                }
            }
            if (set.required && !foundArg)
            {
                printUsage();
                throw std::invalid_argument("ArgumentParser: Required mutually exclusive argument not found");
            }
        }
    }

    for (const Argument& argSpec: arguments)
    {
        if (values.count(argSpec.name) == 0)
        {
            if (argSpec.required)
            {
                printUsage();
                throw std::invalid_argument("ArgumentParser: required argument not found: "+argSpec.name);
            }
            else
            {
                // Argument isn't required, and it wasn't found when parsing.
                // Add its default value to the values map.
                if (std::holds_alternative<string>(argSpec.defaultValue))
                {
                    // The default value is a singleton, but the values map contains vectors.
                    // Make a vector of the appropriate type with the single value.
                    values.insert({argSpec.name, {std::get<string>(argSpec.defaultValue)}});
                }
                else
                {
                    assert(std::holds_alternative<vector<string>>(argSpec.defaultValue));
                    values.insert({argSpec.name, std::get<vector<string>>(argSpec.defaultValue)});
                }
            }
        }
        else if (!argSpec.choices.empty())
        {
            for (const string& val: values[argSpec.name])
            {
                if (std::find(argSpec.choices.begin(), argSpec.choices.end(), val) == argSpec.choices.end())
                {
                    printUsage();
                    throw std::invalid_argument("ArgumentParser: Invalid value for argument "+argSpec.name+" : "+val);
                }
            }
        }
    }
}

/////////////////////////////////////////////////////////////////////////
///////////////////// Printing --help info //////////////////////////////
/////////////////////////////////////////////////////////////////////////

static int getTerminalWidth()
{
#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__)
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi);
    return csbi.srWindow.Right - csbi.srWindow.Left + 1;
#elif defined(__linux__) || defined(__MACH__)
    struct winsize w;
    ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);
    return w.ws_col;
#else
    return 80;
#endif
}

static const int DESCRIPTION_COL_INDENT = 24;

// Get the description string for an argument by text-wrapping the provided
// description. Decides to start on the current line or a new line based on
// whether the preceding usage string was longer than the description column indent.
static string getDescription(bool longUsage, const string& description)
{
    if (description.empty())
    {
        return "";
    }

    // If usage was long, we need to start on a new line. Otherwise, start right away
    string s = "";
    if (longUsage)
    {
        s = "\n";
        s.append(DESCRIPTION_COL_INDENT, ' ');
    }

    // The description cannot exceed the terminal width. Every colWidth characters, we need
    // to split a line. Split a line by finding the last preceding space and replacing it
    // with \n followed by DESCRIPTION_COL_INDENT spaces.
    int colWidth = getTerminalWidth() - DESCRIPTION_COL_INDENT;

    string wrapString = "\n";
    wrapString.append(DESCRIPTION_COL_INDENT, ' ');

    int colPos = 0;
    for (int i = 0; i < description.length(); i++)
    {
        if (colPos < colWidth - 1)
        {
            s += description[i];
            colPos++;
        }
        else
        {
            // description[i] would exceed column width. We need to wrap.
            if (description[i] == ' ')
            {
                s += wrapString;
                colPos = 0;
            }
            else
            {
                // Look back to find the last space and replace it with wrapString
                std::size_t lastSpace = s.find_last_of(' ');
                colPos = s.length() - lastSpace + 1;
                s.replace(lastSpace, 1, wrapString);
                s += description[i];
            }
        }
    }
    return s;
}

// Gets the the metavar part of the usage string, based on the argument's nargs value.
static string getMetavarUsage(const Argument& arg)
{
    string s = "";
    if (arg.nargs == NARGS_AT_LEAST_ONE)
    {
        s += " " + arg.metavar;
    }
    if (arg.nargs == NARGS_AT_LEAST_ONE || arg.nargs == NARGS_AT_LEAST_ZERO)
    {
        s += " [" + arg.metavar + " ...]";
    }
    else
    {
        for (int i = 0; i < arg.nargs; i++)
        {
            s += " " + arg.metavar;
        }
    }
    return s;
}

// Gets the usage string for a single argument.
string ArgumentParser::getArgumentUsage(const Argument& arg)
{
    string s = "";
    if (!arg.required &&
        !(arg.isPositional() && arg.nargs == NARGS_AT_LEAST_ZERO) &&
        exclusiveIndices.count(arg.name) == 0)
    {
        s += "[";
    }
    if (!arg.isPositional())
    {
        if (arg.shortName.empty())
        {
            s += arg.name;
        }
        else
        {
            s += arg.shortName;
        }
    }
    s += getMetavarUsage(arg);
    if (!arg.required &&
        !(arg.isPositional() && arg.nargs == NARGS_AT_LEAST_ZERO) &&
        exclusiveIndices.count(arg.name) == 0)
    {
        s += "]";
    }
    return s;
}

void ArgumentParser::printUsage()
{
    cout << "usage: " << programName << " [-h] ";

    // First print optionals
    bool inMutuallyExclusiveGroup = false;
    for (int i = 0; i < optionals.size(); i++)
    {
        if (!inMutuallyExclusiveGroup && exclusiveIndices.count(optionals[i].name) > 0)
        {
            // This is the first arg in a mutually exclusive group
            inMutuallyExclusiveGroup = true;
            if (exclusiveSets[exclusiveIndices[optionals[i].name]].required)
            {
                cout << "(";
            }
            else
            {
                cout << "[";
            }
        }
        else if (inMutuallyExclusiveGroup && exclusiveIndices.count(optionals[i].name) > 0)
        {
            // This is in a mutually exclusive group, and it's not the first one
            cout << "| ";
        }
        else if (inMutuallyExclusiveGroup && exclusiveIndices.count(optionals[i].name) == 0)
        {
            // Previous arg was the last in a mutually exclusive group
            inMutuallyExclusiveGroup = false;
            if (exclusiveSets[exclusiveIndices[optionals[i-1].name]].required)
            {
                cout << ")";
            }
            else
            {
                cout << "]";
            }
        }
        cout << getArgumentUsage(optionals[i]);
        if (i+1 < optionals.size()) cout << " ";
    }
    // Check if the last of the optionals was a mutually exclusive group
    if (exclusiveIndices.count(optionals.back().name) > 0)
    {
        if (exclusiveSets[exclusiveIndices[optionals.back().name]].required)
        {
            cout << ")";
        }
        else
        {
            cout << "]";
        }
    }

    // Then print positionals
    for (Argument& arg: positionals)
    {
        cout << getArgumentUsage(arg);
    }
    cout << "\n";
}

void ArgumentParser::printHelp()
{
    printUsage();

    if (positionals.size() > 0)
    {
        cout << "\npositional arguments:\n";
        for (Argument& arg: positionals)
        {
            string s = "  " + arg.metavar;
            if (s.length() < DESCRIPTION_COL_INDENT)
            {
                s.append(DESCRIPTION_COL_INDENT - s.length(), ' ');
                cout << s << getDescription(false, arg.help) << "\n";
            }
            else
            {
                cout << s << getDescription(true, arg.help) << "\n";
            }
        }
    }

    cout << "\noptions:\n";
    string helpStr = "  -h, --help";
    helpStr.append(DESCRIPTION_COL_INDENT - helpStr.length(), ' ');
    cout << helpStr << getDescription(false, "show this help message and exit") << "\n";
    for (Argument& arg: optionals)
    {
        string s = "  ";
        if (!arg.shortName.empty())
        {
            s += arg.shortName + getMetavarUsage(arg) + ", ";
        }
        s += arg.name + getMetavarUsage(arg);
        if (s.length() < DESCRIPTION_COL_INDENT)
        {
            s.append(DESCRIPTION_COL_INDENT - s.length(), ' ');
            cout << s << getDescription(false, arg.help) << "\n";
        }
        else
        {
            cout << s << getDescription(true, arg.help) << "\n";
        }
    }
}

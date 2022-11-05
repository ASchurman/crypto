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
        throw std::invalid_argument("ArgumentParser::Argument: name cannot be empty");
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
        throw std::invalid_argument("ArgumentParser::Argument: name cannot be empty");
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
        throw std::invalid_argument("ArgumentParser: shortName must be '-' followed by a single letter: "+arg.shortName);
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
            }
        }
        else if (!std::holds_alternative<string>(arg.defaultValue))
        {
            throw std::invalid_argument("ArgumentParser: nargs<=1 requires defaultValue to be a single value for argument:" + arg.name);
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
            optNames.insert({arg.shortName, arg.name});
        }
    }
}

/////////////////////////////////////////////////////////////////////////
//////////////////// Parsing Command Line Arguments /////////////////////
/////////////////////////////////////////////////////////////////////////

// Gets a span of nargs option arguments from args, starting at index i, putting them in optArgs.
// Returns index in args directly after the last optArg taken.
static int getNArgs(const vector<string>& args, int i, int nargs, const string& argName, vector<string>& optArgs)
{
    if (nargs == 0)
    {
        return i;
    }
    if (nargs == NARGS_AT_LEAST_ONE || nargs == NARGS_AT_LEAST_ZERO)
    {
        int numArgs = 0;
        while (i < args.size() && args[i][0] != '-')
        {
            optArgs.push_back(args[i]);
            numArgs++;
            i++;
        }
        if (nargs == NARGS_AT_LEAST_ONE && numArgs == 0)
        {
            throw std::invalid_argument("ArgumentParser: Not enough arguments provided for option: "+argName);
        }
    }
    else
    {
        for (int n = 0; n < nargs; n++)
        {
            if (i < args.size() && args[i][0] != '-')
            {
                optArgs.push_back(args[i]);
                i++;
            }
            else
            {
                throw std::invalid_argument("ArgumentParser: Not enough arguments provided for option: " + argName);
            }
        }
    }
    return i;
}

// Iterate through the arguments. For each of them:
// - if they're required then make sure that there's a value for them. (throw otherwise)
// - if they're not required, and they're not in values, then set the default value.
static void verifyValues(const vector<Argument>& arguments, unordered_map<string, vector<string>>& values)
{
    for (const Argument& argSpec: arguments)
    {
        if (values.count(argSpec.name) == 0)
        {
            if (argSpec.required)
            {
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
        else
        {
            assert(values.at(argSpec.name).size() > 0);
        }
    }
}

void ArgumentParser::parse(int argc, char** argv)
{
    vector<string> args(argv, argv + argc);
    parse(args);
}

void ArgumentParser::parse(const vector<string>& args)
{
    string programName = args[0];
    bool readingPositionals = false; // start with optionals, then positionals
    int posIndex = 0; // index in positionals of next positional argument to find
    int i = 1;
    while (i < args.size())
    {
        string baseArg = args[i];
        i++;
        if (baseArg == "-h" || baseArg == "--help")
        {
            printHelp(programName);
            exit(0);
        }

        if (baseArg[0] == '-')
        {
            // This is an option
            if (readingPositionals)
            {
                throw std::invalid_argument("ArgumentParser: Optional argument found after positional argument: " + baseArg);
            }

            if (optIndex.count(baseArg) == 0 && optNames.count(baseArg) == 0)
            {
                throw std::invalid_argument("ArgumentParser: Invalid option: " + baseArg);
            }
            Argument& argSpec = (optIndex.count(baseArg)==1) ? optionals[optIndex[baseArg]] : optionals[optIndex[optNames[baseArg]]];

            if (values.count(argSpec.name) > 0)
            {
                throw std::invalid_argument("ArgumentParser: Option found more than once: " + baseArg);
            }

            if (argSpec.nargs == 0)
            {
                values.insert({argSpec.name, vector<string>{"true"}});
            }
            else
            {
                // Get the option arguments for this baseArg.
                // How many we try to get depends on this baseArg's nargs value.
                vector<string> optArgs;
                i = getNArgs(args, i, argSpec.nargs, argSpec.name, optArgs);

                // Now that we've gotten the option arguments for this baseArg, add them to the values map
                values.insert({argSpec.name, optArgs});
            }
        }
        else
        {
            // This is a positional argument
            if (!readingPositionals) readingPositionals = true;

            if (posIndex < positionals.size())
            {
                Argument& argSpec = positionals[posIndex];
                posIndex++;
                assert(values.count(argSpec.name) == 0); // if this value has already been written, I've messed up
                assert(argSpec.nargs != 0); // this should have already been validated before parsing
                vector<string> argArgs({baseArg}); // interpret baseArg as the first argument arg
                // get the rest of the argument args
                if (argSpec.nargs == NARGS_AT_LEAST_ONE || argSpec.nargs == NARGS_AT_LEAST_ZERO)
                {
                    // we've already gotten 1, so we need at least 0 more
                    i = getNArgs(args, i, NARGS_AT_LEAST_ZERO, argSpec.name, argArgs);
                }
                else
                {
                    i = getNArgs(args, i, argSpec.nargs-1, argSpec.name, argArgs);
                }
                values.insert({argSpec.name, argArgs}); // then add them to the values map
            }
            else
            {
                throw std::invalid_argument("ArgumentParser: Too many positional arguments.");
            }
        }
    }

    // Iterate through the optionals and positionals.
    // For each of them:
    //      if they're required then make sure that there's a value for them
    //      if they're not required, and they're not in values, then set the default value
    verifyValues(optionals, values);
    verifyValues(positionals, values);
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

// Get the description string. Decides how many \t and \n to use based on the length of usage.
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

static string getArgumentUsage(const Argument& arg)
{
    string s = arg.isPositional() ? "" : " ";
    if (!arg.required && !(arg.isPositional() && arg.nargs == NARGS_AT_LEAST_ZERO))
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
    if (!arg.required && !(arg.isPositional() && arg.nargs == NARGS_AT_LEAST_ZERO))
    {
        s += "]";
    }
    return s;
}

void ArgumentParser::printUsage(const string& programName)
{
    cout << "usage: " << programName << " [-h]";

    // First print optionals
    for (Argument& arg: optionals)
    {
        cout << getArgumentUsage(arg);
    }

    // Then print positionals
    for (Argument& arg: positionals)
    {
        cout << getArgumentUsage(arg);
    }
    cout << "\n";
}

void ArgumentParser::printHelp(const string& programName)
{
    printUsage(programName);

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

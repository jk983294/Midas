#ifndef MIDAS_COMMAND_ARG_H
#define MIDAS_COMMAND_ARG_H

#include <map>
#include <string>

using namespace std;

namespace midas {
enum SwitchArgType { SwitchStandalone, SwitchWithArg };
enum SwitchOptionType { SwitchMandatory, SwitchOptional };

struct CommandArg {
    char shortCmd;
    std::string longCmd;
    bool isWithArg;   // SwitchArgType
    bool isOptional;  // SwitchOptionType
    std::string argName;
    std::string help;

    CommandArg(char shortCmd_, std::string longCmd_, bool isWithArg_, bool isOptional_, std::string argName_,
               std::string help_)
        : shortCmd(shortCmd_),
          longCmd(longCmd_),
          isWithArg(isWithArg_),
          isOptional(isOptional_),
          argName(argName_),
          help(help_) {}
};

typedef std::shared_ptr<CommandArg> CommandArgPtr;
typedef std::map<char, CommandArgPtr> CommandArgs;
}

#endif

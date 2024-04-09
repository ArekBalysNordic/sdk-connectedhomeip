/*
 *
 *    Copyright (c) 2024 Project CHIP Authors
 *
 *    Licensed under the Apache License, Version 2.0 (the "License");
 *    you may not use this file except in compliance with the License.
 *    You may obtain a copy of the License at
 *
 *        http://www.apache.org/licenses/LICENSE-2.0
 *
 *    Unless required by applicable law or agreed to in writing, software
 *    distributed under the License is distributed on an "AS IS" BASIS,
 *    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *    See the License for the specific language governing permissions and
 *    limitations under the License.
 */

#include <lib/core/CHIPCore.h>
#include <lib/shell/Commands.h>
#if CONFIG_DEVICE_LAYER
#include <platform/CHIPDeviceLayer.h>
#endif
#include <lib/shell/Engine.h>
#include <lib/shell/commands/Help.h>
#include <lib/support/CHIPArgParser.hpp>
#include <lib/support/CHIPMem.h>
#include <lib/support/CodeUtils.h>
#include <lib/support/VariableStats.h>

namespace chip {
namespace Shell {

CHIP_ERROR BLEHelpHandler()
{
    streamer_t * sout = streamer_get();

    // TODO Move it to subcommands' help
    streamer_printf(sout, "use -l to list all possible variables, use -p to read a value from persistent storage");
    streamer_printf(sout, "Possible options:\r\n matter variable [name]\r\n matter variable -l\r\n matter variable -p [name]");

    return CHIP_NO_ERROR;
}

CHIP_ERROR VariableDispatch(int argc, char ** argv)
{
    if (argc == 0 || argc > 2)
    {
        BLEHelpHandler();
        return CHIP_NO_ERROR;
    }

    streamer_t * sout = streamer_get();
    uint32_t value    = 0;
    bool persistent   = false;

    // TODO Make subcommand for -l and -p
    if (strcmp(argv[0], "-l") == 0 && strlen(argv[0]) == strlen("-l"))
    {
        // TODO parametrize size of the following table
        char availableVariables[(10 + 1) * 10 + 1] = {};

        chip::VariableStats::GetList(availableVariables, sizeof(availableVariables));
        // TODO ensure that there is null termination
        streamer_printf(sout, "%s", availableVariables);
        return CHIP_NO_ERROR;
    }
    if (strcmp(argv[0], "-lp") == 0 && strlen(argv[0]) == strlen("-lp"))
    {
        // TODO parametrize size of the following table
        char availableVariables[(10 + 1) * 10 + 1] = {};

        // TODO Persistent storage list is not available yet
        // chip::VariableStats::GetList(availableVariables, sizeof(availableVariables));
        // TODO ensure that there is null termination
        // streamer_printf(sout, "%s", availableVariables);
        return CHIP_NO_ERROR;
    }
    else if (strcmp(argv[0], "-p") && strlen(argv[0]) == strlen("-p"))
    {
        persistent = true;
    }

    if (chip::VariableStats::Get(argv[0], value, persistent))
    {
        streamer_printf(sout, "%u\r\n", value);
    }
    else
    {
        streamer_printf(sout, "No variable registered under this record\r\n");
    }

    return CHIP_NO_ERROR;
}

void RegisterVariableCommands()
{
    static const shell_command_t sVariableCommand = { &VariableDispatch, "variable", "Variable statistics commands" };
    Engine::Root().RegisterCommands(&sVariableCommand, 1);
}
} // namespace Shell

} // namespace chip

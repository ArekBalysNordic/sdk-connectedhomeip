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

    streamer_printf(sout, "please provide variable name: matter variable [variable name]\r\n");

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

    if (argc == 1)
    {
        if (strcmp(argv[0], "list") == 0)
        {
            // TODO parametrize size of the following table
            char availableVariables[(10 + 1) * 10 + 1] = {};

            chip::VariableStats::GetList(availableVariables, sizeof(availableVariables));
            streamer_printf(sout, "%s", availableVariables);
            return CHIP_NO_ERROR;
        }

        uint32_t value = 0;
        if (chip::VariableStats::Get(argv[0], value))
        {
            streamer_printf(sout, "%u\r\n", value);
        }
        else
        {
            streamer_printf(sout, "No variable registered under this record\r\n");
        }
    }

    // TODO add support for persistent variables (depending on arguments...)

    return CHIP_NO_ERROR;
}

void RegisterVariableCommands()
{
    static const shell_command_t sVariableCommand = { &VariableDispatch, "variable", "Variable statistics commands" };
    Engine::Root().RegisterCommands(&sVariableCommand, 1);
}
} // namespace Shell

} // namespace chip
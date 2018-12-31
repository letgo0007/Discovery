/******************************************************************************
 * @file    cli_builtin.h
 * @brief   Built-in commands for CLI
 *
 * @author  Nick Yang
 * @date    2018/12/31
 * @version V1.0
 *****************************************************************************/

#ifndef CLI_BUILTIN_H_
#define CLI_BUILTIN_H_

#include "cli.h"

#ifndef CLI_NUM_OF_BUILTIN_CMD
#define CLI_NUM_OF_BUILTIN_CMD 10
#endif

int builtin_debug(int argc, char **args);
int builtin_help(int argc, char **args);
int builtin_history(int argc, char **args);
int builtin_repeat(int argc, char **args);
int builtin_sleep(int argc, char **args);
int builtin_test(int argc, char **args);
int builtin_time(int argc, char **args);
int builtin_version(int argc, char **args);

const CliCommand_TypeDef gConstBuiltinCmdList[CLI_NUM_OF_BUILTIN_CMD];

#endif /* CLI_BUILTIN_H_ */

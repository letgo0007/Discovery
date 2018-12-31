/******************************************************************************
 * @file    cli_port.h
 * @brief   Platform API port file for CLI.
 *
 * @author  Nick Yang
 * @date    2018/11/01
 * @version V1.0
 *****************************************************************************/
#ifndef CLI_PORT_H_
#define CLI_PORT_H_

/*! External CLI command list
 */
extern int cli_reset(int argc, char **argv);
extern int cli_info(int argc, char **argv);
extern int cli_mem(int argc, char **argv);
extern int cli_qspi(int argc, char **argv);
extern int cli_os(int argc, char **argv);
extern int cli_top(int argc, char **argv);
extern int cli_rtc(int argc, char **argv);
extern int cli_nvram(int argc, char **argv);

/*! Porting API
 */
extern void cli_sleep(int ms);
extern unsigned int cli_gettick(void);
extern void * cli_malloc(unsigned int size);
extern void cli_free(void *ptr);
extern int cli_port_init(void);
extern void cli_port_deinit(void);
extern int cli_port_getc(void);

#endif /* CLI_PORT_H_ */

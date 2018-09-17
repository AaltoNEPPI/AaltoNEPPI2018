
#include <stdio.h>

#include "thread.h"
#include "shell.h"
#include "shell_commands.h"

#define ENABLE_DEBUG (1)
#include "debug.h"

#include "neppi_shell.h"

NORETURN static void *shell_thread(void *arg)
{
    (void) puts("Welcome to NEPPI!");
    char line_buf[SHELL_DEFAULT_BUFSIZE];
    shell_run(NULL, line_buf, SHELL_DEFAULT_BUFSIZE);
}

void neppi_shell_start(void)
{
    static char thread_stack[THREAD_STACKSIZE_DEFAULT*2/*XXX for now, due to SD*/];
    thread_create(thread_stack, sizeof(thread_stack),
                  THREAD_PRIORITY_MAIN + 4, THREAD_CREATE_WOUT_YIELD,
                  shell_thread, NULL, "shell");
    DEBUG("neppi_shell: thread created\n");
    
}

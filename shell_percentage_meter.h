#define _GNU_SOURCE

#include <ctype.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <stdbool.h>
#include <errno.h>

#include <sys/types.h>
#include <sys/ioctl.h>

#include <unistd.h>

#define __shell_percentage__CLEAR(x) memset(&(x), 0, sizeof(x))

// Constants
static char *CODE_SAVE_CURSOR="\033[s";
static char *CODE_RESTORE_CURSOR="\033[u";
static char *CODE_CURSOR_IN_SCROLL_AREA="\033[1A";
static char *COLOR_FG="\e[30m";
static char *COLOR_BG="\e[42m";
static char *COLOR_BG_BLOCKED="\e[43m";
static char *RESTORE_FG="\e[39m";
char *RESTORE_BG="\e[49m";

static bool __shell_percentage__PROGRESS_BLOCKED=false;
static int __shell_percentage__CURRENT_NR_LINES=0;
#define __shell_percentage__XTERM_VERT_LINES 23
#define __shell_percentage__XTERM_HOR_COLUMS 79

static void __shell_percentage__setup_scroll_area(void);
static void __shell_percentage__printf_new(char c, int count);

static void __shell_percentage__run_cmd_return_output(const char *command, char *output)
{
    FILE *fp = NULL;
    char path[1035];
    __shell_percentage__CLEAR(path);
    char *pos = NULL;

    if (!output)
    {
        return;
    }

    /* Open the command for reading. */
    fp = popen(command, "r");

    if (fp == NULL)
    {
        output[0] = '\0';
        return;
    }

    /* Read the output a line at a time - output it. */
    while (fgets(path, sizeof(path) - 1, fp) != NULL)
    {
        snprintf(output, 299, "%s", (const char *)path);
    }

    if (strlen(output) > 1)
    {
        if ((pos = strchr(output, '\n')) != NULL)
        {
            *pos = '\0';
        }
    }

    /* close */
    pclose(fp);
}

static int __shell_percentage__tput_lines(void)
{
    char output_str[1000];
    __shell_percentage__CLEAR(output_str);
    __shell_percentage__run_cmd_return_output("tput lines", output_str);
    if (strlen(output_str) > 0)
    {
        int lines = (int)(strtol(output_str, NULL, 10));
        return lines;
    }
    
    return __shell_percentage__XTERM_VERT_LINES;
}

static int __shell_percentage__tput_cols(void)
{
    char output_str[1000];
    __shell_percentage__CLEAR(output_str);
    __shell_percentage__run_cmd_return_output("tput cols", output_str);
    if (strlen(output_str) > 0)
    {
        int cols = (int)(strtol(output_str, NULL, 10));
        return cols;
    }
    
    return __shell_percentage__XTERM_HOR_COLUMS;
}

static void __shell_percentage__clear_progress_bar(void)
{
    int lines = __shell_percentage__tput_lines();
    printf("%s", CODE_SAVE_CURSOR);
    printf("\033[%d;0f", lines);
    if (system("tput el")) {}
    printf("%s", CODE_RESTORE_CURSOR);
}

static void __shell_percentage__destroy_scroll_area(void)
{
    int lines = __shell_percentage__tput_lines();
    printf("%s", CODE_SAVE_CURSOR);
    printf("\033[0;%dr", lines);

    printf("%s", CODE_RESTORE_CURSOR);
    printf("%s", CODE_CURSOR_IN_SCROLL_AREA);
    
    __shell_percentage__clear_progress_bar();
    printf("\n");
    printf("\n");
}

static void __shell_percentage__print_bar_text(int percentage)
{
    int cols = __shell_percentage__tput_cols();
    int bar_size = cols - 17;
    int complete_size = (bar_size * percentage) / 100;
    int remainder_size = bar_size - complete_size;

    printf(" Progress ");
    printf("%d%%", percentage);
    printf(" ");

    printf("[");
    if (__shell_percentage__PROGRESS_BLOCKED)
    {
        printf("%s%s", COLOR_FG, COLOR_BG_BLOCKED);
    }
    else
    {
        printf("%s%s", COLOR_FG, COLOR_BG);
    }
    __shell_percentage__printf_new('#', complete_size);
    printf("%s%s", RESTORE_FG, RESTORE_BG);
    __shell_percentage__printf_new('.', remainder_size);
    printf("]");
}

static void __shell_percentage__printf_new(char c, int count)
{
    for (int i = 0; i < count; i ++)
    {
        printf("%c", c);
    }
}

static void __shell_percentage__draw_progress_bar(int percentage)
{
    int lines = __shell_percentage__tput_lines();
    if (lines != __shell_percentage__CURRENT_NR_LINES)
    {
        __shell_percentage__setup_scroll_area();
    }
    printf("%s", CODE_SAVE_CURSOR);
    printf("\033[%d;0f", lines);
    if (system("tput el")) {}
    __shell_percentage__PROGRESS_BLOCKED=false;
    __shell_percentage__print_bar_text(percentage);
    printf("%s", CODE_RESTORE_CURSOR);
}

static void __shell_percentage__setup_scroll_area(void)
{
    int lines = __shell_percentage__tput_lines();
    __shell_percentage__CURRENT_NR_LINES = lines;
    lines--;
    printf("\n");
    printf("%s", CODE_SAVE_CURSOR);
    printf("\033[0;%dr", lines);

    printf("%s", CODE_RESTORE_CURSOR);
    printf("%s", CODE_CURSOR_IN_SCROLL_AREA);

    __shell_percentage__draw_progress_bar(0);
}



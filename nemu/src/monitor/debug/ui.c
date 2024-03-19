#include "monitor/monitor.h"
#include "monitor/expr.h"
#include "monitor/watchpoint.h"
#include "nemu.h"

#include <stdlib.h>
#include <readline/readline.h>
#include <readline/history.h>

void cpu_exec(uint64_t);

/* We use the `readline' library to provide more flexibility to read from stdin. */
char* rl_gets() {
  static char *line_read = NULL;

  if (line_read) {
    free(line_read);
    line_read = NULL;
  }

  line_read = readline("(nemu) ");

  if (line_read && *line_read) {
    add_history(line_read);
  }

  return line_read;
}

static int cmd_c(char *args) {
  cpu_exec(-1);
  return 0;
}

static int cmd_q(char *args) {
  return -1;
}

static int cmd_help(char *args);

static int cmd_si(char *args)
{
	int n=1;
	if(args!=NULL)
	{
		sscanf(args,"%d",&n);
		if(n<=0)
		{
			printf("args<=0");
			return 0;
		}
	}
	cpu_exec(n);
	return 0;
}

static int cmd_info(char *args)
{
	if(*args=='r')
	{
		for(int i=R_EAX;i<=R_EDI;i++)
		{
			printf("%s\t0x%08x\n",regsl[i],reg_l(i));
		}
		printf("eip\t0x%08x\n",cpu.eip);
	}

	return 0;
}

static int cmd_x(char *args)
{
	int n;
	uint32_t expr;
	sscanf(args,"%d 0x%x",&n,&expr);
	for(int i=0;i<n;i++)
	{
		printf("0x%08x\n",vaddr_read(expr+i*4,4));
	}
	return 0;
}

static int cmd_p(char *args)
{
	bool success;
	int re=expr(args,&success);
	if(success)
	{
		printf("expr=%d\n",re);
	}
	else
	{
		printf("make token fail\n");
	}

	return 0;
}

static int cmd_w(char* args){
	new_wp(args);
	return 0;
}

static int cmd_d(char* args){
	int num=0;
	int nRet=sscanf(args,"%d",&num);
	if(nRet<=0){
		printf("args error in cmd_si\n");
		return 0;
	}
	int r=free_wp(num);
	if(r==false){
		printf("error: no watchpoint %d\n",num);
	}
	else{
		printf("success delete watchpoint %d\n",num);
	}
	return 0;
}
static struct {
  char *name;
  char *description;
  int (*handler) (char *);
} cmd_table [] = {
  { "help", "Display informations about all supported commands", cmd_help },
  { "c", "Continue the execution of the program", cmd_c },
  { "q", "Exit NEMU", cmd_q },

  /* TODO: Add more commands */

  { "si","One-step exec",cmd_si},
  { "info","Print program state",cmd_info},
  { "x","Print memory",cmd_x},
  { "p","expr",cmd_p},
  { "w","set wp",cmd_w},
  { "d","delete wp",cmd_d}
};

#define NR_CMD (sizeof(cmd_table) / sizeof(cmd_table[0]))

static int cmd_help(char *args) {
  /* extract the first argument */
  char *arg = strtok(NULL, " ");
  int i;

  if (arg == NULL) {
    /* no argument given */
    for (i = 0; i < NR_CMD; i ++) {
      printf("%s - %s\n", cmd_table[i].name, cmd_table[i].description);
    }
  }
  else {
    for (i = 0; i < NR_CMD; i ++) {
      if (strcmp(arg, cmd_table[i].name) == 0) {
        printf("%s - %s\n", cmd_table[i].name, cmd_table[i].description);
        return 0;
      }
    }
    printf("Unknown command '%s'\n", arg);
  }
  return 0;
}

void ui_mainloop(int is_batch_mode) {
  if (is_batch_mode) {
    cmd_c(NULL);
    return;
  }

  while (1) {
    char *str = rl_gets();
    char *str_end = str + strlen(str);

    /* extract the first token as the command */
    char *cmd = strtok(str, " ");
    if (cmd == NULL) { continue; }

    /* treat the remaining string as the arguments,
     * which may need further parsing
     */
    char *args = cmd + strlen(cmd) + 1;
    if (args >= str_end) {
      args = NULL;
    }

#ifdef HAS_IOE
    extern void sdl_clear_event_queue(void);
    sdl_clear_event_queue();
#endif

    int i;
    for (i = 0; i < NR_CMD; i ++) {
      if (strcmp(cmd, cmd_table[i].name) == 0) {
        if (cmd_table[i].handler(args) < 0) { return; }
        break;
      }
    }

    if (i == NR_CMD) { printf("Unknown command '%s'\n", cmd); }
  }
}

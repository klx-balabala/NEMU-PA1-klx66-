#include "monitor/monitor.h"
#include "monitor/expr.h"
#include "monitor/watchpoint.h"
#include "nemu.h"

#include <stdlib.h>
#include <readline/readline.h>
#include <readline/history.h>

void cpu_exec(uint32_t);

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

static int cmd_si(char *args);

static int cmd_info(char *args);

static int cmd_x(char *args);

static int cmd_p(char *args);

static int cmd_w(char *args);

static int cmd_d(char *args);

static struct {
	char *name;
	char *description;
	int (*handler) (char *);
} cmd_table [] = {
	{ "help", "Display informations about all supported commands", cmd_help },
	{ "c", "Continue the execution of the program", cmd_c },
	{ "q", "Exit NEMU", cmd_q },
	{ "si", "Run for n times", cmd_si },
	/* TODO: Add more commands */
	{ "info", "Output all value", cmd_info },
	{ "x", "Scan memory", cmd_x },
	{ "p", "Expression evalution", cmd_p },
	{ "w", "Print out infomation in the watchpoints", cmd_w },
	{ "d", "Delete the watchpoint", cmd_d},
};

#define NR_CMD (sizeof(cmd_table) / sizeof(cmd_table[0]))

static int cmd_help(char *args) {
	/* extract the first argument */
	char *arg = strtok(NULL, " ");
	int i;

	if(arg == NULL) {
		/* no argument given */
		for(i = 0; i < NR_CMD; i ++) {
			printf("%s - %s\n", cmd_table[i].name, cmd_table[i].description);
		}
	}
	else {
		for(i = 0; i < NR_CMD; i ++) {
			if(strcmp(arg, cmd_table[i].name) == 0) {
				printf("%s - %s\n", cmd_table[i].name, cmd_table[i].description);
				return 0;
			}
		}
		printf("Unknown command '%s'\n", arg);
	}
	return 0;
}

static int cmd_si(char *args) {
	char *arg = strtok(args, " ");
//	arg = strtok(NULL, " ");
	int klx = 1;
	if(arg != NULL)
	{
	sscanf(arg, "%d", &klx);
	if(klx < -1)
	{
	printf("Parameter Error\n");
	return 0;
	}
	}
	cpu_exec(klx);
	return 0;	
}

static int cmd_info(char *args)
{
	char *arg = strtok(args, " ");
	if(*arg == 'r')
	{
	int i;
	for(i = R_EAX; i <= R_EDI; i ++)
	{
	printf("%s: 0x%08x %d\n", regsl[i], cpu.gpr[i]._32, cpu.gpr[i]._32);
	}
	printf("\n");
	
	for(i = R_AX; i <= R_DI; i ++)
	{
	printf("%s: 0x%08x %d\n", regsw[i], cpu.gpr[i]._16, cpu.gpr[i]._16);
	}
	printf("\n");

	/*for(i = R_AL; i <= R_BH; i ++)
	{
	printf("%s: 0x%04x %d\n", regsb[i], cpu.gpr[i]._8[0], cpu.gpr[i]._8[0]);
	}
	printf("\n");

	for(i = R_AL; i <= R_BH; i ++)
	{
	printf("%s: 0x%04x %d\n", regsb[i], cpu.gpr[i]._8[1], cpu.gpr[i]._8[1]);
	}
	printf("\n");
	*/
	
	printf("eip: 0x%08x %d\n", cpu.eip, cpu.eip);
	return 0;
	}
	else if(*arg == 'w')
	{
	info_wp();
	return 0;
	}
	return 0;
}

static int cmd_x(char *args)
{
	char *arg = strtok(args, " ");
	char *arg1 = strtok(NULL, "");
	int klx = 1;
	sscanf(arg, "%d", &klx);
	bool succe = true;
	swaddr_t address = expr(arg1, &succe);
	int i;
	printf("0x%x:", address);
	for(i = 0; i < klx; i ++)
	{	
	printf("%08x ", swaddr_read(address, 4));
	address += 4;	
	}
	printf("\n");
	return 0;
}

static int cmd_p(char *args)
{
	uint32_t num;
	bool success;
	num = expr(args, &success);
	if(success)
	printf("%d\n", num);
	else
	assert(0);
	return 0;
}

static int cmd_w(char *args)
{
	WP *f;
	bool succ;
	f = new_wp();
	printf("Watchpoint %d: %s\n", f->NO, args);
	strcpy(f->EXp, args);
	f->Val = expr(args, &succ);
	if(!succ)
	Assert(1, "That is not right!\n");
	printf("The value is: %d\n", f->Val);
	return 0;
}

static int cmd_d(char *args)
{
	int klx;
	char *arg = strtok(args, " ");
	sscanf(arg, "%d", &klx);
	delete_wp(klx);
	return 0;
}

void ui_mainloop() {
	while(1) {
		char *str = rl_gets();
		char *str_end = str + strlen(str);

		/* extract the first token as the command */
		char *cmd = strtok(str, " ");
		if(cmd == NULL) { continue; }

		/* treat the remaining string as the arguments,
		 * which may need further parsing
		 */
		char *args = cmd + strlen(cmd) + 1;
		if(args >= str_end) {
			args = NULL;
		}

#ifdef HAS_DEVICE
		extern void sdl_clear_event_queue(void);
		sdl_clear_event_queue();
#endif

		int i;
		for(i = 0; i < NR_CMD; i ++) {
			if(strcmp(cmd, cmd_table[i].name) == 0) {
				if(cmd_table[i].handler(args) < 0) { return; }
				break;
			}
		}

		if(i == NR_CMD) { printf("Unknown command '%s'\n", cmd); }
	}
}

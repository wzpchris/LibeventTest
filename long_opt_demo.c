#include <stdio.h>
#include <getopt.h>

int do_name, do_gf_name;
char *l_opt_arg;

struct option longopts[] = {
	{"name", no_argument, &do_name, 1},
	{"gf_name", no_argument, &do_gf_name, 1},
	{"love", required_argument, NULL, 'l'},
	{0, 0, 0, 0},
};

int main(int argc, char **argv)
{
	int c;
	while((c = getopt_long(argc, argv, ":l:", longopts, NULL)) != -1) {
		switch(c) {
			case 'l':
				l_opt_arg = optarg;
				printf("Our love is %s!\n", l_opt_arg);
				break;
			case 0:
				printf("getopt_long设置变量:do_name=%d\n", do_name);
				printf("getopt_long设置变量:do_gf_name=%d\n", do_gf_name);
				break;
		}
	}

	return 0;
}

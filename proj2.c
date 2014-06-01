/* Memory manager
 *
 * Skeleton program written by Ben Rubinstein, May 2014
 *
 * Modifications by Daniel Benjamin Masters (583334), May 2014
 * algorithms are fun
 */

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <assert.h>

#define TOTALMEM	1048576
#define MAXVARS		1024
#define ERROR		-1
#define SUCCESS		1
#define LINELEN		5000
#define MAXLINES	100000
#define INPUT_INTS	'd'
#define INPUT_CHARS	'c'
#define FREE_DATA	'f'
#define INT_DELIM_S	","
#define INT_DELIM_C	','
#define TRUE 		1
#define FALSE		0

typedef struct {
	char memory[TOTALMEM];		/* TOTALMEM bytes of memory */
	void *null;					/* first address will be  unusable */
	void *vars[MAXVARS];		/* MAXVARS variables, each at an address */
	size_t var_sizes[MAXVARS];	/* number of bytes per variable */
} mmanager_t;

mmanager_t manager;

/****************************************************************/

/* function prototypes */
void mm_free(void *ptr);
int is_vacant(void *first, void *last);
void *select_address(size_t size);
int select_var(void);
void *mm_malloc(size_t size);
void core_dump(char *filename_mem, char *filename_vars, int numCmd,
			   void *stored[], int *storeLen);
int read_line(char *line, int maxlen);
void process_input_char(char *line, char *commands, void *stored[],
                        int *storeLen, int numCommands);
void process_input_int(char *line, char *commands, void *stored[],
                       int *storeLen, int numCommands);
void process_free(char *line, char *commands, void *stored[],
                  int *storeLen, int numCommands);
int count_char(char c, char *s);
int parse_integers(char *str, char *delim, int results[], int max_results);
void print_ints(int *intArray, size_t size);
void print_chars(char *charArray);

/****************************************************************/

/* orchestrate the entire program
 */
int
main(int argc, char *argv[]) {
	char line[LINELEN+1];
	char cmd[MAXLINES];
	void *stored[MAXLINES];
	int storeLen[MAXLINES];
	int i, numCmd = 0;
    
	/* initialise our very own NULL */
	manager.null = manager.memory;
    
	/* process input commands that make use of memory management */
	while (numCmd<MAXLINES && read_line(line,LINELEN)) {
		if (strlen(line) < 2) {
			fprintf(stderr, "Invalid line %s\n", line);
			return EXIT_FAILURE;
		}
		if (line[0] == INPUT_CHARS) {
			process_input_char(line, cmd, stored, storeLen, numCmd);
		} else if (line[0] == INPUT_INTS) {
			process_input_int(line, cmd, stored, storeLen, numCmd);
		} else if (line[0] == FREE_DATA) {
			process_free(line, cmd, stored, storeLen, numCmd);		
		} else {
            fprintf(stderr, "Invalid input %c.\n", line[0]);
            return EXIT_FAILURE;
        }
		numCmd++;
	}
    
	/* print out what we are left with
	 * after creating variables, deleting some, creating more, ...
	 */
	printf("Cmd#\tOffset\tValue\n");
	printf("====\t======\t=====\n");
	for (i=0; i<numCmd; i++) {
		if (storeLen[i] > 0) {
			printf("%d\t%d\t", i, (int)((char*)stored[i]-manager.memory));
			if (cmd[i] == INPUT_CHARS) {
				print_chars((char*)stored[i]);
			} else {
				print_ints((int*)stored[i], storeLen[i]);
			}
		}
	}
	core_dump("core_mem", "core_vars", numCmd, stored, storeLen);
	return 0;
}

/****************************************************************/

/* Deallocates previously allocated memory in the system
 */
void
mm_free(void *ptr){
    if (ptr != NULL){
        ptr = manager.null;
    }
    else return;
}

/****************************************************************/

/* Checks an address range starting from "first" and ending at "last" &
 * returns a logical int indicating whether the range is available or not
 */
int
is_vacant(void *first, void *last){
    char* firstc = (char*)first;
    char* lastc = (char*)last;
    lastc++;
    if (*(firstc-1) != '\0'){
        return FALSE;
    }
    while (firstc != lastc) {
        if(*firstc != '\0'){
            return FALSE;
        }
        firstc++;
    }
    return TRUE;
}

/****************************************************************/

/* Takes the amount of space required & returns a start address pointing
 * to a block of available memory, or manager.null if no memory is available
 */
void *
select_address(size_t size){
	int i = 0, j;    
    for (j=0; manager.vars[j]; j++) {
        i += manager.var_sizes[j];
    }
    for (i=i+1; i < TOTALMEM; i++){
		if(is_vacant(&manager.memory[i], &manager.memory[i+size])) {
			return &manager.memory[i];
		}
	}
	return manager.null;
}

/****************************************************************/

/* Returns the earliest index in manager.vars to an unassigned variable slot
 */
int
select_var(void){
	int i;
	for (i=0; i < MAXVARS; i++) {
		if(manager.vars[i]!=manager.null){
			return i;
		}
	}
	return ERROR;
}

/****************************************************************/

/* Returns a valid address within manager.memory that points to an available
 * block of size bytes & tracks this allocated memory. If no block is
 * available, manager.null is returned.
 */
void *
mm_malloc(size_t size){
	void *address = select_address(size);
	if (address == manager.null){
		return manager.null;
	} else {
		int var = select_var();
		manager.vars[var] = address;
		manager.var_sizes[var] = size;
		return address;
	}
}

/****************************************************************/

/* Writes the contents of manager.memory to a binary file & the integer
 * offset of each allocated variable's address, as well as its size.
 */
void
core_dump(char *filename_mem, char *filename_vars, int numCmd, void *stored[],
		  int storeLen[]){
	int i = 0;
	FILE *fp_mem, *fp_vars;
	fp_mem = fopen(filename_mem, "w");
	fwrite(manager.memory, 1, TOTALMEM, fp_mem);
	fclose(fp_mem);
	fp_vars = fopen(filename_vars, "w");
	for (i=0; i<numCmd; i++) {
		fprintf(fp_vars, "%d 	%d\n", (int)((char*)stored[i]-manager.memory),
				storeLen[i]);
	}
	fclose(fp_vars);
}

/****************************************************************/

/* read in a line of input
 */
int
read_line(char *line, int maxlen) {
	int n = 0;
	int oversize = 0;
	int c;
	while (((c=getchar())!=EOF) && (c!='\n')) {
		if (n < maxlen) {
			line[n++] = c;
		}
		else {
			oversize++;
		}
	}
	line[n] = '\0';
	if (oversize > 0) {
		fprintf(stderr, "Warning! %d over limit. Line truncated.\n",
		        oversize);
	}
	return ((n>0) || (c!=EOF));
}

/****************************************************************/

/* process an input-char command from stdin by storing the string
 */
void
process_input_char(char *line, char *commands, void *stored[],
                   int *storeLen, int numCommands) {
	size_t lineLen = strlen(line);
	commands[numCommands] = line[0];
	stored[numCommands] = mm_malloc(lineLen);
	assert(stored[numCommands] != manager.null);
	strcpy(stored[numCommands], line+1);
	storeLen[numCommands] = lineLen;
}

/****************************************************************/

/* process an input-int command from stdin by storing the ints
 */
void
process_input_int(char *line, char *commands, void *stored[],
                  int *storeLen, int numCommands) {
	int intsLen = count_char(INT_DELIM_C, line+1) + 1;
	char *arr;
	size_t size = sizeof(intsLen) * (1+intsLen);
	commands[numCommands] = line[0];
	stored[numCommands] = mm_malloc(size);
	assert(stored[numCommands] != manager.null);
	parse_integers(line+1, INT_DELIM_S, stored[numCommands], intsLen);
    arr = (char*)stored[numCommands];
    arr[size-1] = '\0';
	storeLen[numCommands] = intsLen;
}

/****************************************************************/

/* process a free command from stdin
 */
void
process_free(char *line, char *commands, void *stored[],
             int *storeLen, int numCommands) {
	int i, size, freearg;
	void **address;
    
	freearg = atoi(line+1);
	commands[numCommands] = line[0];
	size = storeLen[freearg-1];
	address = &stored[freearg-1];
    
	/* Free memory from the start of the block pointed to by "address",
	 * till it reaches the end of it, as indicated by "size" */
	for (i = 0; i < size; i++){
		mm_free(address+i);
	}
    
	/* Zeroing out local variables by setting to NULL or 0 (as shown in FAQ) */
	mm_free(stored[freearg-1]);
	mm_free(stored[freearg+1]);
	storeLen[freearg-1] = 0;
	storeLen[freearg+1] = 0;
    
	/* Stop tracking the address by setting to 0 (as shown in FAQ) */
	manager.vars[freearg-1] = 0;
	manager.var_sizes[freearg-1] = 0;
}

/****************************************************************/

/* Count the number of occurences of a char in a string
 */
int
count_char(char c, char *s) {
	int count = 0;
	if (!s) {
		return count;
	}
	while (*s != '\0') {
		if (*(s++) == c) {
			count++;
		}
	}
	return count;
}

/****************************************************************/

/* parse string for a delimited-list of positive integers.
 * Returns number of ints parsed. If invalid input is detected
 * or if more than max_results ints are parsed, execution
 * will halt. Note! str will be modified as a side-effect of
 * running this function: delims replaced by \0
 */
int
parse_integers(char *str, char *delim, int results[], int max_results) {
	int num_results = 0;
	int num;
	char *token;
	token = strtok(str, delim);
	while (token != NULL) {
		if ((num=atoi(token)) <= 0) {
			fprintf(stderr, "Non-int %s.\n", token);
			exit(EXIT_FAILURE);
		}
		if (num_results >= max_results) {
			fprintf(stderr, "Parsing too many ints.\n");
			exit(EXIT_FAILURE);
		}
		results[num_results++] = num;
		token = strtok(NULL, delim);
	}
	return num_results;
}

/****************************************************************/

/* print an array of ints
 */
void
print_ints(int *intArray, size_t size) {
	int i;
	assert(size > 0);
	printf("ints: %d", intArray[0]);
	for (i=1; i<size; i++) {
		printf(", %d", intArray[i]);
	}
	putchar('\n');
}

/****************************************************************/

/* print an array of chars
 */
void
print_chars(char *charArray) {
	printf("chars: %s\n", charArray);
}

/****************************************************************/

/*  
   ___   __              _ __  __                              ___          __
  / _ | / /__ ____  ____(_) /_/ /  __ _  ___   ___ ________   / _/_ _____  / /
 / __ |/ / _ `/ _ \/ __/ / __/ _ \/  ' \(_-<  / _ `/ __/ -_) / _/ // / _ \/_/ 
/_/ |_/_/\_, /\___/_/ /_/\__/_//_/_/_/_/___/  \_,_/_/  \__/ /_/ \_,_/_//_(_)  
        /___/                                                                 

*/

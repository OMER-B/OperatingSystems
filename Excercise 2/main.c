#include <stdio.h>
#include <zconf.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <string.h>

/* Util declarations */
#define BUFFER_SIZE 32
#define MAX_JOBS 512
#define PROMPT "> "
#define DELIMITER " "

typedef enum state { foreground, background } state;
typedef enum bool { false, true } bool;

char **parse_line(char *);
char *get_line();

/* Jobs handling */
typedef struct job_t {
    pid_t pid;      /* PID of the job */
    int jid;        /* ID of the job */
    state state;    /* state of the job */
    char **cmd;     /* command to print */
};

void kill_all(struct job_t *);
void remove_job(struct job_t *, pid_t);
void add_job(struct job_t *, pid_t, int, state, char **);
struct job_t *job_by_pid(struct job_t *, pid_t);

/* Commands */
bool execute(char **, struct job_t *);
bool start(char **, struct job_t *);
bool cd(char **, struct job_t *);
bool help(char **, struct job_t *);
bool shell_exit(char **, struct job_t *);
bool list_jobs(char **, struct job_t *);
state check_ampersand(char **);

/* Implementations */
/**
 * Main
 * @return 0
 */
int main() {
    bool status = true;
    char *line;
    char **input;
    struct job_t *jobs = (struct job_t *) calloc(MAX_JOBS, sizeof(struct job_t));
    if (!jobs) {
        printf("Allocation failure.\n");
        exit(0);
    }
    bzero(jobs, MAX_JOBS * sizeof(struct job_t *)); /* Initalize to all-zeros */

    while (status == true) {
        printf(PROMPT);
        line = get_line();
        input = parse_line(line);

        status = execute(input, jobs);

//        free(line);
//        free(input);
    }
    return 0;
}

/**
 * Gets line input from user.
 * @return line input of user.
 */
char *get_line() {
    size_t buffer_size = BUFFER_SIZE;
    int position = 0, c;
    char *line = (char *) calloc(buffer_size, sizeof(char)); /* First allocation for MAX_SIZE */

    if (!line) {
        printf("Allocation failure.\n");
        exit(0);
    }

    while (1) {
        c = getchar();
        if (c == '\n') {
            line[position] = '\0';
            return line;
        } else {
            line[position] = (char) c;
        }
        position++;
        if (position >= buffer_size) { /* Reached end of buffer, need to allocate more */
            buffer_size += BUFFER_SIZE;
            line = (char *) realloc(line, buffer_size * sizeof(char));
            if (!line) {
                printf("Allocation failure.\n");
                exit(0);
            }
        }
    }
}

/**
 * Separates line input to array of strings.
 * @param line input from user.
 * @return array of strings.
 */
char **parse_line(char *line) {
    size_t buffer_size = BUFFER_SIZE;
    int position = 0;
    char *token;
    char **tokens = (char **) calloc(buffer_size, sizeof(char *));
    if (!tokens) {
        printf("Allocation failure.\n");
        exit(0);
    }

    token = strtok(line, DELIMITER);
    while (token != NULL) {
        tokens[position] = token;
        position++;
        if (position >= buffer_size) {
            buffer_size += BUFFER_SIZE;
            tokens = (char **) realloc(tokens, buffer_size * sizeof(char));
            if (!tokens) {
                printf("Allocation failure.\n");
                exit(0);
            }
        }
        token = strtok(NULL, DELIMITER);
    }
    return tokens;
}

/**
 * Function pointer for cleaner code.
 * @param input array of command arguments.
 * @param jobs array of jobs.
 * @return true if success, false if fail.
 */
bool (*func[])(char **input, struct job_t *jobs) = {
        &cd,
        &help,
        &list_jobs,
        &shell_exit
};

/**
 * Executes the custom functions and calls start to execute execvp.
 * @param input array of command arguments.
 * @param jobs array of jobs.
 * @return true if success, false if fail.
 */
bool execute(char **input, struct job_t *jobs) {
    if (input[0] == NULL) { return true; } /* Empty command. ask for another. */
    int i, size;
    char *commands[] = {"cd", "help", "jobs", "exit"};
    size = sizeof(commands) / sizeof(char *);
    for (i = 0; i < size; i++) {
        if (!strcmp(input[0], commands[i])) {
            return (*func[i])(input, jobs);
        }
    }
    return start(input, jobs);
}

/**
 * Checks if command should be foreground of background by ampersand.
 * @param input array of command arguments.
 * @return background if command has ampersand, foreground otherwise.
 */
inline state check_ampersand(char **input) {
    register int i = 0;
    for (; input[i] != NULL; i++); /* Find last cell of the array to check if it is ampersand */
    return !strcmp(input[i - 1], "&") ? background : foreground;
}

/**
 * Forks and executes a command with execvp.
 * @param input array of command arguments.
 * @param jobs array of jobs.
 * @return true if success, false if fail.
 */
bool start(char **input, struct job_t *jobs) {
    register int i = 0;
    pid_t pid;
    state ampersand;
    ampersand = check_ampersand(input);
    if (ampersand == background) { /* Replace '&' with '\0' */
        for (; input[i] != NULL; i++);
        input[i - 1] = '\0';
    }

    pid = fork();

    if (pid > 0) { /* Forking successful, pid is child id. */
        printf("%d\n", pid);
        add_job(jobs, pid, 0, ampersand, input);
    } else if (pid == 0) {
        execvp(input[0], input);
//        remove_job(jobs, pid); //TODO find correct place to remove job
        fprintf(stderr, "Execution error\n");
    } else if (pid < 0) {
        perror("Forking unsuccessful\n");
        return false;
    }
    if (ampersand == foreground && pid != 0) {
        waitpid(pid, NULL, 0);
        remove_job(jobs, pid); //TODO find correct place to remove job (this place is good for foregrounds)
    }
    return true;
}

/**
 * Change directory command.
 * @param input array of command arguments.
 * @param jobs array of jobs.
 * @return true if success, false if fail.
 */
bool cd(char **args, struct job_t *jobs) {
    if (args[1] == NULL) {
        chdir(getenv("home"));
        return true;
    } else {
        if (chdir(args[1]) == -1) {
            printf("%s: No such directory.\n", args[1]);
            return true;
        }
    }
    return true;
}

/**
 * Help command.
 * @param input array of command arguments.
 * @param jobs array of jobs.
 * @return true if success, false if fail.
 */
bool help(char **args, struct job_t *jobs) {
    printf("***************************************************\n"
                   "***************************************************\n"
                   "****                                           ****\n"
                   "****                OMER BARAK                 ****\n"
                   "****                                           ****\n"
                   "****             Operating Systems             ****\n"
                   "****                Exercise 2                 ****\n"
                   "****                                           ****\n"
                   "***************************************************\n"
                   "***************************************************\n");
    return true;
}

/**
 * Exit command.
 * @param input array of command arguments.
 * @param jobs array of jobs.
 * @return true if success, false if fail.
 */
bool shell_exit(char **args, struct job_t *jobs) {
//    kill_all(jobs);
    return false;
}

/**
 * Jobs command.
 * @param input array of command arguments.
 * @param jobs array of jobs.
 * @return true if success, false if fail.
 */
bool list_jobs(char **args, struct job_t *jobs) {
    register int i;
    for (i = 0; i < MAX_JOBS; i++) {
        if (jobs[i].pid != 0) {
            printf("%d\t\t%s\n", jobs[i].pid, jobs[i].cmd[0]);
            //TODO currently only print the name of the command without arguments
        }
    }
    return true;
}

/**
 * Adds a job to the list of jobs.
 * @param jobs array of jobs.
 * @param pid pid of the job.
 * @param jid jid of the job.
 * @param state state of the job, foreground or background.
 * @param cmd commandline argument of the job.
 */
void add_job(struct job_t *jobs, pid_t pid, int jid, state state, char **cmd) {
    int i;
    register int j;
    size_t size = 0;
    for (i = 0; i < MAX_JOBS; i++) {
        if (jobs[i].pid == 0) {
            jobs[i].pid = pid;
            jobs[i].jid = jid;
            jobs[i].state = state;
            for (j = 0; cmd[j] != NULL; j++) {
                for (int k = 0; cmd[j][k] != '\0'; k++) {
                    size++; /* Find how much to allocate */
                }
            }
            jobs[i].cmd = calloc(size, sizeof(char));
            memcpy(jobs[i].cmd, cmd, sizeof(char) * size);
            break;
        }
    }
}

/**
 * Removes job from the list by pid.
 * @param jobs array of jobs.
 * @param pid pid of job to remove.
 */
void remove_job(struct job_t *jobs, pid_t pid) {
    register int i;
    for (i = 0; i < MAX_JOBS; i++) {
        if (jobs[i].pid == pid) {
            free(jobs[i].cmd);
            memset(&jobs[i], '\0', sizeof(struct job_t));
        }
    }
}

/**
 * Kills all jobs
 * @param jobs array of jobs to kill.
 */
void kill_all(struct job_t *jobs) {
    register int i;
    for (i = 0; i < MAX_JOBS; i++) {
        if (jobs[i].cmd != NULL) {
            free(jobs[i].cmd);
        }
        kill(jobs[i].pid, SIGKILL);
    }
}

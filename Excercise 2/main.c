#include <stdio.h>
#include <zconf.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <string.h>

/* Util declarations */
#define BUFFER_SIZE 32
#define MAX_JOBS 512
#define PROMPT "omer@\"\"\"SHELL\"\"\"~$ "
#define DELIMITER " "

char **parse_line(char *);
char *get_line();

typedef enum STATE {
    FOREGROUND, BACKGROUND, STOPPED
} STATE;

typedef enum STATUS {
    FALSE, TRUE
} BOOL;

/* Jobs handling */
typedef struct job_t {
    pid_t pid;      /* PID of the job */
    int jid;        /* ID of the job */
    STATE state;    /* state of the job */
    char *cmd;      /* command to print */
} job_t;

void kill_all(struct job_t *jobs);
void remove_job(struct job_t *, pid_t);
void add_job(struct job_t *, pid_t, int, STATE, char *);
struct job_t *job_by_pid(struct job_t *, pid_t);

/* Commands */
BOOL execute(char **, struct job_t *);
BOOL start_foreground(char **, struct job_t *);
BOOL start_background(char **, job_t *);
BOOL cd(char **, struct job_t *);
BOOL help(char **, struct job_t *);
BOOL shell_exit(char **, struct job_t *);
BOOL list_jobs(char **, struct job_t *);
BOOL check_ampersand(char **);

/* Implementations */
int main() {
    BOOL status = TRUE;
    char *line;
    char **input;
    job_t jobs[MAX_JOBS];
    bzero(jobs, MAX_JOBS * sizeof(job_t)); /* Initalize to all-zeros */

    while (status == TRUE) {
        printf(PROMPT);
        line = get_line();
        input = parse_line(line);

        status = execute(input, jobs);

        free(line);
        free(input);
    }
    kill_all(jobs);
    return 0;
}

char *get_line() {
    size_t buffer_size = BUFFER_SIZE;
    int position = 0, c;
    char *line = malloc(sizeof(char) * buffer_size);

    // First allocation for line size
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
            line[position] = c;
        }
        position++;

        if (position >= buffer_size) { // Reached end of buffer, need to allocate more
            buffer_size += BUFFER_SIZE;
            line = realloc(line, buffer_size * sizeof(char));

            if (!line) {
                printf("Allocation failure.\n");
                exit(0);
            }
        }
    }
}

char **parse_line(char *line) {
    size_t buffer_size = BUFFER_SIZE;
    int position = 0;
    char **tokens = malloc(sizeof(char *) * buffer_size);
    char *token;

    if (!tokens) {
        printf("Allocation failure.\n");
        exit(0);
    }

    token = strtok(line, delimiter);
    while (token != NULL) {
        tokens[position] = token;
        position++;
        if (position >= buffer_size) {
            buffer_size += BUFFER_SIZE;
            tokens = realloc(tokens, buffer_size * sizeof(char));

            if (!tokens) {
                printf("Allocation failure.\n");
                exit(0);
            }

        }
        token = strtok(NULL, DELIMITER);
    }
    return tokens;
}

BOOL (*func[])(char **input, job_t *jobs) = {
        &cd,
        &help,
        &list_jobs,
        &shell_exit
};

BOOL execute(char **input, job_t *jobs) {
    if (input[0] == NULL) { /* Empty command. ask for another. */
        return TRUE;
    }

    int i, size;
    BOOL ampersand;

    char *commands[] = {
            "cd",
            "help",
            "jobs",
            "exit"
    };
    size = sizeof(commands) / sizeof(char *);

    ampersand = check_ampersand(input);

    for (i = 0; i < size; i++) {
        if (!strcmp(input[0], commands[i])) {
            return (*func[i])(input, jobs);
        }
    }
    return (ampersand == TRUE ? start_background : start_foreground)(input, jobs);
}


inline BOOL check_ampersand(char **input) {
    int i = 0;
    while (input[i] != NULL, i++);
    return !strcmp(input[--i], "&") ? TRUE : FALSE;
}

BOOL start_background(char **input, job_t *jobs) {
    int status = 0;
    pid_t pid, wpid;

    pid = fork();

    if (pid == 0) {
        if (execvp(input[0], input) == -1) {
            perror("ERROR");
        }
        return FALSE;
    } else if (pid < 0) {
        perror("ERROR\n");
    } else {
        do {
            add_job(jobs, pid, 0, BACKGROUND, input[0]);
            wpid = waitpid(pid, &status, WNOHANG);
            printf("%d\n", getpid());
//            remove_job(jobs, pid);
        } while (!WIFEXITED(status) && !WIFSIGNALED(status));

    }
    return TRUE;
}

BOOL start_foreground(char **input, job_t *jobs) {
    pid_t pid, wpid;
    int status = 0;

    pid = fork();

    if (pid == 0) {
        if (execvp(input[0], input) == -1) {
            perror("ERROR");
        }
        return FALSE;
    } else if (pid < 0) {
        perror("ERROR\n");
    } else {
        printf("%d\n", getpid());
        do {
            add_job(jobs, pid, 0, FOREGROUND, input[0]);
            wpid = waitpid(pid, &status, WUNTRACED);
            remove_job(jobs, pid);
        } while (!WIFEXITED(status) && !WIFSIGNALED(status));

    }
    return TRUE;
}

void add_job(struct job_t *jobs, pid_t pid, int jid, STATE state, char *cmd) {
    int i;
    for (i = 0; i < MAX_JOBS; i++) {
        if (jobs[i].pid == 0) {
            jobs[i].pid = pid;
            jobs[i].jid = jid;
            jobs[i].state = state;
            jobs[i].cmd = (char *) calloc(29, sizeof(char));
            strcpy(jobs[i].cmd, cmd);
            break;
        }
    }
}

BOOL cd(char **args, struct job_t *jobs) {
    if (args[1] == NULL) {
        printf("to which folder?\n");
    } else {
        if (chdir(args[1]) != 0) {
            return FALSE;
        }
    }
    return TRUE;
}

BOOL help(char **args, struct job_t *jobs) {
    printf("no help.\n");
    return TRUE;
}

BOOL shell_exit(char **args, struct job_t *jobs) {
    return FALSE;
}

BOOL list_jobs(char **args, struct job_t *jobs) {
    int i = 0;
    for (i; i < MAX_JOBS; i++) {
        if (jobs[i].cmd == NULL) {
            break;
        }
        printf("%s\n", jobs[i].cmd);
    }
    return TRUE;
}

void remove_job(struct job_t *jobs, pid_t pid) {
    int i;
    for (i = 0; i < MAX_JOBS; i++) {
        if (jobs[i].pid == pid) {
            free(jobs[i].cmd);
            memset(&jobs[i], '\0', sizeof(job_t));
        }
    }
}

void kill_all(struct job_t *jobs) {
}

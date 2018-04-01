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

typedef enum state {
    fg, bg, STOPPED
} state;

typedef enum bool {
    false, true
} bool;

char **parse_line(char *);
char *get_line();

/* Jobs handling */
typedef struct job_t {
    pid_t pid;      /* PID of the job */
    int jid;        /* ID of the job */
    state state;    /* state of the job */
    char **cmd;      /* command to print */
} job_t;

void kill_all(struct job_t **jobs);
void remove_job(struct job_t **, pid_t);
void add_job(struct job_t **, pid_t, int, state, char **);
struct job_t *job_by_pid(struct job_t *, pid_t);

/* Commands */
bool execute(char **, struct job_t **);
bool start(char **, struct job_t **);
bool cd(char **, struct job_t **);
bool help(char **, struct job_t **);
bool shell_exit(char **, struct job_t **);
bool list_jobs(char **, struct job_t **);
bool check_ampersand(char **);

/* Implementations */
int main() {
    bool status = true;
    char *line;
    char **input;
    job_t **jobs = (void *) calloc(MAX_JOBS, sizeof(struct job_t));
    bzero(jobs, MAX_JOBS * sizeof(job_t *)); /* Initalize to all-zeros */

    while (status == true) {
        printf(PROMPT);
        line = get_line();
        input = parse_line(line);

        status = execute(input, jobs);

        free(line);
        free(input);
    }
    return 0;
}

char *get_line() {
    size_t buffer_size = BUFFER_SIZE;
    int position = 0, c;
    char *line = calloc(sizeof(char), buffer_size);

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
    char **tokens = calloc(sizeof(char *), buffer_size);
    char *token;

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

bool (*func[])(char **input, job_t **jobs) = {
        &cd,
        &help,
        &list_jobs,
        &shell_exit
};

bool execute(char **input, job_t **jobs) {
    if (input[0] == NULL) { return true; } /* Empty command. ask for another. */

    int i, size;
    bool ampersand;

    char *commands[] = {"cd", "help", "jobs", "exit"};
    size = sizeof(commands) / sizeof(char *);

    for (i = 0; i < size; i++) {
        if (!strcmp(input[0], commands[i])) {
            return (*func[i])(input, jobs);
        }
    }

    return start(input, jobs);
}

inline bool check_ampersand(char **input) {
    int i = 0;
    for (; input[i] != NULL; i++);
    return !strcmp(input[i - 1], "&") ? true : false;
}

bool start(char **input, job_t **jobs) {
    int status = 0;
    pid_t pid;
    bool ampersand;

    ampersand = check_ampersand(input);
    if (ampersand == true) {
        int j = 0;
        for (; input[j] != NULL; j++);
        input[j - 1] = '\0';
    }

    pid = fork();

    if (pid > 0) { /* Forking successful, pid is child id. */
        printf("%d\n", pid);
    } else if (pid == 0) { /* */ //TODO what is here? failure?
        execvp(input[0], input);
        fprintf(stderr, "execution error\n");
    } else if (pid < 0) {
        perror("Forking unsuccessful\n");
        return false;
    }
    if (ampersand == false && pid != 0) {
        waitpid(pid, NULL, 0);
    }
    return true;
}

void add_job(struct job_t **jobs, pid_t pid, int jid, state state, char **cmd) {
    int i = 0, j = 0;
    size_t size = 0;
    for (i = 0; i < MAX_JOBS; i++) {
        if (jobs[i] == NULL) {
            jobs[i]->pid = pid;
            jobs[i]->jid = jid;
            jobs[i]->state = state;
            for (j = 0; cmd[j] != NULL; j++) {
                for (int k = 0; cmd[j][k] != '\0'; k++) {
                    size++;
                }
            }
            jobs[i]->cmd = calloc(size, sizeof(char));
            memcpy(jobs[i]->cmd, cmd, size * sizeof(char));
            break;
        }
    }
}

bool cd(char **args, struct job_t **jobs) {
    if (args[1] == NULL) {
        printf("to which folder?\n");
    } else {
        if (chdir(args[1]) != 0) {
            return false;
        }
    }
    return true;
}

bool help(char **args, struct job_t **jobs) {
    printf("no help.\n");
    return true;
}

bool shell_exit(char **args, struct job_t **jobs) {
    kill_all(jobs);
    return false;
}

bool list_jobs(char **args, struct job_t **jobs) {
    int i;
    for (i = 0; i < MAX_JOBS; i++) {
        if (jobs[i]->pid != 0) {
            printf("%d\t\t", jobs[i]->pid);
            printf("%s\n", jobs[i]->cmd[0]);
        }
    }
    return true;
}

void remove_job(struct job_t **jobs, pid_t pid) {
    int i;
    for (i = 0; i < MAX_JOBS; i++) {
        if (jobs[i]->pid == pid) {
            free(jobs[i]->cmd);
            memset(&jobs[i], '\0', sizeof(job_t));
        }
    }
}

void kill_all(struct job_t **jobs) {
    for (int i = 0; i < MAX_JOBS; i++) {
        if (jobs[i]->cmd != NULL) {
            free(jobs[i]->cmd);
        }
        kill(jobs[i]->pid, SIGKILL);
    }
}

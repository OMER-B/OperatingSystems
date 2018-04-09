#include <stdio.h>
#include <zconf.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <string.h>

/* Util declarations */
#define MAX_JOBS 512
#define MAX_ARGS 20
#define MAX_ARGS_LEN 512
#define MAX_LINE MAX_ARGS*MAX_ARGS_LEN
#define PROMPT "prompt> "
#define DELIMITER " \""

typedef enum state { foreground, background } state;
typedef enum bool { false, true } bool;

void parse_line(char **, char *);
state check_ampersand(char *);

/* Jobs handling */
struct job_t {
    pid_t pid;               /* PID of the job */
    int jid;                 /* ID of the job */
    state state;             /* state of the job */
    char *cmd[MAX_ARGS];     /* command of the job */
    char line[MAX_LINE];     /* line of the command */
};

struct job_t setup_job(char *);
void kill_all(struct job_t *);
void remove_job(struct job_t *, struct job_t);
void add_job(struct job_t *, struct job_t);

/* Commands */
bool start(struct job_t *, struct job_t);
bool execute(struct job_t *, struct job_t);
bool help(struct job_t *, struct job_t);
bool cd(struct job_t *, struct job_t);
bool list_jobs(struct job_t *, struct job_t);
bool shell_exit(struct job_t *, struct job_t);

/* Implementations */
/**
 * Main
 * @return 0
 */
int main() {
    bool status = true;
    char line[MAX_LINE];
    struct job_t jobs[MAX_JOBS];
    struct job_t job;
    bzero(jobs, MAX_JOBS * sizeof(struct job_t *)); /* Initalize to all-zeros */
    memset(line, '\0', MAX_LINE);
    while (status == true) {
        printf(PROMPT);
        fgets(line, MAX_LINE * sizeof(char), stdin);
        line[strlen(line) - 1] = '\0';
        job = setup_job(line);
        status = execute(jobs, job);
    }
    return 0;
}

/**
 * Set up the job's parameters.
 * @param line intial line to start to job.
 * @return jobs with all fields filled.
 */
struct job_t setup_job(char *line) {
    struct job_t job;
    static int jid = 1;
    int i;
    memset(&job, '\0', sizeof(job));
    memcpy(job.line, line, MAX_LINE);

    parse_line(job.cmd, line);

    job.state = check_ampersand(job.line);
    job.jid = jid++;
    if (job.state == background) {
        for (i = 0; job.cmd[i] != NULL; i++);
        job.cmd[i - 1] = NULL;
        job.line[strlen(job.line) - 1] = '\0';
    }
    return job;
}

/**
 * Separates line input to array of strings.
 * @param cmd where to put the args.
 * @param line input from user.
 */
void parse_line(char **cmd, char *line) {
    int i = 0;
    char *token = strtok(line, DELIMITER);

    while (token != NULL) {
        cmd[i++] = token;
        token = strtok(NULL, DELIMITER);
    }
}

/**
 * Function pointer for cleaner code.
 * @return true if success, false if fail.
 */
bool (*func[])(struct job_t *, struct job_t) = {&cd, &help, &list_jobs, &shell_exit};

/**
 * Executes the custom functions and calls start to execute execvp.
 * @param jobs array of jobs.
 * @param job job to execute.
 * @return true if success, false if fail.
 */
bool execute(struct job_t *jobs, struct job_t job) {
    if (job.cmd[0] == NULL) { return true; } /* Empty command. ask for another. */
    int i;
    size_t size;
    char *commands[] = {"cd", "help", "jobs", "exit"};

    size = sizeof(commands) / sizeof(char *);

    for (i = 0; i < size; i++) {
        if (!strcmp(job.cmd[0], commands[i])) {
            return (*func[i])(jobs, job);
        }
    }
    return start(jobs, job);
}

/**
 * Checks if command should be foreground of background by ampersand.
 * @param line line to check for ampersand.
 * @return background if command has ampersand, foreground otherwise.
 */
inline state check_ampersand(char *line) {
    return line[strlen(line) - 1] == '&' ? background : foreground;
}

/**
 * Forks and executes a command with execvp.
 * @param jobs array of jobs.
 * @param job jobs to start.
 * @return true if success, false if fail.
 */
bool start(struct job_t *jobs, struct job_t job) {
    pid_t pid;
    register int i;

    pid = fork();

    if (pid > 0) { /* Forking successful, pid is child id. */
        printf("%d\n", pid);
        job.pid = pid;
        add_job(jobs, job);
    } else if (pid == 0) {
        execvp(job.cmd[0], job.cmd);
        fprintf(stderr, "Error in system call");

    } else if (pid < 0) {
        perror("Forking unsuccessful.\n");
        return false;
    }
    if (job.state == foreground && pid != 0) {
        waitpid(pid, NULL, 0);
        remove_job(jobs, job);
    }
    return true;
}

/**
 * Change directory command.
 * @param jobs it's not oop so it's ok to force interface (unused).
 * @param job jobs to get the new directory.
 * @return true if success, false if fail.
 */
bool cd(struct job_t *jobs, struct job_t job) {
    printf("%d\n", getpid());
    if (job.cmd[1] == NULL) {
        chdir(getenv("HOME"));
        return true;
    } else {
        if (chdir(job.cmd[1]) == -1) {
            printf("%s: No such directory.\n", job.cmd[1]);
            return true;
        }
    }
    return true;
}

/**
 * Help command.
 * @param jobs it's not oop so it's ok to force interface (unused).
 * @param job it's not oop so it's ok to force interface (unused).
 * @return true if success, false if fail.
 */
bool help(struct job_t *jobs, struct job_t job) {
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
 * @param jobs it's not oop so it's ok to force interface (unused).
 * @param job it's not oop so it's ok to force interface (unused).
 * @return true if success, false if fail.
 */
bool shell_exit(struct job_t *jobs, struct job_t job) {
    kill_all(jobs);
    return false;
}

/**
 * Jobs command.
 * @param jobs array of jobs.
 * @param job it's not oop so it's ok to force interface (unused).
 * @return true if success, false if fail.
 */
bool list_jobs(struct job_t *jobs, struct job_t job) {
    register int i;
    int status;
    for (i = 0; i < MAX_JOBS; i++) {
        if (jobs[i].pid != 0 && waitpid(jobs[i].pid, &status, WNOHANG) == 0) {
            printf("%d %s\n", jobs[i].pid, jobs[i].line);
        } else if (jobs[i].pid != 0 && waitpid(jobs[i].pid, &status, WNOHANG) != 0) {
            remove_job(jobs, jobs[i]);
        }
    }
    return true;
}

/**
 * Adds a job to the list of jobs.
 * @param jobs array of jobs.
 * @param job job to add to the array.
 */
void add_job(struct job_t *jobs, struct job_t job) {
    int i;
    register int j;
    size_t size = 0;
    for (i = 0; i < MAX_JOBS; i++) {
        if (jobs[i].pid == 0) {
            jobs[i] = job;
            break;
        }
    }
}

/**
 * Removes job from the list.
 * @param jobs array of jobs.
 * @param job job to remove.
 */
void remove_job(struct job_t *jobs, struct job_t job) {
    register int i;
    for (i = 0; i < MAX_JOBS; i++) {
        if (jobs[i].pid == job.pid) {
            memset(&jobs[i], '\0', sizeof(struct job_t));
            break;
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
        kill(jobs[i].pid, 0);
    }
}

#include <stdio.h>
#include <string.h>
#include <zconf.h>
#include <stdlib.h>
#include <dirent.h>
#include <wait.h>
#include <fcntl.h>

#define ERROR           -1
#define MAX_LENGTH      160
#define NUM_OF_LINES    3
#define CSV_WRITE_SIZE  200
#define SYS_CALL_ERROR  "Error in system call\n"
#define C_EXTENSION     "c"
#define DOT             '.'
#define COMMA           ","
#define NEW_LINE        "\n"
#define COMPILE_GCC     "gcc"
#define OUT_FILE        "user.out"
#define OUTPUT_FILE     "output.txt"
#define DOT_OUT_FILE    "./user.out"
#define DOT_COMP_FILE   "./comp.out"
#define DOT_OUTPUT_FILE "./output.txt"
#define DOT_RESULT_FILE "./result.csv"

const char
    *reason_arr[] = {"NO_C_FILE", "COMPILATION_ERROR", "TIMEOUT", "BAD_OUTPUT", "SIMILAR_OUTPUT", "GREAT_JOB", "TBD"};
const char *grade_arr[] = {"0", "0", "0", "60", "80", "100", "101"};

typedef enum bool { false, true } bool;
typedef enum reason { NO_C_FILE, COMPILATION_ERROR, TIMEOUT, BAD_OUTPUT, SIMILAR_OUTPUT, GREAT_JOB, TBD } reason;

typedef struct Student {
  char folder_name[MAX_LENGTH];
  char folder_path[MAX_LENGTH];
  char file_name[MAX_LENGTH];
  char file_path[MAX_LENGTH];
  char out_folder[MAX_LENGTH];
  reason reason;
} Student;
typedef struct Config {
  char folders_location[MAX_LENGTH];
  char input_file[MAX_LENGTH];
  char output_file[MAX_LENGTH];
} Config;

void error();
void compile(Student);
void grade(Student *, int, Config);
void parse_config(char *, Config *);
bool check_for_out();
reason run(Config config);
reason compare(Config config);
Student *check_directories(char *, int *);
void save_CSV(Student *, int);

/**
 * Prints error and exits.
 */
void error() {
  write(2, SYS_CALL_ERROR, strlen(SYS_CALL_ERROR));
  exit(ERROR);
}

/**
 * Save the list of students to CSV file with their grade and reason.
 * @param   students          List of all students.
 * @param   num_of_students   Number of students in the list.
 */
void save_CSV(Student *students, int num_of_students) {
  int CSV = open(DOT_RESULT_FILE, O_CREAT | O_WRONLY | O_TRUNC, 0644);
  if (CSV == ERROR)
    error();
  for (int i = 0; i < num_of_students; ++i) {
    char CSV_arr[CSV_WRITE_SIZE] = {};
    strcpy(CSV_arr, students[i].folder_name);
    strcat(CSV_arr, COMMA);
    strcat(CSV_arr, grade_arr[students[i].reason]);
    strcat(CSV_arr, COMMA);
    strcat(CSV_arr, reason_arr[students[i].reason]);
    strcat(CSV_arr, NEW_LINE);
    if (write(CSV, CSV_arr, CSV_WRITE_SIZE) == ERROR)
      error();
  }
  if (close(CSV) == ERROR)
    error();
}

/**
 * Compares two files using the comp.out file, to check if output is good.
 * @param   config  Config struct with correct output.
 * @return  Reason of student's grade.
 */
reason compare(Config config) {
  char *args[] = {DOT_COMP_FILE, config.output_file, DOT_OUTPUT_FILE, NULL};
  int success;
  pid_t pid = fork();
  if (pid == 0) {
    success = execvp(args[0], args);
    if (success == ERROR)
      error();
  } else {
    int value;
    waitpid(pid, &value, 0);
    if (WIFEXITED(value)) {
      switch (WEXITSTATUS(value)) {
      case 1: return BAD_OUTPUT;
      case 2: return SIMILAR_OUTPUT;
      case 3: return GREAT_JOB;
      default: return TBD;
      }
    }
  }
  return TBD;
}

/**
 * Runs the compiled file.
 * @param   config  Config struct with correct input/output.
 * @return  Returns reason of student's grade.
 */
reason run(Config config) {
  char *args[] = {DOT_OUT_FILE, NULL};
  int success;
  reason res;
  int pid = fork();
  if (pid == 0) {
    int out = open(DOT_OUTPUT_FILE, O_CREAT | O_WRONLY | O_TRUNC, 0644);
    if (out == ERROR)
      error();
    int in = open(config.input_file, O_RDONLY);
    if (in == ERROR)
      error();
    success = dup2(in, STDIN_FILENO);
    if (success == ERROR)
      error();
    success = dup2(out, STDOUT_FILENO);
    if (success == ERROR)
      error();
    success = execvp(args[0], args);
    if (success == ERROR)
      error();
    success = close(in);
    if (success == ERROR)
      error();
    success = close(out);
    if (success == ERROR)
      error();
  } else {
    sleep(5);
    pid_t ret = waitpid(pid, NULL, WNOHANG);
    if (ret == 0) { // More than 5 seconds.
      return TIMEOUT;
    } else {
      return compare(config);
    }
  }
}

/**
 * Compiles the files using GCC and forking.
 * @param   student   Current student to compile.
 */
void compile(Student student) {
  char *args[] = {COMPILE_GCC, "-o", OUT_FILE, student.file_path, NULL};
  int success;
  pid_t pid;
  pid = fork();
  if (pid == 0) {
    success = execvp(args[0], args);
    if (success == ERROR)
      error();
  } else {
    waitpid(pid, NULL, 0);
  }
}

/**
 * Grades the students.
 * @param students
 * @param num_of_students
 * @param config
 */
void grade(Student *students, int num_of_students, Config config) {
  int i = 0;
  bool has_out;
  while (i < num_of_students) {
    if (students[i].reason != NO_C_FILE) { // Compile only students with C files
      compile(students[i]);
      has_out = check_for_out();
      if (has_out == false) {
        students[i].reason = COMPILATION_ERROR;
        i++;
        continue;
      }
      students[i].reason = run(config);
      if (unlink(OUT_FILE) == ERROR)
        error();
    }
    i++;
  }
}

/**
 * Returns the file extension.
 * @param   filename    path of the file.
 * @param   sep         Extension to check by (e.g: ".").
 * @return  Returns the extension of the file.
 */
const char *get_filename_ext(const char *filename, const char sep) {
  const char *dot = strrchr(filename, sep);
  if (!dot || dot == filename)
    return "";
  return dot + 1;
}

/**
 * Checks if .out file exists.
 * @return  True if exists, false otherwise.
 */
bool check_for_out() {
  struct dirent *entry;
  char cwd[MAX_LENGTH];
  bool ret = false;
  DIR *dir;
  if (!(dir = opendir(getcwd(cwd, MAX_LENGTH))))
    error();

  while ((entry = readdir(dir)) != NULL) {
    if (entry->d_type == DT_DIR) {
      continue;
    } else {
      if (strcmp(entry->d_name, OUT_FILE) != 0) {
        ret = false;
        continue;
      }
      ret = true;
    }
  }
  if (closedir(dir) == ERROR)
    error();
  return ret;
}

/**
 * Recursively check all subdirectories until a C file is found.
 * @param student   Student being checked.
 * @param name      Path to current folder.
 */
void check_subdirectories(Student *student, char *name) {
  if (student->reason != TBD)
    student->reason = NO_C_FILE;

  DIR *dir;
  struct dirent *entry;

  if (!(dir = opendir(name)))
    return;

  while ((entry = readdir(dir)) != NULL) {
    if (entry->d_type == DT_DIR) {
      char path[300];
      if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
        continue;
      snprintf(path, sizeof(path), "%s/%s", name, entry->d_name);
      check_subdirectories(student, path);
    } else {
      if (strcmp(get_filename_ext(entry->d_name, DOT), C_EXTENSION) != 0)
        continue;
      student->reason = TBD;
      strcpy(student->out_folder, name);
      strcpy(student->file_name, entry->d_name);
      strcpy(student->file_path, name);
      strcat(student->file_path, "/");
      strcat(student->file_path, entry->d_name);
    }

  }
  if (closedir(dir) == ERROR)
    error();
}

/**
 * Check all subdirectories in main folder and make them as students.
 * @param   location          Path to main director.
 * @param   num_of_students    Number of students to update.
 * @return  Dynamically allocated list of all students.
 */
Student *check_directories(char *location, int *num_of_students) {
  Student *students = malloc(sizeof(Student));
  char *loc = calloc(sizeof(char), strlen(location) + 1);
  if (!loc)
    error();
  strcpy(loc, location);
  DIR *dir;
  struct dirent *entry;
  if (!(dir = opendir(location)))
    return students;
  while ((entry = readdir(dir)) != NULL) {
    if (entry->d_type == DT_DIR) {
      if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
        continue;
      students = realloc(students, sizeof(*students) + sizeof(Student) * (*num_of_students));
      students[*num_of_students].reason = TBD;
      strcpy(students[*num_of_students].folder_name, entry->d_name);
      strcpy(students[*num_of_students].folder_path, loc);
      strcat(strcat(students[*num_of_students].folder_path, "/"), entry->d_name);
      students[*num_of_students] = students[*num_of_students];
      check_subdirectories(&students[*num_of_students], students[*num_of_students].folder_path);
      (*num_of_students)++;
    }
  }
  if (closedir(dir) == ERROR)
    error();
  free(loc);
  return students;
}

/**
 * Parse the config file and save it as a struct.
 * @param path_to_config    Path to the file
 * @param config            Config struct.
 */
void parse_config(char *path_to_config, Config *config) {
  int fd;
  int i = 0;
  char buffer[MAX_LENGTH * NUM_OF_LINES] = {};
  char tokens[NUM_OF_LINES][MAX_LENGTH] = {};
  ssize_t num_bytes;
  char *token;
  fd = open(path_to_config, O_RDONLY);
  if (fd == ERROR)
    error();

  num_bytes = read(fd, buffer, sizeof(buffer));
  if (num_bytes == ERROR)
    error();

  token = strtok(buffer, NEW_LINE);
  while (token != NULL) {
    strcpy(tokens[i++], token);
    token = strtok(NULL, NEW_LINE);
  }
  strcpy(config->folders_location, tokens[0]);
  strcpy(config->input_file, tokens[1]);
  strcpy(config->output_file, tokens[2]);
}

/**
 * Main function.
 * @param   argc    Number of arguements (should be 2).
 * @param   argv    Path to configuration file.
 * @return  Exit code
 */
int main(int argc, char *argv[]) {
  if (argc != 2)
    return ERROR;

  Config config;
  parse_config(argv[1], &config);

  Student *students;
  int num_of_students = 0;
  students = check_directories(config.folders_location, &num_of_students);

  grade(students, num_of_students, config);

  save_CSV(students, num_of_students);
  if (unlink(OUTPUT_FILE) == ERROR)
    error();
  free(students);
  return 0;
}

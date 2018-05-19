#include <stdio.h>
#include <string.h>
#include <zconf.h>
#include <stdlib.h>
#include <dirent.h>
#include <wait.h>
#include <fcntl.h>

#define SYS_CALL_ERROR  "Error in system call\n"
#define C_EXTENSION     "c"
#define DOT             '.'
#define COMMA           ','
#define COMPILE_GCC     "gcc"
#define MAX_LENGTH      160
#define NUM_OF_LINES    3
#define OUT_FILE        "user.out"
#define NEW_LINE        "\n"
#define COMP_FILE       "./comp.out"
#define OUTPUT_FILE     "./output.txt"
#define RESULT_FILE     "./result.csv"

const char *reason_arr[] = {"NO_C_FILE", "COMPILATION_ERROR", "TIMEOUT", "BAD_OUTPUT", "SIMILAR_OUTPUT", "GREAT_JOB", "TBD"};
const char *grade_arr[] = {"0", "0", "0", "50", "70", "100", "101"};

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
void save_CSV(Student *pStudent, int students);

void error() {
  write(2, SYS_CALL_ERROR, strlen(SYS_CALL_ERROR));
  exit(-1);
}

void save_CSV(Student *students, int num_of_students) {
  int CSV = open(RESULT_FILE, O_CREAT | O_WRONLY | O_TRUNC, 0644);
  const int arr_size = 200;
  for (int i = 0; i < num_of_students; ++i) {
    char CSV_arr[200];
    strcpy(CSV_arr, students[i].folder_name);
    strcat(CSV_arr, COMMA);
    strcat(CSV_arr, grade_arr[students[i].reason]);
    strcat(CSV_arr, COMMA);
    strcat(CSV_arr, reason_arr[students[i].reason]);
    strcat(CSV_arr, NEW_LINE);
    write(CSV, CSV_arr, 200);
    printf("%s", CSV_arr);
  }
  close(CSV);
}

reason compare(Config config) {
  char *args[] = {COMP_FILE, config.output_file, OUTPUT_FILE, NULL};
  int success;
  pid_t pid = fork();
  if (pid == 0) {
    success = execvp(args[0], args);
    if (success == 0)
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

reason run(Config config) {
  char *args[] = {OUT_FILE, NULL};
  int success;
  int pid = fork();
  if (pid == 0) {
    int out = open(OUTPUT_FILE, O_CREAT | O_WRONLY | O_TRUNC, 0644);
    int in = open(config.input_file, O_RDONLY);
    success = dup2(in, STDIN_FILENO);
    success = dup2(out, STDOUT_FILENO);
    success = execvp(args[0], args);
    success = close(in);
    success = close(out);

  } else {
    sleep(1);
    pid_t ret = waitpid(pid, NULL, WNOHANG);
    if (ret == 0) { // More than 5 seconds.
      return TIMEOUT;
    } else {
      return compare(config);
    }
  }
}

void compile(Student student) {
  char *args[] = {COMPILE_GCC, "-o", OUT_FILE, student.file_path, NULL};
  int success;
  pid_t pid;
  pid = fork();
  if (pid == 0) {
    success = execvp(args[0], args);
    if (success == 0)
      error();
  } else {
    waitpid(pid, NULL, 0);
  }
}

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
      unlink(OUT_FILE);
    }
    i++;
  }
}

const char *get_filename_ext(const char *filename, const char sep) {
  const char *dot = strrchr(filename, sep);
  if (!dot || dot == filename) {
    return "";
  }
  return dot + 1;
}

bool check_for_out() {
  struct dirent *entry;
  char cwd[MAX_LENGTH];
  DIR *dir = opendir(getcwd(cwd, MAX_LENGTH));
  bool ret = false;
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
  closedir(dir);
  return ret;
}

void check_subdirectories(Student *student, char *name) {
  if (student->reason != TBD) {
    student->reason = NO_C_FILE;
  }

  DIR *dir;
  struct dirent *entry;

  if (!(dir = opendir(name))) {
    return;
  }

  while ((entry = readdir(dir)) != NULL) {
    if (entry->d_type == DT_DIR) {
      char path[300];
      if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
        continue;
      snprintf(path, sizeof(path), "%s/%s", name, entry->d_name);
      check_subdirectories(student, path);
    } else {
      if (strcmp(get_filename_ext(entry->d_name, DOT), C_EXTENSION) != 0) {
        continue;
      }
      student->reason = TBD;
      strcpy(student->out_folder, name);
      strcpy(student->file_name, entry->d_name);
      strcpy(student->file_path, name);
      strcat(student->file_path, "/");
      strcat(student->file_path, entry->d_name);
    }

  }
  closedir(dir);
}

Student *check_directories(char *location, int *num_of_students) {
  Student *students = malloc(sizeof(Student));
  char *loc = calloc(sizeof(char), strlen(location));
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
      strcpy(students[*num_of_students].folder_name, entry->d_name);
      strcpy(students[*num_of_students].folder_path, loc);
      strcat(strcat(students[*num_of_students].folder_path, "/"), entry->d_name);
      students[*num_of_students] = students[*num_of_students];
      check_subdirectories(&students[*num_of_students], students[*num_of_students].folder_path);
      (*num_of_students)++;
    }
  }
  closedir(dir);
  return students;
}

void parse_config(char *path_to_config, Config *config) {
  int fd;
  int i = 0;
  char buffer[MAX_LENGTH * NUM_OF_LINES] = {};
  char tokens[NUM_OF_LINES][MAX_LENGTH] = {};
  ssize_t num_bytes;
  char *token;
  fd = open(path_to_config, O_RDONLY);
  if (fd < 0) {
    error();
  }
  num_bytes = read(fd, buffer, sizeof(buffer));
  if (num_bytes < 0)
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

int main(int argc, char *argv[]) {
  if (argc != 2)
    error();

  Config config;
  parse_config(argv[1], &config);

  Student *students;
  int num_of_students = 0;
  students = check_directories(config.folders_location, &num_of_students);

  grade(students, num_of_students, config);

  save_CSV(students, num_of_students);
  free(students);
  return 0;
}

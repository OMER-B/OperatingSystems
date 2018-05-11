#include <stdio.h>
#include <string.h>
#include <zconf.h>
#include <stdlib.h>
#include <dirent.h>

#define SYS_CALL_ERROR "Error in system call\n"
#define C_EXTENSION "c"
#define DOT '.'
#define SLASH '/'

typedef struct Student {
  char *folder_name;
  char *file_path;
  char *file_name;
  int grade;
  struct Student *next;
} Student;

void error();
void get_students(char *name, Student *);
void fill_struct(Student *, char *, const char *, char *);
void grade(Student student);

/**
 *
 * @param argc
 * @param argv
 * @return
 */
int main(int argc, char *argv[]) {
  if (argc != 2) {
    error();
  }

  Student head;
  bzero(&head, sizeof(Student));
  get_students(argv[1], &head);
  head = *head.next; // Remove the first NULL student.

  grade(head);
  return 0;
}

void grade(Student student) {
  //TODO: How to compile?
  char *t[2] = {"/usr/bin/gcc", student.file_path};
  printf(student.file_path);
  execvp(t[0], t);
}

/**
 *
 */
void error() {
  size_t len = strlen(SYS_CALL_ERROR);
  write(2, SYS_CALL_ERROR, len);
  exit(-1);
}

/**
 *
 * @param filename
 * @param sep
 * @return
 */
const char *get_filename_ext(const char *filename, const char sep) {
  const char *dot = strrchr(filename, sep);
  if (!dot || dot == filename) {
    return "";
  }
  return dot + 1;
}

/**
 *
 * @param name
 * @param students
 */
void get_students(char *name, Student *root) {
  Student *student = calloc(sizeof(Student), 1);
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
      get_students(path, root);
    } else {
      if (strcmp(get_filename_ext(entry->d_name, DOT), C_EXTENSION) != 0)
        continue;
      fill_struct(student, entry->d_name, get_filename_ext(name, SLASH), name);
      printf("file name: %s\nfolder name: %s\nfile path: %s\n\n",
             student->file_name,
             student->folder_name,
             student->file_path);

      while (root->next != NULL) {
        root = root->next;
      }
      root->next = student;

    }

  }
  closedir(dir);
}

/**
 *
 * @param student
 * @param file_name
 * @param folder_name
 * @param file_path
 */
void fill_struct(Student *student, char *file_name, const char *folder_name, char *file_path) {
  student->file_name = (char *) calloc(sizeof(char), strlen(file_name));
  strcpy(student->file_name, file_name);

  student->folder_name = (char *) calloc(sizeof(char), strlen(folder_name));
  strcpy(student->folder_name, folder_name);

  student->file_path = (char *) malloc(sizeof(char) * (strlen(file_path) + strlen(file_name) + strlen("/")));
  strcpy(student->file_path, file_path);
  strcat(student->file_path, "/");
  strcat(student->file_path, file_name);
}

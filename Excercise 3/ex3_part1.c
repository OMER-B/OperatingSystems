#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>

#define MAX(a, b) a>(b)?(a):b
#define BUFFER_SIZE 128
#define CASE_DIFF 32
#define ALLOCATION_FAILURE "Allocation failure.\n"
#define SYS_CALL_ERROR "Error in system call"

const char spaces[] = {' ', '\t', '\n', '\r', '\f'};

typedef enum bool { false, true } bool;
typedef enum diff { INVALID, DIFFERENT, SIMILAR, IDENTICAL } diff;

bool identical(const char *, const char *, ssize_t);
bool similar(const char *, const char *, ssize_t);
bool is_space(char);
ssize_t file_to_buffer(char *, char **);
void check_sys_call(ssize_t);
void check_allocation(void *);

int main(int arc, char *argv[]) {
  if (!argv[1] || !argv[2]) { // Check if no argument is given.
    return INVALID;
  }
  ssize_t file1_len = 0, file2_len = 0, max_len = 0;
  diff difference = INVALID;
  char *file1_buffer = NULL;
  char *file2_buffer = NULL;

  // Load files to heap.
  file1_len = file_to_buffer("/home/omer/CLionProjects/untitled8/fir", &file1_buffer);
  file2_len = file_to_buffer("/home/omer/CLionProjects/untitled8/sec", &file2_buffer);

  max_len = MAX(file1_len, file2_len);

  // First check for the files lengths. If length is the same, check if identical.
  if (file1_len == file2_len && identical(file1_buffer, file2_buffer, max_len)) {
    difference = IDENTICAL;
    printf("IDENTICAL\n");
  } else if (similar(file1_buffer, file2_buffer, max_len)) { // If files are not identical, check if similar.
    printf("SIMILAR\n");
    difference = SIMILAR;
  } else { // If files are not identical and not similar, they are different.
    printf("DIFFERENT\n");
    difference = DIFFERENT;
  }
  printf("RESULT IS: %d\n", difference);

  free(file1_buffer);
  free(file2_buffer);

  return difference;
}

/**
 * Loads the file to a buffer on the heap. Saves read from memory.
 * It's messy because needed to change file operations to system calls.
 * @param path Path of the file.
 * @param file_buffer Buffer to load into.
 * @return Length of the file.
 */
ssize_t file_to_buffer(char *path, char **file_buffer) {
  char temp_buffer[BUFFER_SIZE];
  bzero(temp_buffer, BUFFER_SIZE * sizeof(char));
  int file_descriptor;
  register ssize_t file_len = 0;
  register ssize_t num_bytes_read;

  file_descriptor = open(path, O_RDONLY);
  check_sys_call(file_descriptor);
  num_bytes_read = read(file_descriptor, temp_buffer, BUFFER_SIZE * sizeof(char));
  file_len = num_bytes_read;
  *file_buffer = (char *) malloc((num_bytes_read + 1) * sizeof(char));
  check_allocation(*file_buffer);
  strcpy(*file_buffer, temp_buffer);

  num_bytes_read = read(file_descriptor, temp_buffer, BUFFER_SIZE * sizeof(char));

  while (num_bytes_read) {
    file_len = +num_bytes_read;
    check_sys_call(num_bytes_read);
    *file_buffer = (char *) realloc(*file_buffer, num_bytes_read * sizeof(char));
    check_allocation(*file_buffer);
    strcat(*file_buffer, temp_buffer);
    num_bytes_read = read(file_descriptor, temp_buffer, BUFFER_SIZE * sizeof(char));
  }

  num_bytes_read = close(file_descriptor);
  check_sys_call(num_bytes_read);

  return file_len;
}

/**
 * Check if allocation is successful in different function because code got messy.
 * @param allocated allocated pointer.
 */
inline void check_allocation(void *allocated) {
  if (!allocated) {
    printf(ALLOCATION_FAILURE);
    exit(-1);
  }
}

/**
 * Check system call failure in different function because code got messy.
 * @param fd File descriptor.
 */
inline void check_sys_call(ssize_t fd) {
  if (fd < 0) {
    size_t len = strlen(SYS_CALL_ERROR);
    write(stderr, SYS_CALL_ERROR, len);
    exit(-1);
  }
}

/**
 * Checks if two buffers (containing the content of the files) are the same.
 * @param file1 First buffer to compare.
 * @param file2 Second buffer to compare.
 * @param max_len Length of the buffers.
 * @return True if files are identical, false otherwise.
 */
bool identical(const char *file1, const char *file2, ssize_t max_len) {
  register int i = 0;
  for (i = 0; i < max_len; i++) {
    if (file1[i] != file2[i]) {
      return false;
    }
  }
  return true;
}

/**
 * Checks if two buffers (containing the content of the files) are similar.
 * Currently uses flag. Maybe will change when have time.
 * @param file1 First buffer to compare
 * @param file2 Second buffer to compare
 * @param max_len Length of the larger buffer
 * @return True if files are similar, false otherwise.
 */
bool similar(const char *file1, const char *file2, ssize_t max_len) {
  char a[max_len], b[max_len];
  bzero(a, (size_t) max_len);
  bzero(b, (size_t) max_len);
  register int i = 0;
  register int j = 0;

  for (i = 0, j = 0; i < max_len; i++) {
    if (is_space(file1[i])) {
      continue;
    }
    a[j++] = file1[i];
  }
  for (i = 0, j = 0; i < max_len; i++) {
    if (is_space(file2[i])) {
      continue;
    }
    b[j++] = file2[i];
  }

  for (i = 0, j = 0; i < max_len && j < max_len; i++, j++) {
    if (a[i] != b[j]) {
      if (a[i] == b[j] + CASE_DIFF || a[i] + CASE_DIFF == b[j]) {
        continue;
      }
      return false;
    }
  }
  return true;
}

/**
 * Checks if a char is space.
 * @param c Char to check
 * @return True if char is space, false otherwise.
 */
inline bool is_space(char c) {
  register int i = 0;
  size_t num_of_spaces = sizeof(spaces) / sizeof(char);
  for (i = 0; i < num_of_spaces; i++) {
    if (c == spaces[i]) {
      return true;
    }
  }
  return false;
}

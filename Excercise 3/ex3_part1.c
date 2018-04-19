#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>

#define MAX(a, b) a>(b)?(a):b
#define BUFFER_SIZE 128
#define ALLOCATION_FAILURE "Allocation failure.\n"
#define SYS_CALL_ERROR "Error in system call"

typedef enum bool { false, true } bool;
typedef enum diff { INVALID, DIFFERENT, SIMILAR, IDENTICAL } diff;

bool identical(const char *, const char *, ssize_t);
bool similar(const char *, const char *, ssize_t);
ssize_t file_to_buffer(char *, char **);

int main(int arc, char *argv[]) {
  if (!argv[1] || !argv[2]) { // Check if no argument is given.
    return INVALID;
  }
  ssize_t file1_len = 0, file2_len = 0, max_len = 0;
  diff difference = INVALID;
  char *file1_buffer = NULL;
  char *file2_buffer = NULL;

  // Load files to heap.
  file1_len = file_to_buffer(argv[1], &file1_buffer);
  file2_len = file_to_buffer(argv[2], &file2_buffer);

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
  ssize_t file_len = 0;
  ssize_t num_bytes_read;

  file_descriptor = open(path, O_RDONLY);
  if (file_descriptor < 0) {
    fprintf(stderr, SYS_CALL_ERROR);
    exit(-1);
  }

  num_bytes_read = read(file_descriptor, temp_buffer, BUFFER_SIZE * sizeof(char)); // read entire file
  file_len = num_bytes_read;
  *file_buffer = (char *) malloc((num_bytes_read) * sizeof(char));
  if (!*file_buffer) {
    printf(ALLOCATION_FAILURE);
    exit(-1);
  }
  strcpy(*file_buffer, temp_buffer);
  num_bytes_read = read(file_descriptor, temp_buffer, BUFFER_SIZE * sizeof(char));
  while (num_bytes_read) {
    file_len = +num_bytes_read;
    if (num_bytes_read < 0) {
      fprintf(stderr, SYS_CALL_ERROR);
      exit(-1);
    }
    *file_buffer = (char *) realloc(*file_buffer, num_bytes_read * sizeof(char));
    if (!*file_buffer) {
      printf(ALLOCATION_FAILURE);
      exit(-1);
    }
    strcat(*file_buffer, temp_buffer);
    num_bytes_read = read(file_descriptor, temp_buffer, BUFFER_SIZE * sizeof(char));
  }

  num_bytes_read = close(file_descriptor);
  if (num_bytes_read < 0) {
    fprintf(stderr, SYS_CALL_ERROR);
    exit(-1);
  }
  return file_len;
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
  register int i = 0;
  register int j = 0;
  bool flag = false;
  for (i = 0, j = 0; i < max_len && j < max_len; i++, j++) {
    if (file1[i] != file2[j]) {
      flag = false;
      if (file1[i] == file2[j] + 32 || file1[i] + 32 == file2[j]) {
        flag = true;
        continue;
      }
      if (file1[i] == '\n' || file1[i] == ' ' || file1[i] == '\t') {
        --j;
        continue;
      } else if (file2[j] == '\n' || file2[j] == ' ' || file2[j] == '\t') {
        --i;
        continue;
      }
      return false;
    } else {
      flag = true;
    }
  }
  return flag;
}

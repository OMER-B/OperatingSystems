#include <stdio.h>
#include <stdlib.h>

#define MAX(a, b) a>(b)?(a):b
typedef enum bool { false, true } bool;
typedef enum diff { INVALID, DIIFFERENT, SIMILAR, IDENTICAL } diff;

bool indetical(const char *, const char *, size_t);
bool similar(const char *, const char *, size_t);
size_t len(FILE *);
size_t file_to_buffer(char *, char **);

int main(int arc, char *argv[]) {
  if (!argv[1] || !argv[2]) { // Check if no argument is given.
    return INVALID;
  }
  size_t file1_len = 0, file2_len = 0, max_len = 0;
  diff difference = INVALID;
  char *file1_buffer = NULL;
  char *file2_buffer = NULL;

  // Load files to heap.
  file1_len = file_to_buffer(argv[1], &file1_buffer);
  file2_len = file_to_buffer(argv[2], &file2_buffer);

  max_len = MAX(file1_len, file2_len);

  // First check for the files lengths. If length is the same, check if identical.
  if (file1_len == file2_len && indetical(file1_buffer, file2_buffer, max_len)) {
    difference = IDENTICAL;
    printf("IDENTICAL\n");
  } else if (similar(file1_buffer, file2_buffer, max_len)) { // If files are not identical, check if similar.
    printf("SIMILAR\n");
    difference = SIMILAR;
  } else { // If files are not identical and not similar, they are different.
    printf("DIFFERENT\n");
    difference = DIIFFERENT;
  }
  printf("RESULT IS: %d\n", difference);

  free(file1_buffer);
  free(file2_buffer);

  return difference;
}

/// Loads the file to a buffer on the heap. Save read from memory.
/// \param path Path of the file.
/// \param file_buffer Buffer to load into.
/// \return Length of the file.
size_t file_to_buffer(char *path, char **file_buffer) {
  size_t file_len = 0;
  int success;
  FILE *file = fopen(path, "rb");
  if (file == NULL) {
    fprintf(stderr, "Error in system call");
    exit(-1);
  }

  file_len = len(file);

  *file_buffer = (char *) malloc((file_len + 1) * sizeof(char));
  if (!*file_buffer) {
    printf("Allocation failure.\n");
    exit(-1);
  }

  fread(*file_buffer, file_len, sizeof(char), file); // read entire file
  success = fclose(file);
  if (success == EOF) {
    fprintf(stderr, "Error in system call");
    exit(-1);
  }
  return file_len;
}

/// Goes through the file and checks for its length.
/// \param file File to check the length for.
/// \return Length of the file.
inline size_t len(FILE *file) {
  size_t len = 0;
  fseek(file, 0, SEEK_END); // seek to end of file
  len = (size_t) ftell(file); // get current file pointer
  rewind(file); // seek back to beginning of file
  return len;
}

/// Checks if two buffers (containing the content of the files) are the same.
/// \param file1 First buffer to compare.
/// \param file2 Second buffer to compare.
/// \param max_len Length of the buffers.
/// \return true if files are identical, false otherwise.
bool indetical(const char *file1, const char *file2, size_t max_len) {
  register int i = 0;
  for (i = 0; i < max_len; i++) {
    if (file1[i] != file2[i]) {
      return false;
    }
  }
  return true;
}

/// Checks if two buffers (containing the content of the files) are similar.
/// Currently uses flag. Maybe will change when have time.
/// \param file1 First buffer to compare.
/// \param file2 Second buffer to compare.
/// \param max_len Length of the bigger buffer.
/// \return true if files are similar, false otherwise.
bool similar(const char *file1, const char *file2, size_t max_len) {
  register int i = 0;
  register int j = 0;
  bool flag = false;
  for (i = 0, j = 0; i < max_len && j < max_len; i++, j++) {
    if (file1[i] != file2[j]) {
      if (file1[i] == file2[j] + 32 || file1[i] + 32 == file2[j]) {
        flag = true;
        continue;
      }
      if (file1[i] == '\n' || file1[i] == ' ') {
        --j;
        continue;
      } else if (file2[j] == '\n' || file2[j] == ' ') {
        --i;
        continue;
      }
      flag = false;
    } else {
      flag = true;
    }
  }
  return flag;
}

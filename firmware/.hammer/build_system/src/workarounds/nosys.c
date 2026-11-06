#include <errno.h>
#include <sys/types.h>

struct stat;
struct timeval;
struct tms;

int _chown(const char *path, uid_t owner, gid_t group) __attribute__((weak));
int _close(int fildes) __attribute__((weak));
int _execve(char *name, char **argv, char **env) __attribute__((weak));
int _fork(void) __attribute__((weak));
int _fstat(int fildes, struct stat *st) __attribute__((weak));
int _getpid(void) __attribute__((weak));
int _gettimeofday(struct timeval *ptimeval, void *ptimezone) __attribute__((weak));
int _isatty(int file) __attribute__((weak));
int _kill(int pid, int sig) __attribute__((weak));
int _link(char *existing, char *new) __attribute__((weak));
int _lseek(int file, int ptr, int dir) __attribute__((weak));
int _open(char *file, int flags, int mode) __attribute__((weak));
int _read(int file, char *ptr, int len) __attribute__((weak));
int _readlink(const char *path, char *buf, size_t bufsize) __attribute__((weak));
int _stat(const char *file, struct stat *st) __attribute__((weak));
int _symlink(const char *path1, const char *path2) __attribute__((weak));
clock_t _times(struct tms *buf) __attribute__((weak));
int _unlink(char *name) __attribute__((weak));
int _wait(int *status) __attribute__((weak));
int _write(int file, char *ptr, int len) __attribute__((weak));

int _chown(const char * /*path*/, uid_t /*owner*/, gid_t /*group*/) {
  errno = ENOSYS;
  return -1;
}
int _close(int /*fildes*/) {
  errno = ENOSYS;
  return -1;
}
int _execve(char * /*name*/, char ** /*argv*/, char ** /*env*/) {
  errno = ENOSYS;
  return -1;
}
int _fork(void) {
  errno = ENOSYS;
  return -1;
}
int _fstat(int /*fildes*/, struct stat * /*st*/) {
  errno = ENOSYS;
  return -1;
}
int _getpid(void) {
  errno = ENOSYS;
  return -1;
}
int _gettimeofday(struct timeval * /*ptimeval*/, void * /*ptimezone*/) {
  errno = ENOSYS;
  return -1;
}
int _isatty(int /*file*/) {
  errno = ENOSYS;
  return 0;
}
int _kill(int /*pid*/, int /*sig*/) {
  errno = ENOSYS;
  return -1;
}
int _link(char * /*existing*/, char * /*new*/) {
  errno = ENOSYS;
  return -1;
}
int _lseek(int /*file*/, int /*ptr*/, int /*dir*/) {
  errno = ENOSYS;
  return -1;
}
int _open(char * /*file*/, int /*flags*/, int /*mode*/) {
  errno = ENOSYS;
  return -1;
}
int _read(int /*file*/, char * /*ptr*/, int /*len*/) {
  errno = ENOSYS;
  return -1;
}
int _readlink(const char * /*path*/, char * /*buf*/, size_t /*bufsize*/) {
  errno = ENOSYS;
  return -1;
}
int _stat(const char * /*file*/, struct stat * /*st*/) {
  errno = ENOSYS;
  return -1;
}
int _symlink(const char * /*path1*/, const char * /*path2*/) {
  errno = ENOSYS;
  return -1;
}
clock_t _times(struct tms * /*buf*/) {
  errno = ENOSYS;
  return -1;
}
int _unlink(char * /*name*/) {
  errno = ENOSYS;
  return -1;
}
int _wait(int * /*status*/) {
  errno = ENOSYS;
  return -1;
}
int _write(int /*file*/, char * /*ptr*/, int /*len*/) {
  errno = ENOSYS;
  return -1;
}

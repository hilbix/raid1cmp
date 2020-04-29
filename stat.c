#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

int main(int argc, char **argv)
{
  struct stat st;
  int	ret;

  ret = lstat(argv[1], &st);
  if (ret)
    perror(argv[1]);

  printf("name %s\n", argv[1]);
  printf("dev  %llx\n", (unsigned long long)st.st_dev);
  printf("ino  %llu\n", (unsigned long long)st.st_ino);
  printf("mode %llo\n", (unsigned long long)st.st_mode);
  printf("link %llu\n", (unsigned long long)st.st_nlink);
  printf("uid  %llu\n", (unsigned long long)st.st_uid);
  printf("gid  %llu\n", (unsigned long long)st.st_gid);
  printf("rdev %llx\n", (unsigned long long)st.st_rdev);
  printf("size %llu\n", (unsigned long long)st.st_size);
  printf("bsiz %llu\n", (unsigned long long)st.st_blksize);
  printf("bcnt %llu\n", (unsigned long long)st.st_blocks);
  return ret;
}


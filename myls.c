/* myls.c
 * C program file.
 *
 * Name: Michael Ciminesi
 * This program is designed to catalog the contents of the current directory.
 * Program can be compiled using the <make> command from the included makefile.
 * Features:
 *  +Dynamically resizing multi-column format.
 *  +Runs on multiple command line directory arguments, including:
 *    -The root directory ("/")
 *    -Named directories ("/users/")
 *    -Current or previous directory (either with no argments or by using ".")
 *    -Directory relative to the current or previous ("./users/" or "../users/")
 * Options included:
 * -a   Desc: do not hide entries starting with .
 * -A   Desc: do not list implied . and ..
 * -F   Desc: append indicator (one of *=@|/) to entries
 * -g   Desc: like -l, but do not list owner
 * -G   Desc: inhibit display of group information
 * -i   Desc: print index number of each file
 * -l   Desc: use a long listing format
 * -n   Desc: like -l, but list numeric UIDs and GIDs
 * -o   Desc: like -l, but do not list group information
 * -p   Desc: append indicator (one of /=@|) to entries
 * -Q   Desc: enclose entry names in double quotes
 * -r   Desc: reverse order while sorting
 * -s   Desc: print size of each file, in blocks
 * -w   Desc: assume screen width instead of current value
 *            Entering a value o 0 for this option will result in all output on a single line.
 *
 * All options interact the same as they do in ls.
 */
#include <ctype.h>
#include <dirent.h>
#include <grp.h>
#include <limits.h>
#include <pwd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>
#define COL_MAX 20

static void sortFiles();
static int myftw(char *);
static int dopath();
static int padName(char oldname[], char *name);
static unsigned int countDigits(unsigned long int num);
static void calculateColumnCount();
static void printDir();
static void initializeFiles();
static void convertMode(mode_t oldmode, char *mode);
static char *fullpath; /* contains full pathname for every file */
static size_t pathlen;
static int aflag, Aflag, Fflag, gflag, Gflag, iflag, lflag, nflag, oflag, pflag, Qflag, rflag, sflag, wflag,
  line_format, max_size, column_count, win_cols, name_indent, user_indent, group_indent, size_indent,
  links_indent, blocks_indent, col_indent_inode_max, total_files;
static char *wvalue;
static struct winsize w;
static blkcnt_t total_blocks;
static int col_indent[COL_MAX];
static int col_indent_blocks[COL_MAX];
static int col_indent_inode[COL_MAX];
struct FileInfo {
  char name[NAME_MAX + 3];
  int size;
  ino_t inode;
  uid_t user;
  gid_t group;
  off_t size_bytes;
  time_t time;
  nlink_t links;
  mode_t mode;
  blkcnt_t blocks;
};
static struct FileInfo files[2048];

int main(int argc, char *argv[]) {
  wvalue = NULL;
  opterr = 0;  
  // Options are handled here, with each one setting the necessary flags.
  int option;
  while ((option = getopt(argc, argv, "aAFgGilnopQrsw:")) != -1) {
    switch (option) {
      case 'a':
        aflag = 1;
        break;
      case 'A':
        Aflag = 1;
        break;
      case 'F':
        Fflag = 1;
        break;
      case 'g':
        gflag = 1;
        lflag = 1;
        line_format = 1;
        break;
      case 'G':
        Gflag = 1;
        break;
      case 'i':
        iflag = 1;
        break;
      case 'l':
        lflag = 1;
        line_format = 1;
        break;
      case 'n':
        nflag = 1;
        lflag = 1;
        line_format = 1;
        break;
      case 'o':
        oflag = 1;
        lflag = 1;
        line_format = 1;
        break;
      case 'p':
        pflag = 1;
        break;
      case 'Q':
        Qflag = 1;
        break;
      case 'r':
        rflag = 1;
        break;
      case 's':
        sflag = 1;
        break;
      case 'w':
        wflag = 1;
        wvalue = optarg;
        break;
      case '?':
        if(optopt == 'w') {
          printf("ls: option requires an argument -- %c\n", optopt);
          exit(1);
        } else if (isprint(optopt)) {
          printf("ls: Unknown option -- %c\nTry the following options 'aAFgGilnopQrsw'.\n", optopt);
          exit(1);
        } else {
          printf("ls: Unknown option\nTry the following options 'aAFgGilnopQrsw'.\n");
          exit(1);
        }
      default:
        abort();
    }
  }
  // If w option is used, changes the number of columns.
  ioctl(0, TIOCGWINSZ, &w);
  if (wflag == 1) {
    for (int i = 0; i < strlen(wvalue); ++i) {
      if (!isdigit(wvalue[i])) {
        printf("ls: invalid line width: '%s'\n", wvalue);
        exit(1);
      }
    }
    char *endptr;
    long value;
    value = strtol(wvalue, &endptr, 10);
    win_cols = value;
  } else win_cols = w.ws_col;
  int ret;  
  // Get the current working directory.
  char curr_path[1024];
  getcwd(curr_path, sizeof(curr_path));
  // If multiple command line arguments were used, each directory is handled in turn.
  if (optind < argc) {
    for (int index = optind; index < argc; ++index) {
      char new_path[1024] = "";
      // If more than 1 command line argument, print the directories.
      if (argc - 1 > optind) {
        if (strcmp(argv[index], ".") == 0 || strcmp(argv[index], "..") == 0 || strcmp(argv[index], "/") == 0)
          printf ("%s:\n", argv[index]);
        else {
          printf("'%s':\n", argv[index]);
        }
      }
      if (argv[index][0] == '/') {
        strncpy(new_path, argv[index], strlen(argv[index]));
      } else {
        strncpy(new_path, curr_path, 1024);
        strncat(new_path, "/", strlen("/"));
        strncat(new_path, argv[index], (sizeof(new_path) - strlen(new_path)) );
      }
      strcpy(&new_path[strlen(new_path)], "\0");
      initializeFiles();
      ret = myftw(new_path);
      sortFiles();
      if (win_cols == 0) column_count = total_files;
      else if (!line_format) calculateColumnCount();
      printDir();
      if (index < argc -1) printf ("\n");
    }
  // If no command line arguments were used, just the current working directory
  // is listed.
  } else {
    initializeFiles();
    ret = myftw(curr_path);
    sortFiles();
    if (win_cols == 0) column_count = total_files;
    else if(!line_format) calculateColumnCount();
    printDir();
  }
  return ret;
}
// This function is used as a helper function for dopath. It allocates memory
// for the path and then kicks off the file processing.
// Pre: None.
// Post: Memory is allocated for and pathname parameter is copied to fullpath.
static int myftw(char *pathname) {
  int path_size = 1024;
  if((fullpath = malloc(path_size)) == NULL) {
    printf("malloc failed\n");
    exit(1);
  }
  pathlen = path_size;
  if (pathlen <= strlen(pathname)) {
    pathlen = strlen(pathname) * 2;
    if ((fullpath = realloc(fullpath, pathlen)) == NULL) {
      printf("realloc failed\n");
      exit(1);
    }
  }
  strcpy(fullpath, pathname);
  return(dopath());
}
// This function is used to read in file information for a directory.
// Pre: The fullpath variable must contain a valid path to a directory.
// Post: All files and folders in the directory will have info stored in array.
static int dopath() {
  struct stat statbuf;
  struct dirent *dirp;
  DIR *dp;
  int ret, n;
  n = strlen(fullpath);
  // Expands path buffer if it is too small.
  if (n + NAME_MAX + 2 > pathlen) {
    pathlen *= 2;
    if ((fullpath = realloc(fullpath, pathlen)) == NULL) {
      printf("realloc failed\n");
      exit(1);
    }
  }
  // Open directory and exit if unsuccessful.
  if ((dp = opendir(fullpath)) == NULL) {
    printf("ls: cannot access '%s'\n", fullpath);
    exit(0);
  }
  // Iterates through the files in the directory.
  while ((dirp = readdir(dp)) != NULL) {
    // Handles hidden files, including . and .., using options a and A.
    if (!aflag && !Aflag) {
      // This is used to ignore all entries starting with a . in the 0 position.
      if (dirp->d_name[0] == '.') {
        continue;
      }
    }
    // For option A, ignores . and .. files.
    if (Aflag) {
      if (strcmp(dirp->d_name, ".") == 0 || strcmp(dirp->d_name, "..") == 0) {
        continue;
      }
    }
    char name[NAME_MAX + 1] = "";
    strncpy(name, dirp->d_name, strlen(dirp->d_name));
    int len = strlen(name);
    char *name_pointer;
    // For option Q, adds double quotes to file name.
    if (Qflag == 1) { 
      char new_name[NAME_MAX + 3] = "";
      strncpy(new_name, "\"", 1);
      strncat(new_name, name, len);
      strcpy(&new_name[strlen(new_name)], "\"");
      name_pointer = new_name;
    } else {
      name_pointer = name;
    }
    // Each entry appended and lstated here.
    if (fullpath[n - 1] != '/') {
      strcpy(&fullpath[n], "/");
      strcpy(&fullpath[n + 1], dirp->d_name);
    } else {
      strcpy(&fullpath[n], dirp->d_name);
    }
    if (lstat(fullpath, &statbuf) < 0) printf("Stat error\n");
    if (Fflag || pflag) {
      switch (statbuf.st_mode & S_IFMT) {
        case S_IFDIR:
          name_pointer[strlen(name_pointer)] = '/';
          break;
        case S_IFIFO:
          name_pointer[strlen(name_pointer)] = '|';
          break;
        case S_IFLNK:
          name_pointer[strlen(name_pointer)] = '@';
          break;
        case S_IFSOCK:
          name_pointer[strlen(name_pointer)] = '=';
          break;
        case S_IFREG:
          if (Fflag) {
            if (statbuf.st_mode & S_IXUSR) name_pointer[strlen(name_pointer)] = '*';
            else if (statbuf.st_mode & S_IXGRP) name_pointer[strlen(name_pointer)] = '*';
            else if (statbuf.st_mode & S_IXOTH) name_pointer[strlen(name_pointer)] = '*';
          }
          break;
        default:
          break;
      }
    }
    // Fill in values for each file here.
    strcpy(files[total_files].name, name_pointer);
    files[total_files].size = strlen(name_pointer);
    files[total_files].inode = statbuf.st_ino;
    files[total_files].user = statbuf.st_uid;
    files[total_files].group = statbuf.st_gid;
    files[total_files].size_bytes = statbuf.st_size;
    files[total_files].time = statbuf.st_mtime;
    files[total_files].links = statbuf.st_nlink;
    files[total_files].mode = statbuf.st_mode;
    files[total_files].blocks = statbuf.st_blocks;
    total_blocks += statbuf.st_blocks;
    // For option n, calculate padding based on numberic uid and gid.
    if (nflag) {
      if (countDigits(statbuf.st_gid) > group_indent) group_indent = countDigits(statbuf.st_gid);
      if (countDigits(statbuf.st_uid) > user_indent) user_indent = countDigits(statbuf.st_uid);
    // Otherwise, use the string version of uid and gid for padding.
    } else {
      struct passwd *pwd;
      struct group *grp;
      pwd = getpwuid(statbuf.st_uid);
      grp = getgrgid(statbuf.st_gid);
      if (strlen(grp->gr_name) > group_indent) group_indent = strlen(grp->gr_name);
      if (strlen(pwd->pw_name) > user_indent) user_indent = strlen(pwd->pw_name);
    }
    // Calculate padding for names, links, size, blocks, and inodes.
    if (strlen(name) > max_size) max_size = strlen(name_pointer);
    if (countDigits(statbuf.st_nlink) > links_indent) links_indent = countDigits(statbuf.st_nlink);
    if (countDigits(statbuf.st_size) > size_indent) size_indent = countDigits(statbuf.st_size);
    if (countDigits(statbuf.st_blocks) > blocks_indent) blocks_indent = countDigits(statbuf.st_blocks);
    if (countDigits(statbuf.st_ino) > col_indent_inode_max) col_indent_inode_max = countDigits(statbuf.st_ino);
    ++total_files;   
  }
  // Reset fullpath and close directory.
  fullpath[n-1] = 0;
  if (closedir(dp) < 0) {
    printf("can’t close directory %s\n", fullpath);  
    exit(1);
  }
  return(ret);
}
// This function is used to count the digits in a long int, used for padding.
// Pre: None.
// Post: Returns the number of digits used to represent the parameter.
unsigned int countDigits(unsigned long int num) {
  if (num < 10) return 1;
  return 1 + countDigits(num / 10);
}
// This function is used to calculate the length of each column when not using line format.
// Pre: Must have file information in array, including name size, blocks, and inode counts.
// Post: The column_count is set to ensure all file info will fit onscreen.
void calculateColumnCount() {
  // Starting from COL_MAX, reduce column_count until all columns will fit in rows.
  column_count = COL_MAX;
  int row_size;
  while (column_count > 1) {
    row_size = total_files / column_count;
    if (total_files % column_count > 0) ++row_size;
    int new_column_count = total_files / row_size;
    if (total_files % row_size > 0) ++new_column_count;
    if (new_column_count < column_count) column_count = new_column_count;
    for (int i = 0; i < total_files; ++i) {
      int pos = i;
      if (rflag) pos = (total_files - 1) - pos;
      // Checks size of file name.
      if (files[pos].size > col_indent[i / row_size])
        col_indent[i / row_size] = files[pos].size;
      if (countDigits(files[pos].blocks) > col_indent_blocks[i / row_size])
        col_indent_blocks[i / row_size] = countDigits(files[pos].blocks);
      if (countDigits(files[pos].inode) > col_indent_inode[i / row_size])
        col_indent_inode[i / row_size] = countDigits(files[pos].inode);
    }
    int row_len = 0;
    for (int j = 0; j < column_count; ++j) {
      row_len += col_indent[j];
      if (sflag == 1) row_len += blocks_indent + 1;
      if (iflag == 1) row_len += col_indent_inode[j] + 1;
    }
    // If the row is longer than the the number of columns available, decrease column count.
    if (row_len + ((column_count - 1) * 2) >= win_cols) {
      for (int k = 0; k < column_count; ++k) {
        col_indent[k] = 0;
      }
      --column_count;
    } else break;
  }
  if (column_count == 1) col_indent[0] = max_size;
}
// This function is used to print the info stored when reading files.
// Pre: File information must be stored in file array.
// Post: Directory contents are printed to the terminal based on selected options.
void printDir() {
  int pos;
  if (line_format == 1) {
    printf("total %ld\n", total_blocks);
    for (int i = 0; i < total_files; ++i) {
      pos = i;
      if (rflag) pos = (total_files - 1) - pos;
      if (lflag == 1) {
        if (iflag == 1) printf("%*ld ", col_indent_inode_max, files[pos].inode);
        if (sflag == 1) printf("%*ld ", blocks_indent, files[pos].blocks);
        char mode[12];
        convertMode(files[pos].mode, mode);
        printf("%s %*ld ", mode, links_indent, files[pos].links);
        if (gflag == 0) {
          if (nflag) printf("%*d ", user_indent, files[pos].user);
          else {
            struct passwd *pwd;
            pwd = getpwuid(files[pos].user);
            printf("%*s ", user_indent, pwd->pw_name);
          }
        }
        if (Gflag == 0 && oflag == 0) {
          if (nflag) printf("%*d ", group_indent, files[pos].group);
          else {
            struct group *grp;
            grp = getgrgid(files[pos].group);
            printf("%*s ", group_indent, grp->gr_name);
          }
        }
        printf("%*ld ", size_indent, files[pos].size_bytes);
        // Prints times using fill-in functions. Prints year for files more than 6 months old.
        struct tm *file_time;
        char time_buf[41];
        file_time = localtime(&files[pos].time);
        time_t raw_time_now;
        raw_time_now = time (NULL);
        if(raw_time_now - files[pos].time < 180 * 24 * 60 * 60) strftime(time_buf, 41, "%b %e %H:%M", file_time);
        else strftime(time_buf, 41, "%b %e  %Y", file_time);
        printf("%s %s", time_buf, files[pos].name);        
        if (i < total_files - 1) printf("\n");
      }
    }
  // When not using line format, entries are printed in columns based on the
  // number of columns calculated from entry lengths.
  } else {
    int column_len = (total_files) / column_count;
    if (total_files % column_count > 0) ++column_len;
    for (int i = 0; i < column_len; ++i) {
      for (int j = 0; j < column_count; ++j) {
        pos = i + (j * column_len);
        if (pos < total_files) {
          if (rflag) pos = (total_files - 1) - pos;
          if (iflag == 1) {
            printf("%*ld ", col_indent_inode_max, files[pos].inode);
          }
          if (sflag == 1) {
            printf("%*s%ld ", blocks_indent - countDigits(files[pos].blocks), "", files[pos].blocks);
          }
          if (win_cols == 0) printf("%s", files[pos].name);
          else printf("%s%*s", files[pos].name, (int)(col_indent[(i + (j * column_len))/column_len] - strlen(files[pos].name)), "");
        }
        if (j == column_count - 1 && i < column_len - 1) printf("\n");
        else if (j < column_count - 1) printf("  "); 
      }  
    }
  }
  printf("\n");
}
// This function is used to reset values when switching directories.
// Pre: None.
// Post: Values are reset to starting values.
void initializeFiles() {
  total_files = 0; 
  max_size = 0;
  col_indent_inode_max = 0;
  name_indent = 0;
  user_indent = 0;
  group_indent = 0;
  size_indent = 0;
  links_indent = 0;
  blocks_indent = 0;
  column_count = 1;
  total_blocks = 0;
  for (int i = 0; i < COL_MAX; ++i) {
    col_indent[i] = 0;
    col_indent_blocks[i] = 0;
    col_indent_inode[i] = 0;
  }
}
// This function is used to convert the mode value into a string.
// Pre: Mode must be stored in a mode_t object.
// Post: Fill-in string passed by reference contains mode string.
void convertMode(mode_t oldmode, char *mode) {
  if(S_ISDIR(oldmode)) mode[0] = 'd';
  else if(S_ISLNK(oldmode)) mode[0] = 'l';
  else if(S_ISFIFO(oldmode)) mode[0] = 'p';
  else if(S_ISCHR(oldmode)) mode[0] = 'c';
  else if(S_ISBLK(oldmode)) mode[0] = 'b';
  else if(S_ISSOCK(oldmode)) mode[0] = 's';
  else mode[0] = '-';
  if(oldmode & S_IRUSR) mode[1] = 'r';
  else mode[1] = '-';
  if(oldmode & S_IWUSR) mode[2] = 'w';
  else mode[2] = '-';
  if(oldmode & S_IXUSR) mode[3] = 'x';
  else mode[3] = '-';
  if(oldmode & S_IRGRP) mode[4] = 'r';
  else mode[4] = '-';
  if(oldmode & S_IWGRP) mode[5] = 'w';
  else mode[5] = '-';
  if(oldmode & S_IXGRP) mode[6] = 'x';
  else mode[6] = '-';
  if(oldmode & S_IROTH) mode[7] = 'r';
  else mode[7] = '-';
  if(oldmode & S_IWOTH) mode[8] = 'w';
  else mode[8] = '-';
  if(oldmode & S_IXOTH) mode[9] = 'x';
  else mode[9] = '-';
  mode[10] = '\0';
}
// This function sorts the files in the array by name.
// Pre: Files in array must have names.
// Post: Files will be sorted by name in array.
void sortFiles() {
  int j;
  for (int i = 1; i < total_files; ++i) {
    j = i;
    while (j > 0 && strcmp(files[j].name, files[j - 1].name) < 0) {
      struct FileInfo temp = files[j];
      files[j] = files[j - 1];
      files[j - 1] = temp;
      --j;
    }
  }
}
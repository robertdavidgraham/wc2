/* Compares the output of our 'wc' implementation with the standard
 * 'wc' program in order to find out where they differ. */
#include <stdio.h>
#include <sys/stat.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <ctype.h>
#include <sys/wait.h>

/**
 * Tests the filename to make sure it exists
 */
int is_exists(const char *filename)
{
    struct stat st = {0};

    if (stat(filename, &st) != 0) {
        fprintf(stderr, "[-] %s: %s\n", filename, strerror(errno));
        return 0;
    }
    return 1;
}

/**
 * Tests the filename to make sure it's an executable file
 */
int is_executable(const char *filename)
{
    struct stat st = {0};

    if (stat(filename, &st) != 0) {
        fprintf(stderr, "[-] %s: %s\n", filename, strerror(errno));
        return 0;
    }
    if (!(st.st_mode & S_IXUSR)) {
        fprintf(stderr, "[-] %s: not executable\n", filename);
        return 0;
    }
    return 1;
}

struct handles
{
    int child_stdin;
    int child_stdout;
    int child_stderr;
};

struct handles
my_spawn(const char *progname, const char *parms)
{
    int pid;
    int pipe_stdout[2];
    int pipe_stderr[2];
    int pipe_stdin[2];
    struct handles handles = {0,0,0};
    int err;
    char **new_argv;

    
    err = pipe(pipe_stdout);
    if (err < 0) {
        fprintf(stderr, "[-] pipe(): %s\n", strerror(errno));
        return handles;
    }
    err = pipe(pipe_stderr);
    if (err < 0) {
        fprintf(stderr, "[-] pipe(): %s\n", strerror(errno));
        return handles;
    }
    err = pipe(pipe_stdin);
    if (err < 0) {
        fprintf(stderr, "[-] pipe(): %s\n", strerror(errno));
        return handles;
    }
    
    /* Save the pipes that we'll use later */
    //workers->parent_stdout = pipe_stdout[0];
    //workers->parent_stderr = pipe_stderr[0];
    //workers->child_stdout = pipe_stdout[1];
    //workers->child_stderr = pipe_stderr[1];
    /*handles.child_stdin = fdopen(pipe_stdin[0], "rwb");
    if (handles.child_stdin == NULL) {
        fprintf(stderr, "[-] fdopen(stdin): %s\n", strerror(errno));
        return handles;
    }
    handles.child_stdout = fdopen(pipe_stdout[0], "rwb");
    if (handles.child_stdout == NULL) {
        fprintf(stderr, "[-] fdopen(stdout): %s\n", strerror(errno));
        return handles;
    }
    handles.child_stderr = fdopen(pipe_stderr[0], "rwb");
    if (handles.child_stderr == NULL) {
        fprintf(stderr, "[-] fdopen(stderr): %s\n", strerror(errno));
        return handles;
    }*/


    
    /* Configure the parent end of the pipes be be non-inheritable.
     * In other words, none of the children can read from these
     * pipes, nor will they exist in child process space */
    //fcntl(pipe_stdout[0], F_SETFD, FD_CLOEXEC);
    //fcntl(pipe_stderr[0], F_SETFD, FD_CLOEXEC);
    //fcntl(pipe_stdin[0], F_SETFD, FD_CLOEXEC);

    pid = fork();
    if (pid == -1) {
        fprintf(stderr, "[-] fork(): %s\n", strerror(errno));
        return handles;
    }
    
    /* Setup child parameters */
    new_argv = malloc(10 * sizeof(char*));
    new_argv[0] = (char *)progname;
    new_argv[1] = (char *)parms;
    new_argv[2] = NULL;
    new_argv[3] = NULL;
    
    if (pid == 0) {
        int err;

        close(pipe_stdin[1]);
        close(pipe_stdout[0]);
        close(pipe_stderr[0]);
        
        dup2(pipe_stdin[0], 0);
        dup2(pipe_stdout[1], 1);
        dup2(pipe_stderr[1], 2);
        
        /* Now execute our child with new program */
        err = execvp(progname, new_argv);
        if (err) {
            fprintf(stderr, "[+] execvp(%s) failed: %s\n", progname, strerror(errno));
            return handles;
        }
        
    } else {
        handles.child_stdin = pipe_stdin[1];
        handles.child_stdout = pipe_stdout[0];
        handles.child_stderr = pipe_stderr[0];

        close(pipe_stdout[1]);
        close(pipe_stderr[1]);
        close(pipe_stdin[0]);

        /* we are the parent */
        return handles;
    }
    return handles;
}

struct wcresults {
    long line_count;
    long word_count;
    long char_count;
};

static long
get_integer(int fd)
{
    int state = 0;
    long result = -1;

    for (;;) {
        char c;
        int count;

        count = read(fd, &c, 1);
        if (count <= 0)
            return -1;
        
        switch (state) {
        case 0:
            if (c == '\n')
                return -1;
            if (isspace(c))
                continue;
            if (isdigit(c)) {
                result = c - '0';
                state++;
                continue;
            }
            return -1;
        case 1:
            if (isspace(c))
                return result;
            if (!isdigit(c))
                return -1;
            result = result * 10 + (c - '0');
            break;
        }
    }
    return result;
}
/**
 * Read the results, which are a series of integers from the 
 * 'wc' program. */
struct wcresults
get_results(int fd)
{
    struct wcresults results;

    results.line_count = get_integer(fd);
    results.word_count = get_integer(fd);
    results.char_count = get_integer(fd);

    return results;
}

static unsigned
r_rand(unsigned *seed)
{
    static const unsigned a = 214013;
    static const unsigned c = 2531011;

    *seed = (*seed) * a + c;
    return (*seed)>>16 & 0x7fff;
}

long
word_count(const char *progname, const char *parms, const unsigned char *buf, size_t length)
{
    struct handles h;
    size_t offset = 0;
    struct wcresults results;
 

    h = my_spawn(progname, parms);
    if (h.child_stdin <= 0 || h.child_stdout <= 0) {
        fprintf(stderr, "[-] spawn failed\n");
        return -1;
    }

    /* keep writing until we've written the entire chunk */
    while (offset < length) {
        size_t count;

        count = write(h.child_stdin, buf+offset, length-offset);
        if (count == 0) {
            perror("write()");
            exit(1);
        }
        offset += count;
    }
    
    /* Close the handle, telling the child process that input has ended */
    close(h.child_stdin);

    /* Read the results from the child */
    results = get_results(h.child_stdout);
    
    close(h.child_stdout);
    close(h.child_stderr);
    wait(NULL);

    printf("%ld %ld %ld\n", results.line_count, results.word_count, results.char_count);

    return results.word_count;
}

int main(int argc, char *argv[])
{
    unsigned char *buf;
    unsigned seed = 0;
    unsigned length = 1024 * 1024;
    unsigned i;
    unsigned min = 0;
    unsigned max = length;
    unsigned min_diff, max_diff;
    long x, y;
    const char *progname = "./wc-fast-utf8";
    const char *parms = NULL;

    /* The first parameter is the program we are testing against 'wc' */
    if (argc < 2 || !is_executable(argv[1])) {
        fprintf(stderr, "[-] first parameter must be an executable program\n");
        return 1;
    } else 
        progname = argv[1];

    /* The second parameter is option parameters */
    if (argc >= 3) {
        parms = argv[2];
    } else
        parms = NULL;

    

    /* Create a buffer of deterministically random data. This will be the 
     * same data whenever we run this program, on any CPu, any OS, and
     * any compiler */
    buf = malloc(length);
    for (i=0; i<length; i++)
        buf[i] = r_rand(&seed);

    /* First, make sure there's a difference */
    x = word_count("wc", parms, buf, max);
    y = word_count(progname, parms, buf, max);
    if (x == y) {
        printf("no differnce!\n");
        return 0;
    }

    /* Now search for one past the first difference */
    min = 0;
    max = length;
    while (min < max) {
        unsigned half = (max - min)/2 + min;
        fprintf(stderr, "min=%u, half=%u, max=%u\n", min, half, max);
        x = word_count("wc", parms, buf, half);
        y = word_count(progname, parms, buf, half);
        if (x == y) {
            /* we went too far, so we need to go back */
            min = half + 1;
        } else {
            /* we didn't go far enough */
            max = half;
        }
    }
    max_diff = max;

    /* Now search for one before the first difference */
    min = 0;
    max = max_diff;
    while (min < max) {
        unsigned half = (max - min)/2 + min;
        fprintf(stderr, "min=%u, half=%u, max=%u\n", min, half, max);
        x = word_count("wc", parms, buf + half, max_diff - half);
        y = word_count("progname", parms, buf + half, max_diff - half);
        if (x == y) {
            /* we went too far, so we need to go back */
            max = half - 1;
        } else {
            /* we didn't go far enough */
            min = half;
        }
    }
    min_diff = min;

    /* print results */
    x = word_count("wc", parms, buf + min_diff, max_diff - min_diff);
    y = word_count(progname, parms, buf + min_diff, max_diff - min_diff);
    printf("Diff string: wc=%ld wc2=%ld \n", x, y);
    for (i=min_diff; i<max_diff; i++) {
        printf("0x%02x ", buf[i]);
    }
    printf("\n");
    


/*

    if (argc < 3 || argv[2][0] != '-') {
        fprintf(stderr, "[-] second parameter must be option like -m or -w\n");
        return 1;
    }

    if (argc < 4 || !is_exists(argv[3])) {
        fprintf(stderr, "[-] third parameter must be file\n");
        return 1;
    }
*/

    return 0;
}

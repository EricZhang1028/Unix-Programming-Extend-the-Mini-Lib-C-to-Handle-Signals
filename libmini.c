#include "libmini.h"

long errno;

#define WRAPPER_RETval(type)    errno = 0; if (ret < 0) { errno = -ret; return -1; } return ((type)ret);
#define WRAPPER_RETptr(type)    errno = 0; if (ret < 0) { errno = -ret; return NULL; } return ((type)ret);

// function wrapper implementation

ssize_t write(int fd, const void *buf, size_t count) {
    long ret = sys_write(fd, buf, count);
    WRAPPER_RETval(ssize_t);
}

void exit(int err_code) {
    sys_exit(err_code);
}

size_t strlen(const char *s) {
    size_t count = 0;
    while(*s++) count++;
    return count;
}

unsigned int alarm(unsigned int seconds) {
    long ret = sys_alarm(seconds);
    WRAPPER_RETval(unsigned int);
}

int pause(void) {
    long ret = sys_pause();
    WRAPPER_RETval(int);
}

int nanosleep(struct timespec *rqtp, struct timespec *rmtp) {
    long ret = sys_nanosleep(rqtp, rmtp);
    WRAPPER_RETval(int);
}

void *memset(void *s, int c, size_t n) {
    char *str = (char*)s;
    while(n--) {
        *(str++) = c;
    }
    return s;
}

// signal section

int sigemptyset(sigset_t *set) {
    if(set == NULL) {
        set_errno(EINVAL);
        return -1;
    }
    memset(set, 0, SIGSET_SIZE);
    return 0;
}

int sigaddset(sigset_t *set, int signum) {
    if(set == NULL || signum <= 0 || signum > SIG_MAX) {
        set_errno(EINVAL);
        return -1;
    }
    *set = *set | sigmask(signum);
    return 0;
}

int sigprocmask(int how, const sigset_t *set, sigset_t *oldset) {
    long ret = sys_rt_sigprocmask(how, set, oldset, SIGSET_SIZE);
    WRAPPER_RETval(int);
}

int sigpending(sigset_t *set) {
    long ret = sys_rt_sigpending(set, SIGSET_SIZE);
    WRAPPER_RETval(int);
}

int sigismember(const sigset_t *set, int signum) {
    if(set == NULL || signum <= 0 || signum > SIG_MAX){
        set_errno(EINVAL);
        return -1;
    }
    return ((*set & sigmask(signum)) != 0);
}

int sigaction(int signum, struct sigaction* act, struct sigaction* oldact) {
    act->sa_flags = act->sa_flags | SA_RESTORER;
	act->sa_restorer = __myrt;
	long ret = sys_rt_sigaction(signum, act, oldact, SIGSET_SIZE);
    WRAPPER_RETval(int);
}

sighandler_t signal(int signum, sighandler_t handler) {
    if(handler == NULL || signum <= 0 || signum > SIG_MAX) {
        set_errno(EINVAL);
        return SIG_ERR;
    }
    struct sigaction act, oldact;
    sigemptyset(&act.sa_mask);
    sigaddset(&act.sa_mask, signum);
    act.sa_handler = handler;
    act.sa_flags = SA_RESTART;
    if(sigaction(signum, &act, &oldact) < 0) {
        return SIG_ERR;
    }

    return oldact.sa_handler;
}

int sigfillset(sigset_t *set) {
    if(set == NULL) {
        set_errno(EINVAL);
        return -1;
    }
    *set = ~(*set & 0);
    return 0;
}

int sigdelset(sigset_t* set, int signum) {
    if(set == NULL || signum <= 0 || signum > SIG_MAX){
        set_errno(EINVAL);
        return -1;
    }

    *set = *set & ~(sigmask(signum));
    return 0;
}

// setjmp and longjmp section
void sigsetjmp(jmp_buf env) {
    sigset_t old;
    long ret = sys_rt_sigprocmask(SIG_BLOCK, NULL, &old, SIGSET_SIZE);
    if(ret < 0) {
        set_errno(-ret);
        perror("signal mask");
    }
    env->mask = old;
    return;
}

void siglongjmp(jmp_buf env) {
    long ret = sys_rt_sigprocmask(SIG_SETMASK, &(env->mask), NULL, SIGSET_SIZE);
    if(ret < 0){
        set_errno(-ret);
        perror("signal mask");
    }
    return;
}

// perror section

#define PERRMSG_MIN     0
#define PERRMSG_MAX     34

static const char *errmsg[] = {
        "Success",
        "Operation not permitted",
        "No such file or directory",
        "No such process",
        "Interrupted system call",
        "I/O error",
        "No such device or address",
        "Argument list too long",
        "Exec format error",
        "Bad file number",
        "No child processes",
        "Try again",
        "Out of memory",
        "Permission denied",
        "Bad address",
        "Block device required",
        "Device or resource busy",
        "File exists",
        "Cross-device link",
        "No such device",
        "Not a directory",
        "Is a directory",
        "Invalid argument",
        "File table overflow",
        "Too many open files",
        "Not a typewriter",
        "Text file busy",
        "File too large",
        "No space left on device",
        "Illegal seek",
        "Read-only file system",
        "Too many links",
        "Broken pipe",
        "Math argument out of domain of func",
        "Math result not representable"
};

void perror(const char *prefix) {
    const char *unknown = "Unknown";
    long backup = errno;
    if(prefix) {
        write(2, prefix, strlen(prefix));
        write(2, ": ", 2);
    }
    if(errno < PERRMSG_MIN || errno > PERRMSG_MAX) write(2, unknown, strlen(unknown));
    else write(2, errmsg[backup], strlen(errmsg[backup]));
    write(2, "\n", 1);
    return;
}
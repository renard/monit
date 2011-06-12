/*
 * Copyright (C) Tildeslash Ltd. All rights reserved.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License version 3.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * In addition, as a special exception, the copyright holders give
 * permission to link the code of portions of this program with the
 * OpenSSL library under certain conditions as described in each
 * individual source file, and distribute linked combinations
 * including the two.
 *
 * You must obey the GNU Affero General Public License in all respects
 * for all of the code used other than OpenSSL.  
 */


#include "Config.h"

#include <ctype.h>
#include <fcntl.h>
#include <signal.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>

#include "Str.h"
#include "Dir.h"
#include "File.h"
#include "List.h"
#include "system/Net.h"

#include "system/System.h"
#include "system/Command.h"


/**
 * Implementation of the Command and Process interfaces.
 *
 * Note: OS_signal installs a process wide SIGCHLD handler to wait for 
 * children to avoid creating zombie processes. The exception is if we
 * have a timeout handler we wait explicit on the subprocess to exit.
 *
 * @author Tildeslash Ltd. - www.tildeslash.com
 * @see www.mmonit.com
 * @file
 */



/* ----------------------------------------------------------- Definitions */


#define T Command_T
struct Process_T {
        T parent;
        pid_t pid;
        uid_t uid;
        gid_t gid;
        int status;
        int stdin_pipe[2];
        int stdout_pipe[2];
        int stderr_pipe[2];
        InputStream_T in;
        InputStream_T err;
        OutputStream_T out;
};
struct T {
        uid_t uid;
        gid_t gid;
        int timeout;
        List_T env;
        List_T args;
        char *working_directory;
        // Event handlers
        void *onExecAp;
        void *onTimeoutAp;
        void(*onExec)(Process_T P, void *ap);
        void(*onTimeout)(Process_T P, void *ap);
};
const char *Command_Path = "PATH=/bin:/usr/bin:/usr/local/bin:/opt/csw/bin:/usr/sfw/bin";


/* -------------------------------------------------------------- Process_T */


uid_t Process_getUid(Process_T P) {
        assert(P);
        return P->uid;
}


gid_t Process_getGid(Process_T P) {
        assert(P);
        return P->gid;
}


int Process_getTimeout(Process_T P) {
        assert(P);
        return P->parent->timeout;
}


const char *Process_getDir(Process_T P) {
        assert(P);
        return P->parent->working_directory;
        
}


pid_t Process_getPid(Process_T P) {
        assert(P);
        return P->pid;
}


int Process_isRunning(Process_T P) {
        assert(P);
        errno = 0;
        if (kill(P->pid, 0) < 0) {
                if (errno == ESRCH)
                        return false;
        }
        return true;
}


OutputStream_T Process_getOutStream(Process_T P) {
        assert(P);
        if (! P->out)
                P->out = OutputStream_new(P->stdin_pipe[1]);
        return P->out;
}


InputStream_T Process_getInputStream(Process_T P) {
        assert(P);
        if (! P->in)
                P->in = InputStream_new(P->stdout_pipe[0]);
        return P->in;
}


InputStream_T Process_getErrorStream(Process_T P) {
        assert(P);
        if (! P->err)
                P->err = InputStream_new(P->stderr_pipe[0]);
        return P->err;
}


void Process_terminate(Process_T P) {
        assert(P);
        kill(P->pid, SIGTERM);
}


void Process_kill(Process_T P) {
        assert(P);
        kill(P->pid, SIGKILL);
}


/* --------------------------------------------------------------- Private */


/* Search the env list and return the pointer to the name (in the list)
 if found, otherwise NULL */
static inline char *findEnv(T C, const char *name) {
        for (list_t p = C->env->head; p; p = p->next)
                if (Str_startsWith(p->e, name))
                        return p->e;
        return NULL;
}


/* Remove env variable and value identified by name */
static inline void removeEnv(T C, const char *name) {
        char *e = findEnv(C, name);
        if (e) {
                List_remove(C->env, e);
                FREE(e);
        }
}


/* Free each string in a list of strings */
static void freeStrings(List_T l) {
        while (List_length(l) > 0) {
                char *s = List_pop(l);
                FREE(s);
        }
}


/* Build the Command args list. The list represent the array sent
 to execv and the List contains the following entries: args[0]
 is the path to the program, the rest are arguments to the program */
static void buildArgs(T C, const char *path, const char *x, va_list ap) {
        freeStrings(C->args);
        List_append(C->args, Str_dup(path));
        va_list ap_copy;
        va_copy(ap_copy, ap);
        for (; x; x = va_arg(ap_copy, char *))
                List_append(C->args, Str_dup(x));
        va_end(ap_copy);
}


/* Build an array allocated on the heap from the given List. The last entry in the array is NULL */
static char **getArray(List_T L) {
        int i = 0;
        char **array = CALLOC(List_length(L) + 1, sizeof (char*));
        for (list_t p = L->head; p; p = p->next, i++)
                array[i] = p->e;
        array[i] = NULL;
        return array;
}


/* Create stdio pipes for communication between parent and child process */
static void createPipes(Process_T P) {
        if (pipe(P->stdin_pipe) < 0 ||
            pipe(P->stdout_pipe) < 0 ||
            pipe(P->stderr_pipe) < 0) {
                THROW(AssertException, "Bad file descriptors -- %s", System_getLastError());
        }
}


/* Setup stdio pipes in subprocess */
static void setupChildPipes(Process_T P) {
        close(P->stdin_pipe[1]);   // close write end
        if (P->stdin_pipe[0] != STDIN_FILENO) {
                if (dup2(P->stdin_pipe[0],  STDIN_FILENO) != STDIN_FILENO)
                        ERROR("Command: dup2(stdin) -- %s", System_getLastError());
                close(P->stdin_pipe[0]);
        }
        close(P->stdout_pipe[0]);  // close read end
        if (P->stdout_pipe[1] != STDOUT_FILENO) {
                if (dup2(P->stdout_pipe[1], STDOUT_FILENO) != STDOUT_FILENO)
                        ERROR("Command: dup2(stdout) -- %s", System_getLastError());
                close(P->stdout_pipe[1]);
        }
        close(P->stderr_pipe[0]);  // close read end
        if (P->stderr_pipe[1] != STDERR_FILENO) {
                if (dup2(P->stderr_pipe[1], STDERR_FILENO) != STDERR_FILENO)
                        ERROR("Command: dup2(stderr) -- %s", System_getLastError());
                close(P->stderr_pipe[1]);
        }
}


/* Setup stdio pipes in parent process for communication with the subprocess */
static void setupParentPipes(Process_T P) {
        close(P->stdin_pipe[0]);    // close read end
        Net_setNonBlocking(P->stdin_pipe[1]);
        close(P->stdout_pipe[1]);   // close write end
        Net_setNonBlocking(P->stdout_pipe[0]);
        close(P->stderr_pipe[1]);   // close write end
        Net_setNonBlocking(P->stderr_pipe[0]);
}


/* Close stdio pipes in parent process */
static void closeParentPipes(Process_T P) {
        close(P->stdin_pipe[1]);    // close write end
        close(P->stdout_pipe[0]);   // close read end
        close(P->stderr_pipe[0]);   // close read end
}


/* Close and destroy opened stdio streams */
static void closeStreams(Process_T P) {
        if (P->in) InputStream_free(&P->in);
        if (P->err) InputStream_free(&P->err);
        if (P->out) OutputStream_free(&P->out);
}


/* The Execute function. Note that we use vfork() rather than fork. Vfork has a
 special semantic in that the child process runs in the parent address space until 
 exec is called in the child. The child also run first and suspend the parent process
 until exec or exit is called */
static void Execute(T C) {
        char **env = NULL; 
        char **args = NULL;
        boolean_t haveHandlers = (C->onExec || C->onTimeout);
        struct Process_T P = {.parent = C};
        if (haveHandlers)
                createPipes(&P);
        if ((P.pid = vfork()) < 0)
                THROW(AssertException, "Cannot create a new subprocess -- %s", System_getLastError());
        // Child
        else if (P.pid == 0) { 
                if (C->working_directory) {
                        if (! Dir_chdir(C->working_directory)) {
                                ERROR("Command: subprocess cannot change working directory to '%s' -- %s", C->working_directory, System_getLastError());
                                _exit(errno);
                        }
                }
                if (C->uid)
                        P.uid = (setuid(C->uid) < 0) ? getuid() : C->uid;
                else
                        P.uid = getuid();
                if (C->gid)
                        P.gid = (setgid(C->gid) < 0) ? getgid() : C->gid;
                else
                        P.gid = getgid();
                setsid(); // Loose controlling terminal
                if (haveHandlers)
                        setupChildPipes(&P);
                else { // open stdio to /dev/null
                        for (int i = 0; i < 3; i++) {
                                close(i);
                                if (open("/dev/null", O_RDWR) != i) {
                                        ERROR("Command: Cannot open /dev/null -- %s", System_getLastError());
                                        _exit(errno);
                                }
                        }
                }
                // Close all descriptors except stdio
                int descriptors = getdtablesize();
                for (int i = 3; i < descriptors; i++)
                        close(i);
                // Unblock any signals and reset signal handlers
                sigset_t mask;
                sigemptyset(&mask);
                pthread_sigmask(SIG_SETMASK, &mask, NULL);
                signal(SIGINT, SIG_DFL);
                signal(SIGQUIT, SIG_DFL);
                signal(SIGABRT, SIG_DFL);
                signal(SIGTERM, SIG_DFL);
                signal(SIGPIPE, SIG_DFL);
                signal(SIGCHLD, SIG_IGN); // Default for SIGCHLD
                signal(SIGHUP, SIG_IGN);  // Ensure future opens won't allocate controlling TTYs
                // Execute the program
                env = getArray(C->env);
                args = getArray(C->args);
                execve(args[0], args, env);
                // Won't print to error log as descriptor was closed above, but will print to stderr and
                // if we have pipes, handlers can pick up the error
                ERROR("Command: '%s' failed to execute -- %s", args[0], System_getLastError());
                _exit(errno);
        }
        // Parent
        if (haveHandlers)
                setupParentPipes(&P);
        if (C->onExec)
                C->onExec(&P, C->onExecAp);
        if (C->onTimeout) {
                pid_t r;
                long timeout = C->timeout;
                while ((r = waitpid(P.pid, &P.status, WNOHANG)) == 0) { //Non-blocking wait
                        if (timeout > 0) {
                                sleep(1);
                                timeout--;
                        } else
                                break;
                }
                if (r == 0)
                        C->onTimeout(&P, C->onTimeoutAp);
        }
        FREE(args);
        FREE(env);
        if (haveHandlers) {
                closeParentPipes(&P);
                closeStreams(&P);
        }
}


/* ---------------------------------------------------------------- Public */


T Command_new(void) {
        T C;
        NEW(C);
        C->env = List_new();
        C->args = List_new();
        List_append(C->env, Str_dup(Command_Path));
        return C;
}


void Command_free(T *C) {
        assert(C && *C);
        freeStrings((*C)->args);
        List_free(&(*C)->args);
        freeStrings((*C)->env);
        List_free(&(*C)->env);
        FREE((*C)->working_directory);
        FREE(*C);
}


void Command_setUid(T C, uid_t uid) {
        assert(C);
        C->uid = uid;
}


uid_t Command_getUid(T C) {
        assert(C);
        return C->uid;
}


void Command_setGid(T C, gid_t gid) {
        assert(C);
        C->gid = gid;
}


gid_t Command_getGid(T C) {
        assert(C);
        return C->gid;
}


int Command_getTimeout(T C) {
        assert(C);
        return C->timeout;
}


void Command_setDir(T C, const char *dir) {
        assert(C);
        if (dir) { // Allow to set a NULL directory, meaning the calling process's current directory
                if (! File_isDirectory(dir))
                        THROW(AssertException, "The working directory '%s' is not a directory", dir);
                if (! File_isExecutable(dir))
                        THROW(AssertException, "The working directory '%s' is not accessible", dir);
        }
        FREE(C->working_directory);
        C->working_directory = Str_dup(dir);
        File_removeTrailingSeparator(C->working_directory);
}


const char *Command_getDir(Command_T C) {
        assert(C);
        return C->working_directory;
}


/* Env variables are stored in the environment list as "name=value" strings */
void Command_setEnv(Command_T C, const char *name, const char *value) {
        assert(C);
        assert(name);
        removeEnv(C, name);
        List_append(C->env, Str_cat("%s=%s", name, value ? value : ""));
}


void Command_setEnvString(T C, const char *env, ...) {
        assert(C);
        if (env && *env) {
                va_list ap;
                char *s, *n, *v, *t;
                va_start(ap, env);
                n = s = Str_vcat(env, ap);
                va_end(ap);
                while ((v = strchr(n, '='))) {
                        *(v++) = 0;
                        t = strchr(v, ';');
                        if (t)
                                *(t++) = 0;
                        Command_setEnv(C, Str_trim(n), Str_trim(v));
                        if (t)
                                n = t;
                        else
                                break;
                }
                FREE(s);
        }
}


/* Returns the value part from a "name=value" environment string */
const char *Command_getEnv(Command_T C, const char *name) {
        assert(C);
        assert(name);
        char *e = findEnv(C, name);
        if (e) {
                char *v = strchr(e, '=');
                        if (v)
                                return ++v;   
        }
        return NULL;
}


T Command_setCommand(T C, const char *path, const char *arg0, ...) {
        assert(C);
        if (! File_exist(path))
                THROW(AssertException, "File '%s' does not exist", path ? path : "null");
        va_list ap;
        va_start(ap, arg0);
        buildArgs(C, path, arg0, ap);
        va_end(ap);
        return C;
}


List_T Command_getCommand(T C) {
        assert(C);
        return C->args;
}


void Command_execute(T C) {
        assert(C);
        if (List_length(C->args) == 0)
                THROW(AssertException, "Command does not contain a program to execute");
        Execute(C);
}


/* -------------------------------------------------------- Event Handlers */


void Command_setOnExec(T C, void(*onExec)(Process_T P, void *ap), void *ap) {
        assert(C);
        assert(onExec);
        C->onExecAp = ap;
        C->onExec = onExec;
}


void Command_setOnTimeout(T C, int timeout, void(*onTimeout)(Process_T P, void *ap), void *ap) {
        assert(C);
        assert(timeout > 0);
        assert(onTimeout);
        C->timeout = timeout;
        C->onTimeoutAp = ap;
        C->onTimeout = onTimeout;
}

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


#ifndef PROCESS_INCLUDED
#define PROCESS_INCLUDED
#include <sys/types.h>
#include "InputStream.h"
#include "OutputStream.h"

/**
 * A <b>Process</b> represent an operating system process. A new Process 
 * object is created via Command_execute() and made available to event 
 * handlers registered on the Command. Event handlers do not run in a separate
 * thread and will block the caller (that is; the parent process). If both an 
 * <em>onExec</em> and an <em>onTimeout</em> handler is registered on the 
 * Command, then they are called in sequence, with the <em>onExec</em> handler
 * called first.
 * 
 * The subprocess represented by this Process does not have its own terminal 
 * or console. All its standard I/O (i.e. stdin, stdout, stderr) operations 
 * will be redirected to the parent Process where they can be accessed using 
 * the streams obtained using the methods Process_getOutStream(), 
 * Process_getInputStream(), and Process_getErrorStream(). The event handlers
 * can use these streams to feed input to and get output from the subprocess.
 * 
 * A subprocess is not killed, but rather the subprocess continues executing
 * asynchronously. 
 *
 * <h4>Environment</h4>
 * The Process does <em>not</em> inherit the environment from the calling 
 * process and has only a spartan PATH set by default; defined by 
 * Command_Path. Clients should call Command_setEnv() to set environment 
 * variables as needed <em>before</em> calling Command_execute()
 *
 * @see Command.h
 * @author Tildeslash Ltd. - www.tildeslash.com
 * @see www.mmonit.com
 * @file
 */


#define T Process_T
typedef struct T *T;


/** @name Properties */
//@{

/**
 * Returns the user id of the subprocess
 * @param P A Process object
 * @return The user id the subprocess
 */
uid_t Process_getUid(T P);


/**
 * Returns the group id of the subprocess
 * @param P A Process object
 * @return The group id the subprocess
 */
gid_t Process_getGid(T P);


/**
 * Returns the Process timeout. 
 * @param P A Process object
 * @return The number of seconds this Process has until exit before
 * the <code>onTimeout</code> handler is called if it is defined for
 * the Command. 0 means that there is no timeout.
 */
int Process_getTimeout(T P);


/**
 * Returns the working directory of the Process
 * @param P A Process object
 * @return The Process working directory
 */
const char *Process_getDir(T P);


/**
 * Returns the Process's identification number
 * @param P A Process object
 * @return The process identification number of the subprocess
 */
pid_t Process_getPid(T P);


/**
 * Returns true if the subprocess is running otherwise false
 * @param P A Process object
 * @return True if Process is running otherwise false
 */
int Process_isRunning(T P);


/**
 * Returns the output stream connected to the normal input of the subprocess. 
 * Output to the stream is piped into the standard input of the process 
 * represented by this Process object. 
 * @param P A Process object
 * @return The output stream connected to the normal input of the subprocess.
 */
OutputStream_T Process_getOutStream(T P);


/**
 * Returns the input stream connected to the normal output of the subprocess. 
 * The stream obtains data piped from the standard output of the process 
 * represented by this Process object.
 * @param P A Process object
 * @return The input stream connected to the normal output of the subprocess.
 */
InputStream_T Process_getInputStream(T P);


/**
 * Returns the input stream connected to the error output of the subprocess. 
 * The stream obtains data piped from the error output of the process 
 * represented by this Process object. 
 * @param P A Process object
 * @return The input stream connected to the error output of the subprocess.
 */
InputStream_T Process_getErrorStream(T P);


//@}


/**
 * Destroy the subprocess. The subprocess is destroyed by sending
 * it a termination signal (SIGTERM)
 * @param P A Process object
 */
void Process_terminate(T P);


/**
 * Kill the subprocess. The subprocess is destroyed by sending
 * it a termination signal (SIGKILL). While SIGTERM may be blocked
 * by a process, SIGKILL can not be blocked and will kill the process
 * @param P A Process object
 */
void Process_kill(T P);


#undef T
#endif


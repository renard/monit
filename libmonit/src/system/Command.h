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


#ifndef COMMAND_INCLUDED
#define COMMAND_INCLUDED
#include <system/Process.h>


/**
 * A <b>Command</b> creates operating system processes. Each Command instance
 * manages a collection of process attributes. The Command_execute() method
 * creates a new subprocess with those attributes and the method can be invoked
 * repeatedly from the same instance to create new subprocesses with identical
 * or related attributes.
 *
 * Modifying a Command's attributes will affect processes subsequently created,
 * but will never affect previously created processes or the calling process
 * itself.
 *
 * The following event handlers can be registered on a Command and will
 * be called after Command_execute() has been called. Each handler is called
 * with a Process object representing the operating system process. 
 * <em>Event handlers will block the calling process.</em> That is, if but 
 * only if event handlers are set for a Command, Command_execute() will block
 * until all event handlers have run. If no handlers are set on the Command,
 * Command_execute() will return imediately.
 * <ul>
 * <li><p>
 * <b>onExec</b> - register with Command_setOnExec(). Called <em>after</em>
 * the operating system program has been executed. A Process object 
 * representing the subprocess is made available to the handler and can be used
 * to communicate with the process.
 * </p></li>
 * <li><p>
 * <b>onTimeout</b> - register with Command_setOnTimeout(). Called if a timeout 
 * was set for the Command and if the subprocess does not exit within 
 * <code>timeout</code> seconds after it was started.
 * </p></li>
 * </ul>
 * Defining an <em>onExec</em> or an <em>onTimeout</em> handler for a Command
 * <em>imply</em> that the subprocess should perform a unit of work and then 
 * exit. Long running processes such as daemons and systems services should 
 * <em style="color:red">not</em> have event handlers attached.
 *
 * @see Process.h
 * @author Tildeslash Ltd. - www.tildeslash.com
 * @see www.mmonit.com
 * @file
 */


#define T Command_T
typedef struct T *T;
/** 
 * Default Path for Command: <code>PATH=/bin:/usr/bin:/usr/local/bin:/opt/csw/bin:/usr/sfw/bin</code>. 
 * May be overridden by Command_setEnv() 
 */
extern const char *Command_Path;


/**
 * Create a new Command object. Use Command_setCommand() to specify the
 * operating system program and arguments to execute. 
 * @return A new Command object
 */
T Command_new(void);


/**
 * Destroy A Command object and release allocated resources. Call this
 * method to release a Command object allocated with Command_new()
 * @param C A Command object reference
 */
void Command_free(T *C);


/** @name Properties */
//@{

/**
 * Set the user id the Command should switch to on exec. If not set, the uid of 
 * the calling process is used. Note that this process must run with super-user
 * privileges for the subprocess to be able to switch uid
 * @param C A Command object
 * @param uid The user id the Command should switch to when executed
 */
void Command_setUid(T C, uid_t uid);


/**
 * Returns the Command should switch to on exec. 
 * @param C A Command object
 * @return The user id the Command should switch to when executed
 */
uid_t Command_getUid(T C);


/**
 * Set the group id the Command should switch to on exec. If not set, the gid of 
 * the calling process is used. Note that this process must run with super-user
 * privileges for the subprocess to be able to switch gid
 * @param C A Command object
 * @param gid The group id the Command should switch to when executed
 */
void Command_setGid(T C, gid_t gid);


/**
 * Returns the group id the Command should switch to on exec. 
 * @param C A Command object
 * @return The group id the Command should switch to when executed
 */
gid_t Command_getGid(T C);


/**
 * Returns the Process timeout. Default is no timeout. The timeout
 * is set with Command_setOnTimeout().
 * @param C A Command object
 * @return The number of seconds to wait before the Process will be
 * destroyed. 0 means that there is no timeout.
 * @see Command_setOnTimeout()
 */
int Command_getTimeout(T C);


/**
 * Set the working directory for the subprocess. If directory cannot be changed
 * the subprocess will exit
 * @param C A Command object
 * @param dir The working directory for the Process
 * @exception AssertException if the directory does not exist or is not accessible
 */
void Command_setDir(T C, const char *dir);


/**
 * Returns the working directory for the Process. Unless previously
 * set, the returned value is NULL, meaning the calling process's current
 * directory
 * @param C A Command object
 * @return The working directory for the Process or NULL meaning the calling
 * process's current directory
 */
const char *Command_getDir(T C);


/**
 * Set or replace the environment variable identified by <code>name</code>.
 * The Process does <em>not</em> inherit the environment from the calling 
 * process and has only a spartan PATH set by default.
 * @param C A Command object
 * @param name The environment variable to set or replace
 * @param value The value
 */
void Command_setEnv(T C, const char *name, const char *value);


/**
 * Set or replace the environment variable(s) specified in <code>env</code>.
 * The <code>env</code> string is expected to be on a name=value; format
 * and each name=value pair must be separated with ';'. This is a
 * convenience function, wrapping Command_setEnv() and is rather inefficient.
 * @param C A Command object
 * @param env An environment string containing name=value pairs separated 
 * with ';'. Example: <code>PATH=/usr/bin; SHELL=/bin/bash;</code>
 * @see Command_setEnv()
 */
void Command_setEnvString(T C, const char *env, ...);


/**
 * Returns the value of the environment variable identified by 
 * <code>name</code>. 
 * @param C A Command object
 * @param name The environment variable to get the value of
 * @return The value for name or NULL if name was not found
 */
const char *Command_getEnv(T C, const char *name);


/**
 * Set a new operating system program with arguments to execute. The 
 * <code>arg0</code> argument is the first argument in a sequence of 
 * arguments to the program. The arguments list can be thought of as
 * arg0, arg1, ..., argn. Together they describe a list of one or more
 * pointers to null-terminated strings that represent the argument list 
 * available to the executed program specified in <code>path</code>. The 
 * list of arguments <em style="color:red">must</em> be terminated by a
 * NULL pointer. Example:
 * <pre>
 * Command_setCommand("/bin/ls", NULL)
 * Command_setCommand("/bin/ls", "-lrt", NULL)
 * Command_setCommand("/bin/sh", "-c", "ps -aef|egrep mmonit", NULL)
 * </pre>
 * @param C A Command object
 * @param path A string containing the path to the program to execute
 * @param arg0 The first argument in a sequence of arguments. The last value
 * in the arguments list <strong>must</strong> be NULL.
 * @exception AssertException if the program does not exist or cannot be
 * executed
 * @return This Command object
 */
T Command_setCommand(T C, const char *path, const char *arg0, ...);


/**
 * Returns the operating system program with arguments to be executed by 
 * this Command. The first element in the list is the path to the program
 * and subsequent elements are arguments to the program. Elements in the 
 * list are C-strings. 
 * @param C A Command object
 * @return A list with the operating system program with arguments to 
 * execute. The first element in the list is the program.
 */
List_T Command_getCommand(T C);


//@}


/** 
 * Executes the Command in a new subprocess. Note that if event handlers are
 * attached to this Command, this call will block until handlers are done.
 * @param C A Command object
 * @exception AssertException if no program has been set via 
 * Command_setCommand() or if a system error occur
 */
void Command_execute(T C);


/** @name Event handlers */
//@{

/**
 * Set the event handler to call when the subprocess execute. The handler is
 * passed a Process object representing the subprocess and an optional 
 * application specific pointer.
 * @param C A Command object
 * @param onExec The event handler to call after the subprocess execute
 * @param ap An application-specific pointer. If such a pointer is 
 * not needed, just use NULL
 * @see Process.h
 */
void Command_setOnExec(T C, void(*onExec)(Process_T P, void *ap), void *ap);


/**
 * Set the event handler to call on timeout. If the subprocess has not 
 * exited within <code>timeout</code> seconds the <code>onTimeout</code>
 * method is called. The handler is passed a Process object representing
 * the subprocess and an optional application specific pointer.
 * @param C A Command object
 * @param timeout Number of seconds to wait for the subprocess to exit
 * until onTimeout is called
 * @param onTimeout The event handler to call on timeout
 * @param ap An application-specific pointer. If such a pointer is 
 * not needed, just use NULL
 * @exception AssertException if timeout is less than or equal to zero
 * @see Command_getTimeout()
 * @see Process.h
 */
void Command_setOnTimeout(T C, int timeout, void(*onTimeout)(Process_T P, void *ap), void *ap);


//@}


#undef T
#endif


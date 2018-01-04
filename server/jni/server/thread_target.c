/**
 * This file is part of the Cachegrab server.
 *
 * Copyright (C) 2017 NCC Group
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Cachegrab.  If not, see <http://www.gnu.org/licenses/>.
 */

/*
 Version 1.0
 Keegan Ryan, NCC Group
*/

#include "capture.h"

#include <sched.h>
#include <unistd.h>
#include <sys/wait.h>

#include "scope.h"

#define ENV_NAME "CACHEGRAB_NAME"
#define ENV_CMDBUF "CACHEGRAB_COMMAND_BUF"
#define ENV_DEBUG "CACHEGRAB_DEBUG"

bool get_target_args (struct target_args *arg, struct capture_config *c, int cpu) {
  arg->cpu = cpu;
  arg->command = c->command;
  arg->name = c->name;
  arg->cbuf = c->cbuf;
  arg->debug = c->debug;
  arg->out_stream = NULL;
  arg->out_len = 0;
  arg->err_stream = NULL;
  arg->err_len = 0;
  return true;
}

/**
 * Get all the data from a pipe.
 *
 * Returns a pointer to an array of size LEN, or NULL otherwise.
 */
uint8_t* get_pipe (int fd, size_t *len) {
  ssize_t rc;
  uint8_t buf[256];
  size_t stream_size = 0;
  size_t stream_offs = 0;
  uint8_t *tmp;
  uint8_t *ret = NULL;

  while ((rc = read(fd, buf, sizeof(buf))) > 0) {
    stream_size += rc;
    if (ret) {
      tmp = realloc(ret, stream_size);
    } else {
      tmp = malloc(stream_size);
    }
    if (!tmp)
      goto err;

    ret = tmp;
    memcpy(ret + stream_offs, buf, rc);
    stream_offs += rc;
  }

  *len = stream_size;
  return ret;
 err:
  if (ret)
    free(ret);
  *len = 0;
  return NULL;
}

bool run_command (struct target_args* arg) {
  // Actually execute the target command.
  int out_pipe[2];
  int err_pipe[2];

  if (0 != pipe(out_pipe)) {
    return false;
  }
  if (0 != pipe(err_pipe)) {
    close(out_pipe[0]);
    close(out_pipe[1]);
    return false;
  }

  pid_t pid = fork();
  if (pid == -1) {
    // Error
    return false;
  } else if (pid == 0) {
    // Child
    if (dup2(out_pipe[1], STDOUT_FILENO) < 0)
      exit(-1);
    if (dup2(err_pipe[1], STDERR_FILENO) < 0)
      exit(-1);
    close(out_pipe[0]);
    close(out_pipe[1]);
    close(err_pipe[0]);
    close(err_pipe[1]);

    setenv("LD_LIBRARY_PATH", "/data/local/tmp", 1);
    setenv(ENV_NAME, arg->name, 1);
    setenv(ENV_CMDBUF, arg->cbuf, 1);
    setenv(ENV_DEBUG, arg->debug ? "y" : "n", 1);
    
    char* argv[] = {"/system/bin/sh", "-c", arg->command, NULL};
    execv("/system/bin/sh", argv);
    exit(-1);
  } else {
    // Parent
    close(out_pipe[1]);
    close(err_pipe[1]);
    sched_yield();
    int status;
    waitpid(pid, &status, 0);
    arg->status = WEXITSTATUS(status);
    arg->out_stream = get_pipe(out_pipe[0], &arg->out_len);
    arg->err_stream = get_pipe(err_pipe[0], &arg->err_len);

    close(out_pipe[0]);
    close(err_pipe[0]);
  }
  return true;
}

void* target_func (void* p_arg) {
  struct target_args *arg = p_arg;
  
  // Child
  if (!set_cpu(arg->cpu)) {
      set_shared_status(arg->shared, CG_CAPTURE_ERR);
  }

  // set up target
  signal_ready(arg->shared);

  if (get_shared_status(arg->shared) == CG_OK) {
    // do target things
    if (!run_command(arg)) {
      set_shared_status(arg->shared, CG_CAPTURE_ERR);
    }

    // Indicate to stalling threads that they may stop spinning
    arg->shared->target_finished = false;
  }
  return NULL;
}

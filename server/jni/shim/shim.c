/**
 * This file is part of the Cachegrab TEE library shim.
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

#include <dlfcn.h>
#include <fcntl.h>
#include <inttypes.h>
#include <pthread.h>
#include <stdio.h>
#include <string.h>

#include <driver.h>

#include "utils.h"

#define ENV_NAME "CACHEGRAB_NAME"
#define ENV_CMDBUF "CACHEGRAB_COMMAND_BUF"
#define ENV_DEBUG "CACHEGRAB_DEBUG"

#define TAG "CACHEGRAB_SHIM: "

int QSEECom_start_app (void **handle, char *path, char *name, uint32_t size);
int QSEECom_shutdown_app (void **handle);
int QSEECom_send_cmd(void *handle,
		     void *cbuf, uint32_t clen,
		     void *rbuf, uint32_t rlen);
int QSEECom_send_modified_cmd (void *handle,
			       void *cbuf, uint32_t clen,
			       void *rbuf, uint32_t rlen,
			       void *info);
int QSEECom_set_bandwidth (void *handle, bool high);

bool load_funcs (void);
void cg_log (const char* fmt, ...);
void cg_buf (const char* name, uint8_t* buf, uint32_t len);

int (*QSEECom_start_app_orig) (void **, char*, char*, uint32_t);
int (*QSEECom_shutdown_app_orig) (void **handle);
int (*QSEECom_send_cmd_orig) (void *handle,
			      void *cbuf, uint32_t clen,
			      void *rbuf, uint32_t rlen);
int (*QSEECom_send_modified_cmd_orig) (void *handle,
				       void *cbuf, uint32_t clen,
				       void *rbuf, uint32_t rlen,
				       void *info);
int (*QSEECom_set_bandwidth_orig) (void *handle, bool high);

static int fd;
static char* target_name;
static uint8_t* target_cbuf;
static size_t target_clen;
static void* target_handle;
static int valid = 0;
static int cg_debug = 0;
static int old_policy = 0;
static struct sched_param old_param;
static int old_sched_valid = 0;

// Get GCC to call this function when library is loaded
__attribute__((constructor))
void init (void) {
  // Initialize shim variables
  int funcs_valid = 0;
  int fd_valid = 0;
  int name_valid = 0;
  int cbuf_valid = 0;
  
  // Initialize debug
  char* dbg = getenv(ENV_DEBUG);
  if (dbg && (0 == strcmp(dbg, "y") || 0 == strcmp(dbg, "Y"))) {
    cg_debug = 1;
  }

  // Initialize functions
  if (load_funcs())
    funcs_valid = 1;

  // Initialize driver file
  fd = open(DEVFILE, O_RDWR);
  if (fd >= 0) {
    fd_valid = 1;
  }

  // Initialize target name
  target_name = getenv(ENV_NAME);
  if (target_name != NULL) {
    name_valid = 1;
  }

  // Initialize command buffer
  char* cbuf = getenv(ENV_CMDBUF);
  if (cbuf != NULL) {
    size_t len = strlen(cbuf);
    target_cbuf = (uint8_t*)malloc(len);
    if (target_cbuf) {
      size_t rv = hex_to_bytes(target_cbuf, cbuf, len);
      target_clen = rv;
      cbuf_valid = 1;
    }
  }
  
  if (fd_valid && funcs_valid && name_valid && cbuf_valid)
    valid = 1;

  if (valid && cg_debug)
    cg_log("Successfully initialized");
}

bool load_funcs () {
  void *lib = dlopen("/vendor/lib64/libQSEEComAPI.so", RTLD_LAZY);
  if (!lib)
    return false;
  
  cg_log("Successfully opened library.");

  QSEECom_start_app_orig =			\
    (int (*) (void **, char*, char*, uint32_t)) \
    dlsym(lib, "QSEECom_start_app");
  if (!QSEECom_start_app_orig)
    return false;
  cg_log("Successfully hooked QSEECom_start_app");

  QSEECom_shutdown_app_orig =			\
    (int (*) (void **))				\
    dlsym(lib, "QSEECom_shutdown_app");
  if (!QSEECom_shutdown_app_orig)
    return false;
  cg_log("Successfully hooked QSEECom_shutdown_app");

  QSEECom_send_cmd_orig =					\
    (int (*) (void*, void*, uint32_t, void*, uint32_t))		\
    dlsym(lib, "QSEECom_send_cmd");
  if (!QSEECom_send_cmd_orig)
    return false;
  cg_log("Successfully hooked QSEECom_send_cmd");

  QSEECom_send_modified_cmd_orig =				\
    (int (*) (void*, void*, uint32_t, void*, uint32_t, void*))	\
    dlsym(lib, "QSEECom_send_modified_cmd");
  if (!QSEECom_send_modified_cmd_orig)
    return false;
  cg_log("Successfully hooked QSEECom_send_modified_cmd");

  QSEECom_set_bandwidth_orig =			\
    (int (*) (void*, bool))			\
    dlsym(lib, "QSEECom_set_bandwidth");
  if (!QSEECom_set_bandwidth_orig)
    return false;
  cg_log("Successfully hooked QSEECom_set_bandwidth");
  
  return true;
}

void cg_log (const char* fmt, ...) {
  if (!cg_debug)
    return;

  fprintf(stderr, TAG);

  va_list args;
  va_start(args,fmt);
  vfprintf(stderr, fmt, args);
  va_end(args);
 
  fprintf(stderr, "\n");
}

void cg_buf (const char* name, uint8_t* buf, uint32_t len) {
  if (!cg_debug)
    return;
  
  cg_log("%s:", name);
  for (uint32_t i = 0; i < len; i++) {
    if (i % 16 == 0)
      fprintf(stderr, TAG);
    fprintf(stderr, "%02X ", buf[i]);
    if ((i + 1) % 16 == 0)
      fprintf(stderr, "\n");
  }
  if (len % 16 != 0)
    fprintf(stderr, "\n");
}

void start_intercept() {
  struct sched_param param;

  pthread_t this_thread = pthread_self();
  int ret = pthread_getschedparam(this_thread, &old_policy, &old_param);
  if (ret == 0) {
    old_sched_valid = 1;
    param.sched_priority = sched_get_priority_max(SCHED_FIFO);
    pthread_setschedparam(this_thread, SCHED_FIFO, &param);
  }

  ioctl(fd, CG_SCOPE_ACTIVATE, NULL);
}

void stop_intercept() {
  ioctl(fd, CG_SCOPE_DEACTIVATE, NULL);

  if (!old_sched_valid)
    return;
  pthread_t this_thread = pthread_self();
  int ret = pthread_setschedparam(this_thread, old_policy, &old_param);
  if (ret < 0)
    return;
  old_sched_valid = 0;
}

int QSEECom_start_app (void **handle, char *path, char *name, uint32_t size) {
  cg_log("Starting path %s and name %s with size %lu", path, name, size);
  int ret = QSEECom_start_app_orig(handle, path, name, size);
  cg_log("QSEECom_start_app returned handle %p", *handle);
  if (valid &&
      *handle != NULL &&
      0 == strcmp(name, target_name) &&
      target_handle == NULL) {
    target_handle = *handle;
  }
  return ret;
}

int QSEECom_shutdown_app (void **handle) {
  cg_log("Shutting down handle %p", *handle);
  if (valid &&
      target_handle != NULL &&
      target_handle == *handle) {
    target_handle = NULL;
  }
  return QSEECom_shutdown_app_orig(handle);
}

int QSEECom_send_cmd(void *handle,
		     void *cbuf, uint32_t clen,
		     void *rbuf, uint32_t rlen) {
  bool intercepting = false;
  int ret;
  cg_log("Sending command to handle %p", handle);
  cg_buf("CBUF", cbuf, clen);
  if (valid &&
      target_handle != NULL &&
      clen >= target_clen &&
      0 == memcmp(cbuf, target_cbuf, target_clen)) {
    cg_log("Intercepting command");
    intercepting = true;
  }
  if (intercepting) {
    start_intercept();
  }
  ret = QSEECom_send_cmd_orig(handle, cbuf, clen, rbuf, rlen);
  if (intercepting) {
    stop_intercept();
  }
  return ret;
}

int QSEECom_send_modified_cmd (void *handle,
			       void *cbuf, uint32_t clen,
			       void *rbuf, uint32_t rlen,
			       void *info) {
  bool intercepting = false;
  int ret;
  cg_log("Sending modified command to handle %p", handle);
  cg_buf("CBUF", cbuf, clen);
  if (valid &&
      target_handle != NULL &&
      clen >= target_clen &&
      0 == memcmp(cbuf, target_cbuf, target_clen)) {
    cg_log("Intercepting command");
    intercepting = true;
  }
  if (intercepting) {
    start_intercept();
  }
  ret = QSEECom_send_modified_cmd_orig(handle, cbuf, clen, rbuf, rlen, info);
  if (intercepting) {
    stop_intercept();
  }
  return ret;
}

int QSEECom_set_bandwidth (void *handle, bool high) {
  cg_log("Setting bandwidth on handle %p to %s",
	 handle,
	 high ? "true" : "false");
  return QSEECom_set_bandwidth_orig(handle, high);
}

/**
 * geoip2.c -- implementation of GeoIP2
 *    ______      ___
 *   / ____/___  /   | _____________  __________
 *  / / __/ __ \/ /| |/ ___/ ___/ _ \/ ___/ ___/
 * / /_/ / /_/ / ___ / /__/ /__/  __(__  |__  )
 * \____/\____/_/  |_\___/\___/\___/____/____/
 *
 * The MIT License (MIT)
 * Copyright (c) 2009-2020 Gerardo Orellana <hello @ goaccess.io>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#if HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <stdio.h>

#include "geoip1.h"

#include "error.h"

/* Determine if we have a valid geoip resource.
 *
 * If the geoip resource is NULL, 0 is returned.
 * If the geoip resource is valid and malloc'd, 1 is returned. */
int
is_geoip_resource (void) {
  pid_t pid;
  int res;

  pid = fork ();
  if (pid == 0) {
    FILE *f = fopen ("/dev/null", "w");
    dup2 (fileno (f), STDOUT_FILENO);

    execlp ("nali", "nali", "-v", (char *)NULL);

    fclose (f);
    exit (1);
  }

  wait (&res);
  return res == 0 ? 1 : 0;
}

/* Free up GeoIP resources */
void
geoip_free (void) {}

/* Open the given GeoIP2 database.
 *
 * On error, it aborts.
 * On success, a new geolocation structure is set. */
void
init_geoip (void) {
  if (!is_geoip_resource ())
    FATAL ("Unable to find nali-cli program");
}

/* Look up an IP address that is passed in as a null-terminated string.
 *
 * On error, it aborts.
 * If no entry is found, 1 is returned.
 * On success, res is set and 0 is returned. */
static int
geoip_lookup (char * res, const char *ip) {
  int fds[2];
  pid_t pid;
  char buf[1024];
  ssize_t len;
  const char *start, *end;

  if (pipe (fds) == -1)
    return 1;

  pid = fork ();
  if (pid == 0) {
    if (dup2 (fds[1], STDOUT_FILENO) == -1) {
      exit(1);
    }
    close (fds[0]);
    close (fds[1]);

    execlp ("nali", "nali", ip, (char *)NULL);
    exit (1);
  }
  close (fds[1]);

  len = read (fds[0], buf, sizeof (buf) - 1);
  if (len <= 0)
    return 1;
  buf[len] = 0;
  start = strstr(buf, " [");
  if (start == NULL)
    return 1;
  start += 2;
  end = strstr(buf, "]");
  if (end == NULL)
    return 1;
  strncpy(res, start, end - start);
  return 0;
}

/* Set country data by record into the given `location` buffer */
void
geoip_get_country (GO_UNUSED const char *ip, char *location, GO_UNUSED GTypeIP type_ip) {
  strcpy (location, "test");
}

/* A wrapper to fetch the looked up result and set the continent. */
void
geoip_get_continent (GO_UNUSED const char *ip, char *location, GO_UNUSED GTypeIP type_ip) {
  strcpy (location, " ");
}

/* Entry point to set GeoIP location into the corresponding buffers,
 * (continent, country, city).
 *
 * On error, 1 is returned
 * On success, buffers are set and 0 is returned */
int
set_geolocation (char *host, char *continent, char *country, char *city) {
  int res;

  if (!is_geoip_resource ())
    return 1;

  strcpy (continent, " ");
  strcpy (country, " ");
  res = geoip_lookup (city, host);
  if (res == 1)
    return 1;

  return 0;
}

/* $Id: wi_linux.c,v 1.4 2004/12/03 18:29:41 benny Exp $ */
/*-
 * Copyright (c) 2003,2004 Benedikt Meurer <benny@xfce.org>
 * Copyright (c) 2004 An-Cheng Huang <pach@cs.cmu.edu>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#if defined(__linux__)

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>

/* Require wireless extensions */
#include <linux/wireless.h> 

#include <wi.h>

struct wi_device
{
  char interface[WI_MAXSTRLEN];
  int socket;
};

struct wi_device *
wi_open(const char *interface)
{
  struct wi_device *device;
  int sock;

  g_return_val_if_fail(interface != NULL, NULL);

  if ((sock = socket(PF_INET, SOCK_DGRAM, 0)) < 0) {
    return(NULL);
  }

  device = g_new0(struct wi_device, 1);
  device->socket = sock;
  g_strlcpy(device->interface, interface, WI_MAXSTRLEN);

  return(device);
}

void
wi_close(struct wi_device *device)
{
  g_return_if_fail(device != NULL);

  close(device->socket);
  g_free(device);
}

int
wi_query(struct wi_device *device, struct wi_stats *stats)
{
  char buffer[1024];
  char *bp;
  double link;
  long level;
  int result;

  struct iwreq wreq;
  struct iw_statistics wstats;
  char essid[IW_ESSID_MAX_SIZE + 1];

  FILE *fp;

  g_return_val_if_fail(device != NULL, WI_INVAL);
  g_return_val_if_fail(stats != NULL, WI_INVAL);

  /* FIXME */
  g_strlcpy(stats->ws_vendor, "Unknown", WI_MAXSTRLEN);

  /* Set interface name */
  strncpy(wreq.ifr_name, device->interface, IFNAMSIZ);

  /* Get ESSID */
  wreq.u.essid.pointer = (caddr_t) essid;
  wreq.u.essid.length = IW_ESSID_MAX_SIZE + 1;
  wreq.u.essid.flags = 0;
  if ((result = ioctl(device->socket, SIOCGIWESSID, &wreq) < 0)) {
    g_strlcpy(stats->ws_netname, "", WI_MAXSTRLEN);
  } else {
    /* ESSID is possibly NOT null terminated but we know its length */
    essid[wreq.u.essid.length] = 0;
    g_strlcpy(stats->ws_netname, essid, WI_MAXSTRLEN);
  }

  /* Get bit rate */
  if ((result = ioctl(device->socket, SIOCGIWRATE, &wreq) < 0)) {
    stats->ws_rate = 0;
  } else {
    stats->ws_rate = wreq.u.bitrate.value;
  }

#if WIRELESS_EXT > 11
  /* Get interface stats through ioctl */
  wreq.u.data.pointer = (caddr_t) &wstats;
  wreq.u.data.length = 0;
  wreq.u.data.flags = 1;
  if ((result = ioctl(device->socket, SIOCGIWSTATS, &wreq)) < 0) {
    return(WI_NOSUCHDEV);
  }
  level = wstats.qual.level;
  link = wstats.qual.qual;
#else /* WIRELESS_EXT <= 11 */
  /* Get interface stats through /proc/net/wireless */
  if ((fp = fopen("/proc/net/wireless", "r")) == NULL) {
    return(WI_NOSUCHDEV);
  }

  for (;;) {
    if (fgets(buffer, sizeof(buffer), fp) == NULL)
      return(WI_NOSUCHDEV);

    if (buffer[6] == ':') {
      buffer[6] = '\0';
      for (bp = buffer; isspace(*bp); ++bp);

      if (strcmp(bp, device->interface) != 0)
        continue;

      /* we found our device, read the stats now */
      bp = buffer + 12;
      link = strtod(bp, &bp);
      level = strtol(bp + 1, &bp, 10);
      break;
    }
  }
  fclose(fp);
#endif

  /* check if we have a carrier signal */
  /* FIXME: does 0 mean no carrier? */
  if (level <= 0)
    return(WI_NOCARRIER);

  /* calculate link quality */
  if (link <= 0)
    stats->ws_quality = 0;
  else {
    /* thanks to google for this hint */
    stats->ws_quality = (int)rint(log(link) / log(92.0) * 100.0);
  }

  return(WI_OK);
}

#endif  /* !defined(__linux__) */

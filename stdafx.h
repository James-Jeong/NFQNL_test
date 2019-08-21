#pragma once
#ifndef STDAFX_H
#define STDAFX_H

#include <iostream>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <unistd.h>
#include <netinet/in.h>
#include <linux/types.h>
#include <linux/netfilter.h>		/* for NF_ACCEPT */
#include <errno.h>
#include <libnetfilter_queue/libnetfilter_queue.h>

#include <vector>

#include <libnet.h>
#include "libnet-asn1.h"
#include "libnet-functions.h"
#include "libnet-headers.h"
#include "libnet-macros.h"
#include "libnet-structures.h"
#include "libnet-types.h"

using namespace std;

#endif

#pragma once

#include "lib/exec_handle.h"

/* public */

struct llcm_exec_handle;

struct llcm_routine {
    void *(*poll)(void *arg0, struct llcm_exec_handle *);
    void *arg0;
};

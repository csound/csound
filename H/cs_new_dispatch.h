#pragma once

#include "csound.h"
#include "csoundCore_common.h"

void dag_build(CSOUND *csound, INSDS *chain);
void dag_reinit(CSOUND *csound);
int dag_get_task(CSOUND *csound, int index, int numThreads, int next_task);
int dag_end_task(CSOUND *csound, int task);
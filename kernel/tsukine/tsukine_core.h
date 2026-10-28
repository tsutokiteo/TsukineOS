#ifndef _TSUKINE_CORE_H
#define _TSUKINE_CORE_H

#include <linux/types.h>

int tsukine_io_handler(uint32_t subcmd, uint8_t *payload);
int tsukine_sync_handler(uint32_t subcmd, uint8_t *payload);
int tsukine_query_handler(uint32_t subcmd, uint8_t *payload);

#endif

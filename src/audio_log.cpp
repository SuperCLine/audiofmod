#include "stdafx.h"
#include "audio_log.h"


__BEGIN_NAMESPACE

static interface_logmgr* g_audio_logger = 0;
void set_audio_logger(interface_logmgr* log)
{
    g_audio_logger = log;
}

interface_logmgr* get_audio_logger(void)
{
    return g_audio_logger;
}

void audio_log(ELogType type, const char* tag, const char* log)
{
    if (get_audio_logger())
    {
        get_audio_logger()->log(type, tag, log);
    }
}

__END_NAMESPACE

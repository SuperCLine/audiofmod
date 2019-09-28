/*------------------------------------------------------------------------------
|
| COPYRIGHT (C) 2018 - 2026 All Right Reserved
|
| FILE NAME  : \audiofmod\inc\private\audio_log.h
| AUTHOR     : CLine
| PURPOSE    :
|
| SPEC       :
|
| MODIFICATION HISTORY
|
| Ver      Date            By              Details
| -----    -----------    -------------   ----------------------
| 1.0      2019-9-28      CLine           Created
|
+-----------------------------------------------------------------------------*/


#ifndef _AUDIO_LOG_H_
#define _AUDIO_LOG_H_


__BEGIN_NAMESPACE


void set_audio_logger(interface_logmgr* log);
interface_logmgr* get_audio_logger(void);

void audio_log(ELogType type, const char* tag, const char* log);

template<typename... Args>
void audio_logf(ELogType type, const char* tag, const char* format, Args... args)
{
    if (get_audio_logger())
    {
        get_audio_logger()->logf(type, tag, format, args...);
    }
}


__END_NAMESPACE


#endif //_AUDIO_LOG_H_
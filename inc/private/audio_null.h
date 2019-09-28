/*------------------------------------------------------------------------------
|
| COPYRIGHT (C) 2018 - 2026 All Right Reserved
|
| FILE NAME  : \audiofmod\inc\private\audio_null.h
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


#ifndef _AUDIO_NULL_H_
#define _AUDIO_NULL_H_


__BEGIN_NAMESPACE


class audio_null : public core_interface, public interface_audio
{
public:
    audio_null(void);
    virtual ~audio_null(void);

    virtual bool init(const char* config_file, interface_logmgr* log);
    virtual void destroy(void);
    virtual void update(float32 tick);
    virtual bool load(const char* config_file);
    virtual audio_id_type play(const char* name, float32* pos = 0, float32* velocity = 0, void* user_data = 0);
    virtual void stop(audio_id_type id, bool immidiate = false);
    virtual void stop(ESoundType type, bool immidiate = false);
    virtual void pause(audio_id_type id);
    virtual void pause(ESoundType type);
    virtual void resume(audio_id_type id);
    virtual void resume(ESoundType type);
    virtual void set_volume(ESoundType type, float32 vol);
    virtual void set_switch(ESoundType type, bool bswitch);
    virtual void mute(ESoundType type, bool bmute);
    virtual bool apply_reverb(audio_id_type id, const char* name);
    virtual bool apply_reverb(const char* name);
    virtual const char* get_name(audio_id_type id);
    virtual bool is_valid(audio_id_type id);
    virtual bool set_attr(audio_id_type id, float32* pos, float32* velocity = 0, float32* orientation = 0);
    virtual bool get_attr(audio_id_type id, float32* pos, float32* velocity = 0, float32* orientation = 0);
    virtual bool set_listener_attr(float32* pos, float32* forward, float32* up = 0, float32* velocity = 0);
    virtual void add_listener(audio_listener* listener);
    virtual void set_user_data(audio_id_type id, void* data);
    virtual void* get_user_data(audio_id_type id);

    static audio_null* singleton(void);
};


__END_NAMESPACE


#endif //_AUDIO_NULL_H_
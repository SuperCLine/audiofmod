/*------------------------------------------------------------------------------
|
| COPYRIGHT (C) 2018 - 2026 All Right Reserved
|
| FILE NAME  : \audiofmod\inc\public\interface_audio.h
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


#ifndef _INTERFACE_AUDIO_H_
#define _INTERFACE_AUDIO_H_


__BEGIN_NAMESPACE

typedef uint32 audio_id_type;
#define audio_invalid_id uint32(-1)

enum ESoundType
{
	EST_2D = 1,             // for ui sound
	EST_2DLOOP = 1 << 1,    // for music
	EST_3D = 1 << 2,        // for skill, monster, player, other behavior
	EST_3DLOOP = 1 << 3,    // for environment sound

	EST_SOUND = EST_2D | EST_3D | EST_3DLOOP,
	EST_Music = EST_2DLOOP,
};

class interface_audio
{
public:
    struct audio_listener 
    {
        virtual void onEventFinished(audio_id_type id) {}
        virtual void onEventStarted(audio_id_type id) {}
        virtual void onStolen(audio_id_type id) {}
    };

public:
    virtual ~interface_audio(void) {}

    virtual bool init(const char* config_file, interface_logmgr* log) = 0;
    virtual void destroy(void) = 0;
    virtual void update(float32 tick) = 0;
    virtual bool load(const char* config_file) = 0;

    virtual audio_id_type play(const char* name, float32* pos = 0, float32* velocity = 0, void* user_data = 0) = 0;

    virtual void stop(audio_id_type id, bool immidiate = false) = 0;
    virtual void stop(ESoundType type, bool immidiate = false) = 0;
    virtual void pause(audio_id_type id) = 0;
    virtual void pause(ESoundType type) = 0;
    virtual void resume(audio_id_type id) = 0;
    virtual void resume(ESoundType type) = 0;

    virtual void set_volume(ESoundType type, float32 vol) = 0;
    virtual void set_switch(ESoundType type, bool bswitch) = 0;

    virtual void mute(ESoundType type, bool bmute) = 0;

    // reverb
    virtual bool apply_reverb(audio_id_type id, const char* name) = 0;
    virtual bool apply_reverb(const char* name) = 0;

    // effect

    virtual const char* get_name(audio_id_type id) = 0;
    virtual bool is_valid(audio_id_type id) = 0;

    // for updating sound property
    // 3d sound attribute
    virtual bool set_attr(audio_id_type id, float32* pos, float32* velocity = 0, float32* orientation = 0) = 0;
    virtual bool get_attr(audio_id_type id, float32* pos, float32* velocity = 0, float32* orientation = 0) = 0;

    // for updating listener property
    // note : up will use default value (0, 1, 0) when is zero.
    virtual bool set_listener_attr(float32* pos, float32* forward, float32* up = 0, float32* velocity = 0) = 0;

    // listener
    virtual void add_listener(audio_listener* listener) = 0;

    // user data
    virtual void set_user_data(audio_id_type id, void* data) = 0;
    virtual void* get_user_data(audio_id_type id) = 0;
};

extern "C"
{
    AUDIOFMOD_API void load_module_audiofmod(void);
    AUDIOFMOD_API void unload_module_audiofmod(void);

    // make sure the pointer of audio is not null.
    AUDIOFMOD_API interface_audio* audio_singleton_ptr(void);
};

__END_NAMESPACE


#endif //_INTERFACE_AUDIO_H_
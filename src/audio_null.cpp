#include "stdafx.h"
#include "audio_null.h"


__BEGIN_NAMESPACE

audio_null* audio_null::singleton(void)
{
    static audio_null g_audio_null;
    return &g_audio_null;
}

audio_null::audio_null(void)
{

}

audio_null::~audio_null(void)
{

}

bool audio_null::init(const char* config_file, interface_logmgr* log)
{
    return false;
}

void audio_null::destroy(void)
{

}

void audio_null::update(float32 tick)
{

}

bool audio_null::load(const char* config_file)
{
    return false;
}

audio_id_type audio_null::play(const char* name, float32* pos /*= 0*/, float32* velocity /*= 0*/, void* user_data /*= 0*/)
{
    return audio_invalid_id;
}

void audio_null::stop(audio_id_type id, bool immidiate /*= false*/)
{

}

void audio_null::stop(ESoundType type, bool immidiate /*= false*/)
{

}

void audio_null::pause(audio_id_type id)
{

}

void audio_null::pause(ESoundType type)
{

}

void audio_null::resume(audio_id_type id)
{

}

void audio_null::resume(ESoundType type)
{

}

void audio_null::set_volume(ESoundType type, float32 vol)
{

}

void audio_null::set_switch(ESoundType type, bool bswitch)
{

}

void audio_null::mute(ESoundType type, bool bmute)
{

}

bool audio_null::apply_reverb(audio_id_type id, const char* name)
{
    return false;
}

bool audio_null::apply_reverb(const char* name)
{
    return false;
}

const char* audio_null::get_name(audio_id_type id)
{
    return 0;
}

bool audio_null::is_valid(audio_id_type id)
{
    return false;
}

bool audio_null::set_attr(audio_id_type id, float32* pos, float32* velocity /*= 0*/, float32* orientation /*= 0*/)
{
    return false;
}

bool audio_null::get_attr(audio_id_type id, float32* pos, float32* velocity /*= 0*/, float32* orientation /*= 0*/)
{
    return false;
}

bool audio_null::set_listener_attr(float32* pos, float32* forward, float32* up /*= 0*/, float32* velocity /*= 0*/)
{
    return false;
}

void audio_null::add_listener(audio_listener* listener)
{

}

void audio_null::set_user_data(audio_id_type id, void* data)
{

}

void* audio_null::get_user_data(audio_id_type id)
{
    return 0;
}

__END_NAMESPACE

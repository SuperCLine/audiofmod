#include "stdafx.h"
#include "audio_fmod.h"
#include "audio_null.h"


__BEGIN_NAMESPACE

AUDIOFMOD_API void load_module_audiofmod(void)
{
    audio_fmod::instance();
}

AUDIOFMOD_API void unload_module_audiofmod(void)
{
    if (audio_fmod::singleton())
    {
        delete audio_fmod::singleton();
    }
}

AUDIOFMOD_API interface_audio* audio_singleton_ptr(void)
{
    return audio_fmod::singleton() ? app_cast_static(interface_audio*, audio_fmod::singleton()) : 
        app_cast_static(interface_audio*, audio_null::singleton());
}

audio_fmod* audio_fmod::m_singleton = 0;
audio_fmod* audio_fmod::singleton(void)
{
    return m_singleton;
}

audio_fmod* audio_fmod::instance(void)
{
    if (!m_singleton)
    {
        m_singleton = new audio_fmod();
    }

    return m_singleton;
}


//////////////////////////////////////////////////////////////////////////
//audiofmod_impl
struct audiofmod_impl
{
public:
    class audio_project
    {
    public:
        audio_project(const char* name) : m_project(0), m_name(name) {}
        ~audio_project(void)
        {
            m_project->release();
        }

        bool load_project(audiofmod_impl* impl)
        {
            if (m_project) 
                return false;// has loaded

            ustring project(m_name);
            project += m_project_ext;

            return impl->m_mgr->check(impl->m_event_system->load(project.c_str(), 0, &m_project));
        }

        bool load_project(audiofmod_impl* impl, const char* data, int32 size)
        {
            if (m_project)
                return false;// has loaded

            FMOD_EVENT_LOADINFO info;
            app_memset(&info, 0, sizeof(FMOD_EVENT_LOADINFO));
            info.loadfrommemory_length = size;

            return impl->m_mgr->check(impl->m_event_system->load(data, &info, &m_project));
        }

        inline const ustring& get_name(void) const { return m_name; }
        inline FMOD::EventProject* get_project(void) const { return m_project; }

    private:
        FMOD::EventProject*	    m_project;
        ustring                 m_name;
        static const ustring    m_project_ext;
    };
    // for static group load policy
    class audio_group
    {
    public:
        audio_group(const char* name) : m_group(0), m_name(name) {}
        ~audio_group(void)
        {
            m_group->freeEventData();
        }

        bool load_group(audiofmod_impl* impl)
        {
            if (m_group) 
                return false;

            if (impl->m_mgr->check(impl->m_event_system->getGroup(m_name.c_str(), false, &m_group)))
            {
                return impl->m_mgr->check(m_group->loadEventData(FMOD_EVENT_RESOURCE_SAMPLES, FMOD_EVENT_DEFAULT));
            }

            return false;
        }

        inline const ustring& getName(void) const { return m_name; }
        inline FMOD::EventGroup*   getGroup(void) const { return m_group; }

    private:
        FMOD::EventGroup*   m_group;
        ustring             m_name;
    };

    class audio_event
    {
    public:
        audio_event(void) {}
        ~audio_event(void) {}

        bool init(audiofmod_impl* impl, const char* name, ESoundType type, FMOD_EVENT_MODE mode, void* data)
        {
            m_name = name;
            m_type = type;
            m_user_data = data;

            return impl->m_mgr->check(impl->m_event_system->getEvent(name, mode, &m_event));
        }
        void reset(audiofmod_impl* impl)
        {
            impl->m_mgr->check(m_event->setUserData(0));
            m_event->release();
            m_user_data = 0;
            m_type = EST_2DLOOP;
            m_name = "";
        }

        inline const ustring& get_name(void) const { return m_name; }
        inline ESoundType get_type(void) const { return m_type; }
        inline FMOD::Event* get_event(void) const { return m_event; }
        inline void* get_user_data(void) { return m_user_data; }

        static ESoundType build_type(audiofmod_impl* impl, const char* name)
        {
            ESoundType   type = EST_2DLOOP;
            FMOD::Event* event = 0;

            if (impl->m_mgr->check(impl->m_event_system->getEvent(name, FMOD_EVENT_INFOONLY, &event)))
            {
                type = impl->get_type(event);
                event->release();
            }

            return type;
        }

    private:
        FMOD::Event*		m_event;
        void*				m_user_data;
        ESoundType			m_type;
        ustring             m_name;
    };

public:
    audiofmod_impl(audio_fmod* mgr, FMOD::EventSystem* system)
        : m_mgr(mgr)
        , m_event_system(system)
        , m_2d_vol(1.f)
        , m_2dloop_vol(1.f)
        , m_3d_vol(1.f)
        , m_3dloop_vol(1.f)
        , m_2d_switch(true)
        , m_2dloop_switch(true)
        , m_3d_switch(true)
        , m_3dloop_switch(true)
        , m_event_pool(new core_pool<audio_event>())
    {

    }

    ~audiofmod_impl(void) { destroy(); }

    bool init()
    {
        // TO CLine: implement using new fmod later.

        return true;
    }

    void destroy(void)
    {
        // clear running stuff
        clear();

        // destroy pool
        app_safe_delete(m_event_pool);

        // destroy static group and unload all
        for (auto itr = m_group_map.begin(); itr != m_group_map.end(); ++itr)
        {
            app_safe_delete(itr->second);
        }
        m_group_map.clear();

        // destroy project and unload all
        for (auto itr = m_project_map.begin(); itr != m_project_map.end(); ++itr)
        {
            app_safe_delete(itr->second);
        }
        m_project_map.clear();

        // destroy event system
        m_event_system->unload();
        m_event_system->release();

        m_listener_list.clear();
    }

    void clear(void)
    {
        m_mgr->stop(EST_2D | EST_2DLOOP | EST_3D | EST_3DLOOP, true);
    }

    void erase(audio_id_type id)
    {
        if (!is_valid(id)) 
            return;

        audio_event*  audio_evt = 0;
        FMOD::Event* event = (FMOD::Event*)id;
        m_mgr->check(event->getUserData((void**)&audio_evt));

        ESoundType type = audio_evt->get_type();

        switch (type)
        {
        case EST_2D:
            erase_event_map(m_2devent_map, id);
            break;
        case EST_2DLOOP:
            erase_event_map(m_2dloopevent_map, id);
            break;
        case EST_3D:
            erase_event_map(m_3devent_map, id);
            break;
        case EST_3DLOOP:
            erase_event_map(m_3dloopevent_map, id);
            break;
        }
    }

    void insert(audio_id_type id)
    {
        audio_event*  audio_evt = 0;
        FMOD::Event* event = (FMOD::Event*)id;
        m_mgr->check(event->getUserData((void**)&audio_evt));

        ESoundType type = audio_evt->get_type();

        switch (type)
        {
        case EST_2D:
            m_2devent_map[id] = audio_evt;
            break;
        case EST_2DLOOP:
            m_2dloopevent_map[id] = audio_evt;
            break;
        case EST_3D:
            m_3devent_map[id] = audio_evt;
            break;
        case EST_3DLOOP:
            m_3dloopevent_map[id] = audio_evt;
            break;
        }
    }

    bool is_valid(audio_id_type id)
    {
        if (m_2dloopevent_map.find(id) != m_2dloopevent_map.end()) return true;
        if (m_3dloopevent_map.find(id) != m_3dloopevent_map.end()) return true;
        if (m_2devent_map.find(id) != m_2devent_map.end()) return true;
        if (m_3devent_map.find(id) != m_3devent_map.end()) return true;

        return false;
    }

    ESoundType get_type(FMOD::Event* event)
    {
        ESoundType type = EST_2DLOOP;
        FMOD_MODE is_3d = 0;
        int32 oneshort = 0;

        bool check_is_3d = m_mgr->check(event->getPropertyByIndex(FMOD_EVENTPROPERTY_MODE, app_cast_static(void*, &is_3d), false));
        bool check_oneshort = m_mgr->check(event->getPropertyByIndex(FMOD_EVENTPROPERTY_ONESHOT, app_cast_static(void*, &oneshort), false));

        if (check_is_3d && check_oneshort)
        {
            if (is_3d == FMOD_3D)
            {
                type = (oneshort ? EST_3D : EST_3DLOOP);
            }
            else
            {
                type = (oneshort ? EST_2D : EST_2DLOOP);
            }
        }

        return type;
    }

    audio_fmod*			m_mgr;
    FMOD::EventSystem*	m_event_system;

    umap<ustring, audio_project*>	        m_project_map;
    // static group for loading policy
    umap<ustring, audio_group*>	            m_group_map;

    // running event according to ESoundType, so as to find easily
    umap<audio_id_type, audio_event*>		m_2devent_map;
    umap<audio_id_type, audio_event*>		m_2dloopevent_map;
    umap<audio_id_type, audio_event*>		m_3devent_map;
    umap<audio_id_type, audio_event*>		m_3dloopevent_map;
    core_pool<audio_event>*                 m_event_pool;

    ulist<interface_audio::audio_listener*> m_listener_list;

    // volume and switch
    float32	m_2d_vol;
    float32	m_2dloop_vol;
    float32	m_3d_vol;
    float32	m_3dloop_vol;
    bool	m_2d_switch;
    bool	m_2dloop_switch;
    bool	m_3d_switch;
    bool	m_3dloop_switch;

private:
    void erase_event_map(umap<audio_id_type, audio_event*>& map, audio_id_type id)
    {
        umap<audio_id_type, audio_event*>::iterator itr = map.find(id);

        if (itr != map.end())
        {
            audio_event* evt = itr->second;
            evt->reset(this);
            m_event_pool->cycle(evt);

            map.erase(itr);
        }
    }
};
const ustring audiofmod_impl::audio_project::m_project_ext = ".fev";



//////////////////////////////////////////////////////////////////////////
// call back
FMOD_RESULT F_CALLBACK event_call(FMOD_EVENT* event, FMOD_EVENT_CALLBACKTYPE  type, void* param1, void* param2, void* userdata)
{
    audio_id_type id = (audio_id_type)event;
    audiofmod_impl* impl = app_cast_static(audiofmod_impl*, userdata);

    switch (type)
    {
    case FMOD_EVENT_CALLBACKTYPE_SOUNDDEF_START:
        break;
    case FMOD_EVENT_CALLBACKTYPE_SOUNDDEF_END:
        break;
    case FMOD_EVENT_CALLBACKTYPE_EVENTFINISHED:
    {
        for (ulist<interface_audio::audio_listener*>::iterator itr = impl->m_listener_list.begin(); itr != impl->m_listener_list.end(); ++itr)
        {
            (*itr)->onEventFinished(id);
        }
        impl->erase(id);
    }
    break;
    case FMOD_EVENT_CALLBACKTYPE_STOLEN:
    {
        for (ulist<interface_audio::audio_listener*>::iterator itr = impl->m_listener_list.begin(); itr != impl->m_listener_list.end(); ++itr)
        {
            (*itr)->onStolen(id);
        }
        impl->erase(id);
    }
    break;
    case FMOD_EVENT_CALLBACKTYPE_EVENTSTARTED:
    {
        for (ulist<interface_audio::audio_listener*>::iterator itr = impl->m_listener_list.begin(); itr != impl->m_listener_list.end(); ++itr)
        {
            (*itr)->onEventStarted(id);
        }
        impl->insert(id);
    }
    break;
    }

    return FMOD_OK;
}

typedef FMOD_RESULT(F_CALLBACK *event_set_bool)(FMOD_EVENT *event, FMOD_BOOL val);
typedef FMOD_RESULT(F_CALLBACK *event_set_float)(FMOD_EVENT *event, float val);

template < typename T, typename V >
class event_set_functor
{
public:
    event_set_functor(T fun) : m_fun(fun)
    {

    }

    void operator()(audiofmod_impl* impl, ESoundType type, V val)
    {
        umap<audio_id_type, audiofmod_impl::audio_event*>* exec_map = 0;
        if (type & EST_2D)
        {
            exec_map = &impl->m_2devent_map;
        }

        if (type & EST_3D)
        {
            exec_map = &impl->m_3devent_map;
        }

        if (type & EST_2DLOOP)
        {
            exec_map = &impl->m_2dloopevent_map;
        }

        if (type & EST_3DLOOP)
        {
            exec_map = &impl->m_3dloopevent_map;
        }

        if (exec_map)
        {
            // TO CLine: must pay more attention to iterator because of the operation of erase will take place at stop call back function.
            for (auto itr = exec_map->begin(); itr != exec_map->end(); ++itr)
            {
                impl->m_mgr->check((*m_fun)((FMOD_EVENT*)itr->second->get_event(), val));
            }
        }
    }

private:
    T m_fun;
};



//////////////////////////////////////////////////////////////////////////
//audio_fmod
audio_fmod::audio_fmod(void)
    : m_impl(0)
    , m_max_channels(256)
    , m_load_atinit(true)
    , m_enable_log(true)
    , m_enable_assert(true)
{

}

audio_fmod::~audio_fmod(void)
{
    destroy();
}

bool audio_fmod::init(const char* config_file, interface_logmgr* log)
{
    set_audio_logger(log);

    // TO CLine: read from json or xml at later.
    m_max_channels = 0;
    m_load_atinit = 0;
    m_enable_assert = true;
    m_enable_assert = true;
    m_media_path = "";

    //////////////////////////////////////////////////////////////////////////
    // init event system
    FMOD::EventSystem* event_system = 0;
    if (!check(FMOD::EventSystem_Create(&event_system)))
    {
        audio_log(ELT_ERROR, "audiofmod", "failed to create fmod event system.");
        return false;
    }

    core_scoped_ptr<FMOD::EventSystem> scoped_ptr(event_system, EFP_RELEASE);
    FMOD::System* system = 0;
    if (!check(event_system->getSystemObject(&system)))
    {
        audio_log(ELT_ERROR, "audiofmod", "failed to get fmod system.");
        return false;
    }

    uint32 version;
    if (!check(system->getVersion(&version)))
    {
        audio_log(ELT_ERROR, "audiofmod", "failed to check fmod version.");
        return false;
    }
    if (version < FMOD_VERSION)
    {
        audio_logf(ELT_ERROR, "audiofmod", "Error! You are using an old version of FMOD %08x.  This program requires %08x\n", version, FMOD_VERSION);
        return false;
    }

    int numdrivers;
    if (!check(system->getNumDrivers(&numdrivers)))
    {
        audio_log(ELT_ERROR, "audiofmod", "failed to get fmod drivers.");
        return false;
    }
    if (numdrivers == 0)
    {
        if (!check(system->setOutput(FMOD_OUTPUTTYPE_NOSOUND)))
        {
            audio_log(ELT_ERROR, "audiofmod", "failed to set fmod output.");
            return false;
        }
    }
    else
    {
        FMOD_CAPS caps;
        FMOD_SPEAKERMODE speakermode;
        if (!check(system->getDriverCaps(0, &caps, 0, &speakermode)))
        {
            audio_log(ELT_ERROR, "audiofmod", "failed to get fmod drivers caps.");
            return false;
        }
        if (!check(system->setSpeakerMode(speakermode)))
        {
            audio_log(ELT_ERROR, "audiofmod", "failed to set fmod speaker mode.");
            return false;
        }
        if (caps & FMOD_CAPS_HARDWARE_EMULATED)
        {
            if (!check(system->setDSPBufferSize(1024, 10)))
            {
                audio_log(ELT_ERROR, "audiofmod", "failed to set fmod dsp buffer.");
                return false;
            }
        }

        char name[256] = { 0 };
        if (!check(system->getDriverInfo(0, name, 256, 0)))
        {
            audio_log(ELT_ERROR, "audiofmod", "failed to get fmod driver info.");
            return false;
        }
        if (strstr(name, "SigmaTel"))
        {
            if (!check(system->setSoftwareFormat(48000, FMOD_SOUND_FORMAT_PCMFLOAT, 0, 0, FMOD_DSP_RESAMPLER_LINEAR)))
            {
                audio_log(ELT_ERROR, "audiofmod", "failed to set fmod audio format.");
                return false;
            }
        }
    }

    FMOD_RESULT result = event_system->init(m_max_channels, FMOD_INIT_3D_RIGHTHANDED | FMOD_INIT_OCCLUSION_LOWPASS | FMOD_INIT_HRTF_LOWPASS, 0, FMOD_EVENT_INIT_NORMAL);
    if (result == FMOD_ERR_OUTPUT_CREATEBUFFER)
    {
        if (!check(system->setSpeakerMode(FMOD_SPEAKERMODE_STEREO)))
        {
            audio_log(ELT_ERROR, "audiofmod", "failed to set fmod speaker stereo mode.");
            return false;
        }
        result = event_system->init(m_max_channels, FMOD_INIT_3D_RIGHTHANDED | FMOD_INIT_OCCLUSION_LOWPASS | FMOD_INIT_HRTF_LOWPASS, 0, FMOD_EVENT_INIT_NORMAL);
    }
    if (!check(result))
    {
        return false;
    }

    // set media
    if (!check(event_system->setMediaPath(m_media_path.c_str())))
    {
        audio_log(ELT_ERROR, "audiofmod", "failed to set media path.");
        return false;
    }

    // init impl
    m_impl = new audiofmod_impl(this, scoped_ptr.detach());
    bool resourceInit = m_impl->init();

    return resourceInit;
}

void audio_fmod::destroy(void)
{
    set_audio_logger(0);
    app_safe_delete(m_impl);
}
void audio_fmod::update(float32 tick)
{
    m_impl->m_event_system->update();
}

bool audio_fmod::load(const char* config_file)
{
    return true;
}

audio_id_type audio_fmod::play(const char* name, float32* pos /*= 0*/, float32* velocity /*= 0*/, void* user_data /*= 0*/)
{
    ESoundType type = audiofmod_impl::audio_event::build_type(m_impl, name);

    bool  is_3d = false;
    bool  is_mute = false;
    float32 vol = 1.f;
    audio_id_type id = audio_invalid_id;
    FMOD_EVENT_MODE	 mode = FMOD_EVENT_DEFAULT;

    switch (type)
    {
    case EST_2D:
        is_3d = false;
        is_mute = !m_impl->m_2d_switch;
        vol = m_impl->m_2d_vol;
        mode = FMOD_EVENT_DEFAULT | FMOD_EVENT_ERROR_ON_DISKACCESS;
        break;
    case EST_3D:
        is_3d = true;
        is_mute = !m_impl->m_3d_switch;
        vol = m_impl->m_3d_vol;
        mode = FMOD_EVENT_DEFAULT | FMOD_EVENT_ERROR_ON_DISKACCESS;
        break;
    case EST_3DLOOP:
        is_3d = true;
        is_mute = !m_impl->m_3dloop_switch;
        vol = m_impl->m_3dloop_vol;
        mode = FMOD_EVENT_DEFAULT | FMOD_EVENT_ERROR_ON_DISKACCESS;
        break;
    case EST_2DLOOP:
        is_3d = false;
        is_mute = !m_impl->m_2dloop_switch;
        vol = m_impl->m_2dloop_vol;
        mode = FMOD_EVENT_NONBLOCKING;
        break;
    }

    if (m_enable_assert && !is_3d && (pos || velocity))
    {
        app_assert(!"this event is not 3d, but set up pos or velocity parameter!");
    }

    audiofmod_impl::audio_event* audio_evt = m_impl->m_event_pool->get();

    if (audio_evt->init(m_impl, name, type, mode, user_data))
    {
        FMOD::Event* event = audio_evt->get_event();
        id = (audio_id_type)event;

        if (is_3d)
        {
            FMOD_VECTOR vec_position;
            FMOD_VECTOR vec_velocity;

            app_memset(&vec_position, 0, sizeof(FMOD_VECTOR));
            app_memset(&velocity, 0, sizeof(FMOD_VECTOR));

            if (pos)
            {
                vec_position.x = pos[0];
                vec_position.y = pos[1];
                vec_position.z = pos[2];
            }
            if (velocity)
            {
                vec_velocity.x = velocity[0];
                vec_velocity.y = velocity[1];
                vec_velocity.z = velocity[2];
            }
            event->set3DAttributes(&vec_position, &vec_velocity);
        }

        event->setCallback(event_call, m_impl);
        event->setMute(is_mute);
        event->setVolume(vol);
        event->setUserData(app_cast_static(void*, event));
        event->start();
    }
    else
    {
        m_impl->m_event_pool->cycle(audio_evt);
    }

    return id;
}

void audio_fmod::stop(audio_id_type id, bool immidiate /*= false*/)
{
    if (!is_valid(id)) return;

    FMOD::Event* event = (FMOD::Event*)id;

    check(event->stop(immidiate));
}

void audio_fmod::stop(ESoundType type, bool immidiate /*= false*/)
{
    umap<audio_id_type, audiofmod_impl::audio_event*>* temp_map = 0;
    if (type & EST_2D)
    {
        temp_map = &m_impl->m_2devent_map;
    }

    if (type & EST_3D)
    {
        temp_map = &m_impl->m_3devent_map;
    }

    if (type & EST_2DLOOP)
    {
        temp_map = &m_impl->m_2dloopevent_map;
    }

    if (type & EST_3DLOOP)
    {
        temp_map = &m_impl->m_3dloopevent_map;
    }

    if (temp_map)
    {
        for (auto itr = temp_map->begin(); itr != temp_map->end(); ++itr)
        {
            m_impl->m_mgr->check(itr->second->get_event()->stop(immidiate));
        }
    }
}

void audio_fmod::pause(audio_id_type id)
{
    if (!is_valid(id)) return;

    FMOD::Event* event = (FMOD::Event*)id;

    check(event->setPaused(true));
}

void audio_fmod::pause(ESoundType type)
{
    event_set_functor<event_set_bool, bool> fun_obj((event_set_bool)FMOD_Event_SetPaused);
    fun_obj(m_impl, type, true);
}

void audio_fmod::resume(audio_id_type id)
{
    if (!is_valid(id)) return;

    FMOD::Event* event = (FMOD::Event*)id;

    check(event->setPaused(false));
}

void audio_fmod::resume(ESoundType type)
{
    event_set_functor<event_set_bool, bool> fun_obj((event_set_bool)FMOD_Event_SetPaused);
    fun_obj(m_impl, type, false);
}

void audio_fmod::set_volume(ESoundType type, float32 vol)
{
    if (type & EST_2D)
    {
        m_impl->m_2d_vol = vol;
    }
    if (type & EST_3D)
    {
        m_impl->m_3d_vol = vol;
    }
    if (type & EST_2DLOOP)
    {
        m_impl->m_2dloop_vol = vol;
    }
    if (type & EST_3DLOOP)
    {
        m_impl->m_3dloop_vol = vol;
    }

    event_set_functor<event_set_float, float> fun_obj((event_set_float)FMOD_Event_SetVolume);
    fun_obj(m_impl, type, vol);
}

void audio_fmod::set_switch(ESoundType type, bool bswitch)
{
    if (type & EST_2D)
    {
        m_impl->m_2d_switch = bswitch;
    }
    if (type & EST_3D)
    {
        m_impl->m_3d_switch = bswitch;
    }
    if (type & EST_2DLOOP)
    {
        m_impl->m_2dloop_switch = bswitch;
    }
    if (type & EST_3DLOOP)
    {
        m_impl->m_3dloop_switch = bswitch;
    }

    event_set_functor<event_set_bool, bool> fun_obj((event_set_bool)FMOD_Event_SetMute);
    fun_obj(m_impl, type, !bswitch);
}

void audio_fmod::mute(ESoundType type, bool bmute)
{
    set_switch(type, !bmute);
}

bool audio_fmod::apply_reverb(audio_id_type id, const char* name)
{
    if (!name || !is_valid(id))
        return false;

    FMOD::Event* event = (FMOD::Event*)id;
    FMOD_REVERB_PROPERTIES reverbProperties;

    if (check(m_impl->m_event_system->getReverbPreset(name, &reverbProperties)))
    {
        //return check(event->setReverbProperties( &reverbProperties ) );
    }

    return false;
}

bool audio_fmod::apply_reverb(const char* name)
{
    if (!name)
        return false;

    FMOD_REVERB_PROPERTIES reverbProperties;
    if (check(m_impl->m_event_system->getReverbPreset(name, &reverbProperties)))
    {
        return check(m_impl->m_event_system->setReverbProperties(&reverbProperties));
    }

    return false;
}

const char* audio_fmod::get_name(audio_id_type id)
{
    if (!is_valid(id)) return "";

    FMOD::Event* event = (FMOD::Event*)id;
    audiofmod_impl::audio_event* audio_evt = 0;

    if (check(event->getUserData((void**)&audio_evt)))
    {
        return audio_evt->get_name().c_str();
    }

    return "";
}

bool audio_fmod::is_valid(audio_id_type id)
{
    if (audio_invalid_id == id)
        return false;

    return m_impl->is_valid(id);
}

bool audio_fmod::set_attr(audio_id_type id, float32* pos, float32* velocity /*= 0*/, float32* orientation /*= 0*/)
{
    if (!is_valid(id)) return false;

    FMOD::Event* event = (FMOD::Event*)id;

    return check(event->set3DAttributes((const FMOD_VECTOR*)pos, (const FMOD_VECTOR*)velocity, (const FMOD_VECTOR*)orientation));
}

bool audio_fmod::get_attr(audio_id_type id, float32* pos, float32* velocity /*= 0*/, float32* orientation /*= 0*/)
{
    if (!is_valid(id)) return false;

    FMOD::Event* event = (FMOD::Event*)id;

    return check(event->get3DAttributes((FMOD_VECTOR*)pos, (FMOD_VECTOR*)velocity, (FMOD_VECTOR*)orientation));
}

bool audio_fmod::set_listener_attr(float32* pos, float32* forward, float32* up /*= 0*/, float32* velocity /*= 0*/)
{
    static const FMOD_VECTOR defaultUP = { 0, 1, 0 };
    const FMOD_VECTOR* up_vector = (up ? (const FMOD_VECTOR*)up : &defaultUP);

    return check(m_impl->m_event_system->set3DListenerAttributes(0, (const FMOD_VECTOR*)pos, (const FMOD_VECTOR*)velocity, (const FMOD_VECTOR*)forward, (const FMOD_VECTOR*)up_vector));
}

void audio_fmod::add_listener(audio_listener* listener)
{
    if (listener)
    {
        m_impl->m_listener_list.push_back(listener);
    }
}

void audio_fmod::set_user_data(audio_id_type id, void* data)
{
    if (!is_valid(id)) return;

    FMOD::Event* event = (FMOD::Event*)id;

    check(event->setUserData(data));
}

void* audio_fmod::get_user_data(audio_id_type id)
{
    if (!is_valid(id)) return 0;

    FMOD::Event* event = (FMOD::Event*)id;
    void* data = 0;

    check(event->getUserData(&data));

    return data;
}

bool audio_fmod::check(FMOD_RESULT result)
{
    if (result != FMOD_OK)
    {
        if (m_enable_log)
            audio_logf(ELT_ERROR, "audiofmod", "FMOD error! (%d) %s", result, FMOD_ErrorString(result));

        if (m_enable_assert)
            app_assert(!"FMOD error!");

        return false;
    }

    return true;
}

__END_NAMESPACE



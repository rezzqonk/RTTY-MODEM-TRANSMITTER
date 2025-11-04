#include <alsa/asoundlib.h>
#define FS 48000
char* devcID;// Sound Card ID                                                      
snd_pcm_t *playback_handle;
snd_pcm_hw_params_t *hw_params;
snd_pcm_format_t format = SND_PCM_FORMAT_S16_LE;

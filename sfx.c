// SFX.c : uses module player for SFX
#include "lib/mod/mod32.h"

#define SFX_CHANNEL (MOD_CHANNELS-1) // use last channel
#define SFX_VOLUME 64
#define SFX_NOTE	214

extern const uint16_t amigaPeriods[];
extern struct Player mod32_player;
extern struct Mod *mod;
extern struct Mixer mixer;

void play_sfx(int sample_id)
{
    const struct Sample *sample = &mod->samples[sample_id];

    mixer.sampleBegin[SFX_CHANNEL] = 0;
    mixer.sampleEnd[SFX_CHANNEL] = sample->length;
    mixer.sampleLoopLength[SFX_CHANNEL] =0;
    mixer.channelSampleNumber[SFX_CHANNEL] = sample_id;
    mixer.channelSampleOffset[SFX_CHANNEL] = 0;
    mixer.channelFrequency[SFX_CHANNEL] = mod32_player.amiga / amigaPeriods[SFX_NOTE];
    mixer.channelVolume[SFX_CHANNEL] = SFX_VOLUME;
}

void mod_jumpto(uint8_t order)
{
    mod32_player.orderIndex = order;
    mod32_player.row = 0;
}


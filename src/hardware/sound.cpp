/****************************************************************************
 *   Tu May 22 21:23:51 2020
 *   Copyright  2020  Dirk Brosswick
 *   Email: dirk.brosswick@googlemail.com
 ****************************************************************************/
 
/*
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */
#include "config.h"

#include "powermgm.h"
#include "sound.h"
#include "blectl.h"
#include "timesync.h"
#include "callback.h"
#include "utils/alloc.h"
#include "hardware/config/soundconfig.h"

#ifdef NATIVE_64BIT
    #include "utils/logging.h"
#else
    #include <SPIFFS.h>
    /*
    * based on https://github.com/earlephilhower/ESP8266Audio
    */
    #if defined( M5PAPER )
    #elif defined( LILYGO_WATCH_2020_V1 ) || defined( LILYGO_WATCH_2020_V3 )
        #include "TTGO.h"

        #include "AudioFileSourceSPIFFS.h"
        #include "AudioFileSourcePROGMEM.h"
        #include "AudioFileSourceFunction.h"
        #include "AudioFileSourceID3.h"
        #include "AudioGeneratorMP3.h"
        #include "AudioGeneratorWAV.h"
        #include "AudioGeneratorMIDI.h"

        #include "AudioOutputI2S.h"
        #include "BluetoothA2DPSink.h"
        #include "BluetoothA2DPSource.h"

        #define c3_frequency  130.81

        AudioFileSourceSPIFFS *spliffs_file;
        AudioOutputI2S *out;
        AudioFileSourceID3 *id3;

        BluetoothA2DPSink a2dp_sink;
        BluetoothA2DPSource a2dp_source;

	// midi soundfont
	AudioFileSourceSPIFFS *midi_sf2;

        AudioFileSourceFunction* funsource;
        AudioFileSourceFunction* mf_funsource;
        AudioFileSourceFunction* dt_funsource;

        AudioGeneratorMP3 *mp3;
        AudioGeneratorWAV *wav;
	AudioGeneratorMIDI *midi;

        AudioFileSourcePROGMEM *progmem_file;
    #elif defined( LILYGO_WATCH_2020_V2 )
        #include "TTGO.h"

        #include "AudioFileSourceSPIFFS.h"
        #include "AudioFileSourcePROGMEM.h"
        #include "AudioFileSourceFunction.h"
        #include "AudioFileSourceID3.h"
        #include "AudioGeneratorMP3.h"
        #include "AudioGeneratorWAV.h"
        #include "AudioGeneratorMIDI.h"

        #include "BluetoothA2DPSource.h"

        #define c3_frequency  130.81

        AudioFileSourceSPIFFS *spliffs_file;
        AudioFileSourceID3 *id3;

        BluetoothA2DPSource a2dp_source;

	// midi soundfont
	AudioFileSourceSPIFFS *midi_sf2;

        AudioFileSourceFunction* funsource;
        AudioFileSourceFunction* mf_funsource;
        AudioFileSourceFunction* dt_funsource;

        AudioGeneratorMP3 *mp3;
        AudioGeneratorWAV *wav;
	AudioGeneratorMIDI *midi;

        AudioFileSourcePROGMEM *progmem_file;
    #elif defined( LILYGO_WATCH_2021 )    
    #else
        #warning "no hardware driver for sound"
    #endif
#endif

bool sound_init = false;
bool is_speaking = false;
bool is_bt = false; 

char *mf_insound; 
char *dt_insound; 

sound_config_t sound_config;

callback_t *sound_callback = NULL;

bool sound_powermgm_event_cb( EventBits_t event, void *arg );
bool sound_powermgm_loop_cb( EventBits_t event, void *arg );
bool sound_send_event_cb( EventBits_t event, void*arg );
bool sound_is_silenced( void );

void sound_setup( void ) {
    if ( sound_init )
        return;

    /*
     * read config from SPIFFS
     */
    sound_config.load();
    /*
     * config sound driver and interface
     */
    #ifdef NATIVE_64BIT

    #else
        #if defined( LILYGO_WATCH_2020_V1 ) || defined( LILYGO_WATCH_2020_V3 )
            /*
            * set sound chip voltage on V1
            */
            #if defined( LILYGO_WATCH_2020_V1 )
                    TTGOClass *ttgo = TTGOClass::getWatch();
                    ttgo->power->setLDO3Mode( AXP202_LDO3_MODE_DCIN );
                    ttgo->power->setLDO3Voltage( 2900 );
            #endif
            /**
             * set sound driver
             */
            out  = new AudioOutputI2S();
            out->SetPinout( TWATCH_DAC_IIS_BCK, TWATCH_DAC_IIS_WS, TWATCH_DAC_IIS_DOUT );

            mp3  = new AudioGeneratorMP3();
            wav  = new AudioGeneratorWAV();
            midi = new AudioGeneratorMIDI();

            /*
            * register all powermgm callback functions
            */
            powermgm_register_cb( POWERMGM_SILENCE_WAKEUP | POWERMGM_STANDBY | POWERMGM_WAKEUP, sound_powermgm_event_cb, "powermgm sound" );
            powermgm_register_loop_cb( POWERMGM_STANDBY | POWERMGM_SILENCE_WAKEUP | POWERMGM_WAKEUP, sound_powermgm_loop_cb, "powermgm sound loop" );

            sound_set_enabled( sound_config.enable );
            sound_init = true;
            sound_set_volume_config( sound_config.volume );

            sound_send_event_cb( SOUNDCTL_ENABLED, (void *)&sound_config.enable );
            sound_send_event_cb( SOUNDCTL_VOLUME, (void *)&sound_config.volume );

        #else
            sound_set_enabled( false );
            sound_init = false;
        #endif
    #endif
}

bool sound_get_available( void ) {
    bool retval = false;

    #ifdef NATIVE_64BIT
    #else
        #if defined( LILYGO_WATCH_2020_V1 ) || defined( LILYGO_WATCH_2020_V3 )
            retval = true;
        #endif
    #endif

   return( retval );
}

bool sound_powermgm_event_cb( EventBits_t event, void *arg ) {
    /**
     * check if sound available
     */
    if( !sound_get_available() ) {
        return( true );
    }

    switch( event ) {
        case POWERMGM_STANDBY:          if(is_bt) 
                                        {
                                            log_i("Stopping standby as A2DP active");
                                            return false; 
                                        }
                                        sound_set_enabled( false );
                                        break;
        case POWERMGM_WAKEUP:           sound_set_enabled( sound_config.enable );
                                        break;
        case POWERMGM_SILENCE_WAKEUP:   sound_set_enabled( sound_config.enable );
                                        break;
    }
    return( true );
}

bool sound_powermgm_loop_cb( EventBits_t event, void *arg ) {
    /**
     * check if sound available
     */
    if( !sound_get_available() ) {
        return( true );
    }
#ifdef NATIVE_64BIT

#else
    #if defined( LILYGO_WATCH_2020_V1 ) || defined( LILYGO_WATCH_2020_V3 )
        if ( sound_config.enable && sound_init ) {
            // we call sound_set_enabled(false) to ensure the PMU stops all power
            if ( mp3->isRunning() && !mp3->loop() ) {
                mp3->stop();
            }
            if ( wav->isRunning() && !wav->loop() ) {
                wav->stop(); 
            }
            if ( midi->isRunning() && !midi->loop() ) {
                midi->stop(); 
            }
        }
    #endif
#endif
    return( true );
}

bool sound_register_cb( EventBits_t event, CALLBACK_FUNC callback_func, const char *id ) {
    /**
     * check if sound available
     */
    if( !sound_get_available() ) {
        return( true );
    }

    /*
     * check if an callback table exist, if not allocate a callback table
     */
    if ( sound_callback == NULL ) {
        sound_callback = callback_init( "sound" );
        if ( sound_callback == NULL ) {
            log_e("sound callback alloc failed");
            while(true);
        }
    }
    /*
     * register an callback entry and return them
     */
    return( callback_register( sound_callback, event, callback_func, id ) );
}

bool sound_send_event_cb( EventBits_t event, void *arg ) {
    /**
     * check if sound available
     */
    if( !sound_get_available() ) {
        return( true );
    }
    /*
     * call all callbacks with her event mask
     */
    return( callback_send( sound_callback, event, arg ) );
}

/**
 * @brief enable or disable the power output for AXP202_LDO3 or AXP202_LDO4
 * depending on the current value of: sound_config.enable
 */
void sound_set_enabled( bool enabled ) {
    /**
     * check if sound available
     */
    if( !sound_get_available() ) {
        return;
    }
#ifdef NATIVE_64BIT

#else
    #if defined( LILYGO_WATCH_2020_V1 ) || defined( LILYGO_WATCH_2020_V3 )
        if ( enabled ) {
            /**
             * ttgo->enableAudio() is not working
             */
            #if     defined( LILYGO_WATCH_2020_V1 )
                    TTGOClass *ttgo = TTGOClass::getWatch();
                    ttgo->power->setPowerOutPut( AXP202_LDO3, AXP202_ON );
            #elif   defined( LILYGO_WATCH_2020_V3 )
                    TTGOClass *ttgo = TTGOClass::getWatch();
                    ttgo->power->setPowerOutPut( AXP202_LDO4, AXP202_ON );
            #endif
            delay( 50 );
        }
        else {
            if ( sound_init ) {
                if ( mp3->isRunning() ) mp3->stop();
                if ( wav->isRunning() ) wav->stop();
                if ( midi->isRunning() ) midi->stop();
            }
            /**
             * ttgo->disableAudio() is not working
             */
            #if     defined( LILYGO_WATCH_2020_V1 )
                    TTGOClass *ttgo = TTGOClass::getWatch();
                    ttgo->power->setPowerOutPut( AXP202_LDO3, AXP202_OFF );
            #elif   defined( LILYGO_WATCH_2020_V3 )
                    TTGOClass *ttgo = TTGOClass::getWatch();
                    ttgo->power->setPowerOutPut( AXP202_LDO4, AXP202_OFF );
            #endif
        }
    #endif
#endif
}

void sound_a2dp_sink(void) 
{
#if defined( LILYGO_WATCH_2020_V1 ) || defined( LILYGO_WATCH_2020_V3 )
    if( sound_config.enable && sound_init && !sound_is_silenced() && blectl_get_autoon() ) 
    {
        is_bt = true;
        i2s_pin_config_t my_pin_config = {
          .bck_io_num = TWATCH_DAC_IIS_BCK,
          .ws_io_num = TWATCH_DAC_IIS_WS,
          .data_out_num = TWATCH_DAC_IIS_DOUT,
          .data_in_num = I2S_PIN_NO_CHANGE
        };
        a2dp_sink.set_pin_config(my_pin_config);
        a2dp_sink.start("eMusic");
    }
    else
    {
        log_i("Cannot enable A2DP sink per settings");
    }
#endif
}

float tone1, tone2; 

float sound_generate_sine_tone(const float time)
{
    float v = sin(TWO_PI * tone1 * time);
    v *= fmod(time, 1.f);
    v *= 0.5; 
    return v;
}

float sound_generate_dual_tone(const float time)
{
    float v = sin(TWO_PI * tone1 * time) + sin(TWO_PI * tone2 * time);
    v *= fmod(time, 1.f);
    v *= 0.5; 
    return v;
}

int sound_generate_sine( const float freq ) 
{
    /**
     * check if sound available
     */
    if( !sound_get_available() ) {
        return -1;
    }
#ifdef NATIVE_64BIT

#else
    #if defined( LILYGO_WATCH_2020_V1 ) || defined( LILYGO_WATCH_2020_V3 )
        if(wav->isRunning())
        {
            return 1; 
        }   

        if ( sound_config.enable && sound_init && !sound_is_silenced() && !wav->isRunning() ) {
            sound_set_enabled( sound_config.enable );
            tone1 = freq;
            funsource = new AudioFileSourceFunction(.5);
            funsource->addAudioGenerators(sound_generate_sine_tone);
            wav->begin(funsource, out);
            return 0; 
        } else {
            log_i("Cannot generate sine tone, sound is disabled");
            return -1; 
        }
    #endif
#endif
}

void sound_generate_mf_string(char *str) 
{
    int x, len;

    /**
     * check if sound available
     */
    if( !sound_get_available() ) {
        return;
    }
#ifdef NATIVE_64BIT

#else
    #if defined( LILYGO_WATCH_2020_V1 ) || defined( LILYGO_WATCH_2020_V3 )
        if(wav->isRunning()) 
        {
            return;  
        }

        if ( sound_config.enable && sound_init && !sound_is_silenced() && !wav->isRunning() ) 
        {
            len = strlen(str);
            if(len <= 0)
            {
                return; 
            }
            sound_set_enabled( sound_config.enable );

	    for (x = 0; x< len; x++)
	    {
		switch(str[x]) 
                {
		    case '1':
                        tone1 = 700.f; 
                        tone2 = 900.f; 
			break;
		    case '2':
                        tone1 = 700.f;
                        tone2 = 1100.f;  
			break;
		    case '3':
                        tone1 = 900.f; 
                        tone2 = 1100.f;  
			break;
		    case '4':
                        tone1 = 700.f; 
                        tone2 = 1300.f; 
			break;
		    case '5':
                        tone1 = 900.f; 
                        tone2 = 1300.f; 
			break;
		    case '6':
                        tone1 = 1100.f; 
                        tone2 = 1300.f; 
			break;
		    case '7':
                        tone1 = 700.f; 
                        tone2 = 1500.f; 
			break;
		    case '8':
                        tone1 = 900.f; 
                        tone2 = 1500.f; 
			break;
		    case '9':
                        tone1 = 1100.f; 
                        tone2 = 1500.f; 
			break;
		    case '0':
                        tone1 = 1300.f;  
                        tone2 = 1500.f; 
			break;
		    // KP
		    case '\\':
                        tone1 = 1500.f; 
                        tone2 = 1700.f; 
			break;
		    // ST
		    case '/':
                        tone1 = 1100.f; 
                        tone2 = 1700.f; 
			break;
		    // " 2600 "
		    case '^':
                        tone1 = 2600.f; 
                        tone2 = 0.f; 
			break;
		}
                mf_funsource = new AudioFileSourceFunction(.4);
                mf_funsource->addAudioGenerators(sound_generate_dual_tone);
                wav->begin(mf_funsource, out);
                delay(500);
                 if ( wav->isRunning()) {
	             wav->stop();
                 }
            }
          return; 
        } else {
            log_i("Cannot generate DTMF, sound is disabled");
            return; 
        }
    #endif
#endif
}

void sound_generate_dtmf_string(char *str) 
{
    int x, len;

    /**
     * check if sound available
     */
    if( !sound_get_available() ) {
        return;
    }
#ifdef NATIVE_64BIT

#else
    #if defined( LILYGO_WATCH_2020_V1 ) || defined( LILYGO_WATCH_2020_V3 )
        if(wav->isRunning()) 
        {
            return;  
        }

        if ( sound_config.enable && sound_init && !sound_is_silenced() && !wav->isRunning() ) 
        {
            len = strlen(str);
            if(len <= 0)
            {
                return; 
            }
            sound_set_enabled( sound_config.enable );

	    for (x = 0; x< len; x++)
	    {
		switch(str[x]) 
                {
		    case '1':
                        tone1 = 697.f; 
                        tone2 = 1209.f; 
			break;
		    case '2':
                        tone1 = 697.f; 
                        tone2 = 1336.f; 
			break;
		    case '3':
                        tone1 = 697.f; 
                        tone2 = 1477.f; 
			break;
		    case '4':
                        tone1 = 770.f; 
                        tone2 = 1209.f; 
			break;
		    case '5':
                        tone1 = 770.f; 
                        tone2 = 1336.f; 
			break;
		    case '6':
                        tone1 = 770.f; 
                        tone2 = 1477.f; 
			break;
		    case '7':
                        tone1 = 852.f; 
                        tone2 = 1209.f; 
			break;
		    case '8':
                        tone1 = 852.f; 
                        tone2 = 1336.f; 
			break;
		    case '9':
                        tone1 = 852.f; 
                        tone2 = 1477.f; 
			break;
		    case '0':
                        tone1 = 941.f; 
                        tone2 = 1336.f; 
			break;
                    case 'A': 
                        tone1 = 697.f; 
                        tone2 = 1633.f; 
                        break;
                    case 'B':
                        tone1 = 770.f; 
                        tone2 = 1633.f; 
                        break;
                    case 'C':
                        tone1 = 852.f;
                        tone2 = 1633.f;
                        break;
                    case 'D': 
                        tone1 = 941.f; 
                        tone2 = 1633.f; 
                        break;
                    case '*':
                        tone1 = 941.f; 
                        tone2 = 1209.f; 
                        break;
                    case '#':
                        tone1 = 941.f; 
                        tone2 = 1477.f; 
                        break;
		}
                dt_funsource = new AudioFileSourceFunction(.4);
                dt_funsource->addAudioGenerators(sound_generate_dual_tone);
                wav->begin(dt_funsource, out);
                delay(500);
                 if ( wav->isRunning()) {
	             wav->stop();
                 }
            }
          return; 
        } else {
            log_i("Cannot generate DTMF, sound is disabled");
            return; 
        }
    #endif
#endif
}

void mf_app_task(void * pvParameters)
{
    mf_insound = (char *)pvParameters; 

    sound_generate_mf_string(mf_insound);
    vTaskDelay(100);
    free(mf_insound); 
    mf_insound = NULL; 
    vTaskDelete(NULL);
}

void dtmf_app_task(void * pvParameters)
{
    dt_insound = (char *)pvParameters; 

    sound_generate_dtmf_string(dt_insound);
    vTaskDelay(100);
    free(dt_insound); 
    dt_insound = NULL; 
    vTaskDelete(NULL);
}

// The supported audio codec in ESP32 A2DP is SBC. SBC audio stream is encoded
// from PCM data normally formatted as 44.1kHz sampling rate, two-channel 16-bit sample data
int32_t get_data_channels(Frame *frame, int32_t channel_len) {
    static double m_time = 0.0;
    double m_amplitude = 10000.0;  // -32,768 to 32,767
    double m_deltaTime = 1.0 / 44100.0;
    double m_phase = 0.0;
    double double_Pi = PI * 2.0;
    // fill the channel data
    for (int sample = 0; sample < channel_len; ++sample) {
        double angle = double_Pi * c3_frequency * m_time + m_phase;
        frame[sample].channel1 = m_amplitude * sin(angle);
        frame[sample].channel2 = frame[sample].channel1;
        m_time += m_deltaTime;
    }

    return channel_len;
}

void sound_a2dp_source(void) 
{
    if( sound_config.enable && sound_init && !sound_is_silenced() && blectl_get_autoon() ) 
    {
        is_bt = true;
        a2dp_source.start("HyperGear BT100", get_data_channels);  
        a2dp_source.set_volume(30);
    }
    else
    {
        log_i("Cannot enable A2DP source per settings");
    }
}

// Results are awful - why? 
void sound_play_spiffs_midi( const char *filename, const char *soundfont ) {
    /**
     * check if sound available
     */
    if( !sound_get_available() ) {
        return;
    }
#ifdef NATIVE_64BIT

#else
    #if defined( LILYGO_WATCH_2020_V1 ) || defined( LILYGO_WATCH_2020_V3 )
        if ( sound_config.enable && sound_init && !sound_is_silenced() ) {
            sound_set_enabled( sound_config.enable );
            log_i("playing MIDI %s from SPIFFS with sf2 %s", filename, soundfont);
            spliffs_file = new AudioFileSourceSPIFFS(filename);
            midi_sf2 = new AudioFileSourceSPIFFS(soundfont);

            midi = new AudioGeneratorMIDI();
            midi->SetSoundfont(midi_sf2);
            midi->begin(spliffs_file, out);
        } else {
            log_i("Cannot play MIDI, sound is disabled");
        }
    #endif
#endif
}

void sound_play_spiffs_mp3( const char *filename ) {
    /**
     * check if sound available
     */
    if( !sound_get_available() ) {
        return;
    }
#ifdef NATIVE_64BIT

#else
    #if defined( LILYGO_WATCH_2020_V1 ) || defined( LILYGO_WATCH_2020_V3 )
        if ( sound_config.enable && sound_init && !sound_is_silenced() && !mp3->isRunning() ) {
            sound_set_enabled( sound_config.enable );
            log_i("playing file %s from SPIFFS", filename);
            spliffs_file = new AudioFileSourceSPIFFS(filename);
            id3 = new AudioFileSourceID3(spliffs_file);
            mp3->begin(id3, out);
        } else {
            log_i("Cannot play mp3, sound is disabled");
        }
    #endif
#endif
}

void sound_play_progmem_wav( const void *data, uint32_t len ) {
    /**
     * check if sound available
     */
    if( !sound_get_available() ) {
        return;
    }
#ifdef NATIVE_64BIT

#else
    #if defined( LILYGO_WATCH_2020_V1 ) || defined( LILYGO_WATCH_2020_V3 )
        if ( sound_config.enable && sound_init && !sound_is_silenced() && !wav->isRunning() ) {
            sound_set_enabled( sound_config.enable );
            log_i("playing audio (size %d) from PROGMEM ", len );
            progmem_file = new AudioFileSourcePROGMEM( data, len );
            wav->begin(progmem_file, out);
        } else {
            log_i("Cannot play wav, sound is disabled");
        }
    #endif
#endif
}

void sound_speak( const char *str ) {
        return;
}

void sound_save_config( void ) {
    /**
     * check if sound available
     */
    if( !sound_get_available() ) {
        return;
    }

    sound_config.save();
}

void sound_read_config( void ) {
    /**
     * check if sound available
     */
    if( !sound_get_available() ) {
        return;
    }

    sound_config.load();
}

bool sound_get_enabled_config( void ) {
    /**
     * check if sound available
     */
    if( !sound_get_available() ) {
        return( false );
    }

    return sound_config.enable;
}

void sound_set_enabled_config( bool enable ) {
    /**
     * check if sound available
     */
    if( !sound_get_available() ) {
        return;
    }

    sound_config.enable = enable;
    if ( sound_config.enable) {
        sound_set_enabled( true );
    }
    else {
        sound_set_enabled( false );
    }
    sound_send_event_cb( SOUNDCTL_ENABLED, (void *)&sound_config.enable ); 
}

uint8_t sound_get_volume_config( void ) {
    /**
     * check if sound available
     */
    if( !sound_get_available() ) {
        return( 0 );
    }

    return( sound_config.volume );
}

void sound_set_volume_config( uint8_t volume ) {
    /**
     * check if sound available
     */
    if( !sound_get_available() ) {
        return;
    }

    sound_config.volume = volume;
        
#ifdef NATIVE_64BIT

#else
    #if defined( LILYGO_WATCH_2020_V1 ) || defined( LILYGO_WATCH_2020_V3 )
        if ( sound_config.enable && sound_init ) {
            // limiting max gain here (max poss gain is 4.0)
            out->SetGain(0.5f * ( sound_config.volume / 100.0f ));
        }
    #endif
#endif
    sound_send_event_cb( SOUNDCTL_VOLUME, (void *)&sound_config.volume ); 
}


bool sound_is_silenced( void ) {
    if ( !sound_config.silence_timeframe ) {
        //log_i("no silence sound timeframe");
        return( false );
    }

    struct tm start;
    struct tm end;
    start.tm_hour = sound_config.silence_start_hour;
    start.tm_min = sound_config.silence_start_minute;
    end.tm_hour = sound_config.silence_end_hour;
    end.tm_min = sound_config.silence_end_minute;

    return timesync_is_between( start, end );
}

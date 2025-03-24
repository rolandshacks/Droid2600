package system;

import android.media.AudioFormat;
import android.media.AudioManager;
import android.media.AudioTrack;
import android.media.audiofx.PresetReverb;
import android.os.Process;

import emu.Emu;
import util.LogManager;
import util.Logger;

/**
 * Created by roland on 12.09.2016.
 */

public class AudioControl {

    public class AudioSpec {

        private final int numSamplesPerSec = 44100;
        private final int numFragmentSamples = 512;
        private final int numBits = 16;
        private final int numChannels = 1;

        private int numFragmentBuffers;
        private int numFragmentBufferBytes;

        public AudioSpec(int hardwareBufferSize) {
            numFragmentBufferBytes = numFragmentSamples * numChannels * numBits / 8;
            numFragmentBuffers = (hardwareBufferSize + numFragmentBufferBytes - 1) / numFragmentBufferBytes;
        }

        public int getNumSamplesPerSec() {
            return numSamplesPerSec;
        }

        public int getNumFragmentSamples() {
            return numFragmentSamples;
        }

        public int getNumBits() {
            return numBits;
        }

        public int getNumChannels() {
            return numChannels;
        }

        public int getNumFragmentBuffers() {
            return numFragmentBuffers;
        }

        public int getNumFragmentBufferBytes() {
            return numFragmentBufferBytes;
        }
    }

    private final static Logger logger = LogManager.getLogger(AudioControl.class.getName());

    private AudioSpec audioSpec;
    private Thread audioThread;
    private Object[] audioBuffers;
    private volatile int audioOutputBufferFilled;
    private volatile boolean audioOutputBufferReady;
    private int audioBufferInputIndex;
    private int audioBufferOutputIndex;

    private AudioTrack audioTrack;
    private PresetReverb audioReverb;

    private volatile boolean running;

    public AudioControl() {
        ;
    }

    public void init() {
        ;
    }

    public synchronized void start() {

        running = true;

        Preferences prefs = Preferences.instance();

        int minBufferSize = AudioTrack.getMinBufferSize(44100, AudioFormat.CHANNEL_OUT_MONO, AudioFormat.ENCODING_PCM_16BIT);

        audioSpec = new AudioSpec(minBufferSize);

        audioTrack = new AudioTrack(AudioManager.STREAM_MUSIC, 44100,
                AudioFormat.CHANNEL_OUT_MONO,
                AudioFormat.ENCODING_PCM_16BIT,
                minBufferSize,
                AudioTrack.MODE_STREAM);

        audioBuffers = new Object[audioSpec.getNumFragmentBuffers()];

        for (int i=0; i<audioSpec.getNumFragmentBuffers(); i++) {
            audioBuffers[i] = new byte[audioSpec.getNumFragmentBufferBytes()];
        }

        audioOutputBufferReady = false;
        audioOutputBufferFilled = audioBufferInputIndex = audioBufferOutputIndex = 0;

        setVolume(prefs.getAudioVolumePercent());

        // initialize reverb effect
        try {
            audioReverb = new PresetReverb(0, audioTrack.getAudioSessionId());
            audioReverb.setPreset(PresetReverb.PRESET_LARGEHALL);
            setReverb(prefs.isReverbEnabled());
        } catch (IllegalArgumentException e) {
            audioReverb = null;
            logger.info("Audio reverb not supported. Disabling effect.");
        }

        if (null != audioReverb) {
            audioTrack.attachAuxEffect(audioReverb.getId());
            audioTrack.setAuxEffectSendLevel(1.0f);
        }

        audioThread = new Thread(new Runnable() {

            @Override
            public void run() {
                audioLoop();
            }

        });

        try {
            audioThread.setPriority(Thread.MAX_PRIORITY);
        } catch (SecurityException e) {
            logger.warning("MAX_PRIORITY not allowed for audio emuThread");
        }

        audioThread.start();
        logger.info("started audio output");

    }

    public synchronized void stop() {

        running = false;

        if (null != audioThread) {
            audioThread.interrupt();
        }

        if (null != audioThread) {
            try {
                audioThread.join();
            } catch (InterruptedException e) {
                ;
            }
            audioThread = null;
            logger.info("stopped audio output");
        }

        if (null != audioTrack) {
            audioTrack.pause();
            audioTrack.flush();
            audioTrack.release();
            audioTrack = null;
        }

        audioBuffers = null;
    }

    public synchronized void pause() {

        if (null != audioTrack) {
            audioTrack.pause();
            audioTrack.flush();
        }
    }

    public synchronized void resume() {

        if (null != audioTrack) {
            audioTrack.play();
        }
    }

    public byte[] getInputBuffer() {
        return (byte[]) audioBuffers[audioBufferInputIndex];
    }

    public void nextInputBuffer() {

        int numBuffers = audioSpec.getNumFragmentBuffers();

        if (audioOutputBufferFilled < numBuffers-1) {
            audioOutputBufferFilled++;
            audioBufferInputIndex = (audioBufferInputIndex+1) % numBuffers;
            //logger.info("audio: set input buffer " + audioBufferInputIndex);

            if (false == audioOutputBufferReady) {
                if (audioOutputBufferFilled > 0) {
                        audioOutputBufferReady = true;
                }
            }
        }
    }

    private void audioLoop() {

        Process.setThreadPriority(Process.THREAD_PRIORITY_AUDIO);

        while (running && !Thread.interrupted()) {
            boolean status = updateAudio();
            if (false == status) {
                try {
                    Thread.sleep(5);
                } catch (InterruptedException e) {
                    break;
                }
            }
        }

        running = false;

        logger.info("stopped audio loop");
    }

    private boolean processAudioInputSync() {

        if (audioOutputBufferFilled >= 1) {
            return true;
        }

        byte[] audioBuffer = getInputBuffer();

        boolean result = Emu.instance().updateAudioAsync(audioBuffer, audioSpec.getNumFragmentBufferBytes());
        if (true == result) {
            nextInputBuffer();
        }

        return result;
    }

    private boolean processAudioInput() {

        if (false == processAudioInputSync()) {
            return false;
        }

        int playState = audioTrack.getPlayState();

        if (audioOutputBufferFilled < 1) {

            if (AudioTrack.PLAYSTATE_PAUSED != playState) {
                audioTrack.pause();
                logger.trace("audio paused");
            }

            return false;

        } else {

            if (AudioTrack.PLAYSTATE_PAUSED == playState) {
                audioTrack.play();
                logger.trace("audio resumed");
            }

        }

        return true;
    }

    private boolean processAudioOutput() {
        //logger.info("audio: reading from buffer " + audioBufferOutputIndex);

        byte[] audioData = (byte[]) audioBuffers[audioBufferOutputIndex];

        // playback
        int bytesWritten = audioTrack.write(audioData, 0,  audioData.length);

        // free buffer and increase index
        if (audioOutputBufferFilled > 0) {
            audioOutputBufferFilled--;
            audioBufferOutputIndex = (audioBufferOutputIndex+1) % audioSpec.getNumFragmentBuffers();
        }

        return (bytesWritten > 0);
    }

    public boolean updateAudio() {

        if (null == audioTrack) {
            return false;
        }

        //Log.w("emu", "audio statis: " + audioBufferInputIndex + " / " + audioBufferOutputIndex + " / " + audioOutputBufferFilled);

        // get audio buffer

        if (false == processAudioInput()) {
            return false; // no audio data available
        }

        if (false == processAudioOutput()) {
            return false;
        }

        if (audioOutputBufferFilled > 0) {
            return false; // indicate loop sleep
        }

        return true;
    }

    public void reset() {
        audioBufferInputIndex = 0;
        audioBufferOutputIndex = 0;
        audioOutputBufferFilled = 0;
        audioOutputBufferReady = false;
    }

    public void setReverb(boolean reverbEnabled) {
        if (null != audioReverb) {
            audioReverb.setEnabled(reverbEnabled);
        }
    }

    public void setVolume(int volume) {
        if (null != audioTrack) {
            float gain = ((float) volume) / 100.0f;
            audioTrack.setStereoVolume(gain, gain);
        }
    }
}

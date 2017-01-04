package system;

import android.app.Activity;
import android.content.res.AssetFileDescriptor;
import android.content.res.AssetManager;
import android.media.AudioManager;
import android.media.SoundPool;
import android.util.Log;

import java.io.IOException;
import java.util.Random;

/**
 * Created by roland on 04.01.2017.
 */

public class SoundEffects implements Runnable {

    private Activity activity;
    private SoundPool sp;
    private Thread updateThread;
    private int soundIds[] = new int[4];
    private Random rand;
    private Random randf;
    private volatile int numLoadedSounds;
    private float playbackTimeout;
    private long lastUpdateTime;

    public SoundEffects(Activity activity) {
        this.activity = activity;
    }

    public synchronized void start() {

        stop();

        activity.setVolumeControlStream(AudioManager.STREAM_MUSIC);

        rand = new Random();
        randf = new Random();

        updateThread = new Thread(this);
        playbackTimeout = 0.0f;
        lastUpdateTime = 0;

        sp = new SoundPool(3, AudioManager.STREAM_MUSIC, 0);

        sp.setOnLoadCompleteListener(new SoundPool.OnLoadCompleteListener() {
            @Override
            public void onLoadComplete(SoundPool soundPool, int sampleId, int status) {
                //Log.i("soundpool", "onLoadComplete: " + sampleId + " / " + status);
                if (0 == status) {
                    numLoadedSounds++;

                    if (numLoadedSounds >= soundIds.length) {
                        updateThread.start();
                    }
                }
            }
        });

        numLoadedSounds = 0;
        for (int i=0; i<soundIds.length; i++) {
            /*
            if (0==i) {
                soundIds[i] = loadSound("intro.mp3");
                continue;
            }
            soundIds[i] = loadSound("wind" + i + ".mp3");
            */
            soundIds[i] = loadSound("wind" + (i+1) + ".mp3");
        }

    }

    @Override
    public void run() {
        while (!Thread.interrupted()) {

            try {
                Thread.sleep(250);
            } catch (InterruptedException e) {
                break;
            }

            long currentTime = System.currentTimeMillis();
            float elapsed = (lastUpdateTime != 0) ? ((float) (currentTime-lastUpdateTime))/1000.0f : 0.0f;
            lastUpdateTime = currentTime;

            updateSound(elapsed);
        }
    }

    public synchronized void stop() {

        if (null != updateThread) {

            updateThread.interrupt();

            try {
                updateThread.join(1000);
            } catch (InterruptedException e) {
                ;
            }
            updateThread = null;
        }

        if (null != sp) {
            sp.autoPause();
            sp.release();
            sp = null;
        }

    }

    private int loadSound(String name) {

        int soundId = 0;

        AssetManager assetManager = activity.getAssets();

        try {
            AssetFileDescriptor afd = null;
            afd = assetManager.openFd(name);
            soundId = sp.load(afd, 1);
            afd.close();
        } catch (IOException e) {
            return 0;
        }

        return soundId;

    }

    private void updateSound(float elapsed) {

        if (null == sp) return;

        playbackTimeout -= elapsed;

        if (playbackTimeout > 0.0f || numLoadedSounds < soundIds.length) {
            return;
        }

        playbackTimeout = 7.0f;

        int idx = rand.nextInt(soundIds.length);

        float volMaster = 0.3f;
        float volLeft = randf.nextFloat() * volMaster;
        float volRight = randf.nextFloat() * volMaster;

        int streamId = sp.play(soundIds[idx], volLeft, volRight, 1, 0, 1.0f);

        //Log.i("soundthread", "playing: " + idx + "/" + volLeft + "/" + volRight + " ->  streamid: " + streamId);
    }

}

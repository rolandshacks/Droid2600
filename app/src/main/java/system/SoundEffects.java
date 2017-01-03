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

public class SoundEffects {

    private Activity activity;
    private SoundPool sp;

    public SoundEffects(Activity activity) {
        this.activity = activity;
    }

    public synchronized void start() {

        stop();

        activity.setVolumeControlStream(AudioManager.STREAM_MUSIC);

        sp = new SoundPool(3, AudioManager.STREAM_MUSIC, 0);

        sp.setOnLoadCompleteListener(new SoundPool.OnLoadCompleteListener() {
            @Override
            public void onLoadComplete(SoundPool soundPool, int sampleId, int status) {
            if (0 == status) {
                sp.play(sampleId, 1.0f, 1.0f, 1, 0, 1.0f);
            }
            }
        });

        loadSound("intro.mp3");

    }

    public synchronized void stop() {

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

}

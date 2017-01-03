package system;

import android.content.Context;
import android.content.SharedPreferences;

import emu.Emu;
import util.LogManager;
import util.Logger;

/**
 * Created by roland on 25.08.2016.
 */

public class Preferences {

    private final static Logger logger = LogManager.getLogger(Preferences.class.getName());
    private static Preferences globalInstance;
    public static final String PREFS_NAME = "Droid64Prefs";
    private static final int DEFAULT_VOLUME_PERCENT = 50;

    private Context context;

    private boolean antialiasingEnabled;
    private boolean warpEnabled;
    private boolean swapJoysticks;
    private boolean reverbEnabled;
    private boolean gammaCorrectionEnabled;
    private boolean maxZoomEnabled;
    private boolean stretchZoomEnabled;
    private boolean textureCompressionEnabled;
    private boolean zipScanEnabled;
    private int audioVolume;

    private boolean initializing;

    public static Preferences instance() {
        return globalInstance;
    }

    public Preferences() {
        globalInstance = this;
    }

    public void init(Context context) {

        this.context = context;

        initializing = true;

        setAntialiasingEnabled(false);
        setGammaCorrectionEnabled(false);
        setMaxZoomEnabled(false);
        setStretchZoomEnabled(true);
        setWarpEnabled(false);
        setJoystickSwapEnabled(false);
        setReverbEnabled(false);
        setTextureCompressionEnabled(false);
        setAudioVolumePercent(DEFAULT_VOLUME_PERCENT);
        setZipScanEnabled(false);

        initializing = false;
    }

    public void load() {
        if (null == context) return;

        SharedPreferences settings = context.getSharedPreferences(PREFS_NAME, 0);
        if (null == settings) return;

        initializing = true;

        setAntialiasingEnabled(settings.getBoolean("antialiasing", isAntialiasingEnabled()));
        setGammaCorrectionEnabled(settings.getBoolean("gamma", isGammaCorrectionEnabled()));
        setMaxZoomEnabled(settings.getBoolean("maxzoom", isMaxZoomEnabled()));
        setStretchZoomEnabled(settings.getBoolean("stretchzoom", isStretchZoomEnabled()));
        setWarpEnabled(settings.getBoolean("warp", isWarpEnabled()));
        setJoystickSwapEnabled(settings.getBoolean("joystickswap", isJoystickSwapEnabled()));
        setReverbEnabled(settings.getBoolean("reverb", isReverbEnabled()));
        setZipScanEnabled(settings.getBoolean("zipscan", isZipScanEnabled()));

        int vol = getAudioVolumePercent();

        setAudioVolumePercent(settings.getInt("volume", vol));

        logger.info("loaded audio volume " + audioVolume + "%");

        initializing = false;
    }

    private void save() {

        if (initializing) return;
        if (null == context) return;

        SharedPreferences settings = context.getSharedPreferences(PREFS_NAME, 0);
        if (null == settings) return;

        SharedPreferences.Editor editor = settings.edit();

        editor.putBoolean("antialiasing", isAntialiasingEnabled());
        editor.putBoolean("gamma", isGammaCorrectionEnabled());
        editor.putBoolean("maxzoom", isMaxZoomEnabled());
        editor.putBoolean("stretchzoom", isStretchZoomEnabled());
        editor.putBoolean("warp", isWarpEnabled());
        editor.putBoolean("joystickswap", isJoystickSwapEnabled());
        editor.putBoolean("reverb", isReverbEnabled());
        editor.putBoolean("zipscan", isZipScanEnabled());
        editor.putInt("volume", getAudioVolumePercent());

        editor.commit();

        logger.info("saved audio volume " + audioVolume + "%");
    }

    public boolean isAntialiasingEnabled() {
        return antialiasingEnabled;
    }

    public void setAntialiasingEnabled(boolean antialiasingEnabled) {
        this.antialiasingEnabled = antialiasingEnabled;
        save();
    }

    public boolean isWarpEnabled() {
        return warpEnabled;
    }

    public void setWarpEnabled(boolean warpEnabled) {

        this.warpEnabled = warpEnabled;
        save();

        Emu emuControl = Emu.instance();
        if (null != emuControl) {
            emuControl.setWarpMode(warpEnabled);
        }

    }

    public boolean isJoystickSwapEnabled() {
        return swapJoysticks;
    }

    public void setJoystickSwapEnabled(boolean swapJoysticks) {

        this.swapJoysticks = swapJoysticks;
        save();

        Emu emuControl = Emu.instance();
        if (null != emuControl) {
            emuControl.setJoystickSwap(swapJoysticks);
        }

    }

    public boolean isReverbEnabled() {
        return reverbEnabled;
    }

    public void setReverbEnabled(boolean reverbEnabled) {
        this.reverbEnabled = reverbEnabled;
        save();

        Emu emuControl = Emu.instance();
        if (null != emuControl) {
            emuControl.setReverbEnabled(reverbEnabled);
        }

    }

    public boolean isGammaCorrectionEnabled() {
        return gammaCorrectionEnabled;
    }

    public void setGammaCorrectionEnabled(boolean gammaCorrectionEnabled) {
        this.gammaCorrectionEnabled = gammaCorrectionEnabled;
        save();
    }

    public boolean isTextureCompressionEnabled() {
        return textureCompressionEnabled;
    }

    public void setTextureCompressionEnabled(boolean textureCompressionEnabled) {
        this.textureCompressionEnabled = textureCompressionEnabled;
        save();
    }

    public int getAudioVolumePercent() {
        return audioVolume;
    }

    public void setAudioVolumePercent(int audioVolume) {

        logger.info("setAudioVolumePercent:" + audioVolume);

        this.audioVolume = audioVolume;
        save();

        Emu emuControl = Emu.instance();
        if (null != emuControl) {
            emuControl.setAudioVolume(audioVolume);
        }

    }

    public boolean isZipScanEnabled() {
        return zipScanEnabled;
    }

    public void setZipScanEnabled(boolean zipScanEnabled) {
        this.zipScanEnabled = zipScanEnabled;
        save();
    }

    public boolean isMaxZoomEnabled() {
        return maxZoomEnabled;
    }

    public void setMaxZoomEnabled(boolean maxZoomEnabled) {
        this.maxZoomEnabled = maxZoomEnabled;
        save();
    }

    public boolean isStretchZoomEnabled() {
        return stretchZoomEnabled;
    }

    public void setStretchZoomEnabled(boolean stretchZoomEnabled) {
        this.stretchZoomEnabled = stretchZoomEnabled;
        save();
    }
}

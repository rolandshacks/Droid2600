package emu;

import system.AudioControl;
import system.Preferences;
import system.Statistics;
import util.LogManager;
import util.Logger;

import android.util.Log;

public class Emu {

	private final static Logger logger = LogManager.getLogger(Emu.class.getName());

    public static final int FRAMES_PER_SECOND = 50;
    private static final int NORMAL_SLEEP_TIME = 1000 / FRAMES_PER_SECOND;
    private static final int PAUSE_SLEEP_TIME = 100;
    
	public static final int DISPLAY_X = 160;
	public static final int DISPLAY_Y = 320;

	public static final int DISPLAY_SHOW_X = 144;
	public static final int DISPLAY_SHOW_Y = 210;

	private static final int DISPLAY_PIXELS = DISPLAY_X * DISPLAY_Y;
	private static final int NUM_VIDEO_BUFFERS = 2;

	private static final int FLAG_UNPACK_GRAPHICS = 0x1;
	private static final int FLAG_USE_GAMMA_CORRECTION = 0x2;
	private static final int FLAG_SWAP_JOYSTICK = 0x4;

	private Thread emuThread;

	private final Object emuLock = new Object();
	private NativeInterface emu;

	private Object[] videoBuffers = new Object[NUM_VIDEO_BUFFERS];
	private int videoBufferIndex;
	private volatile boolean textureBufferFilled;
	private int textureBufferIndex;

	private byte[] emuStats = new byte[12];

	private int displayWidth;
	private int displayHeight;
	private int displayYStart;

	private byte[] snapshotBuffer;
	private int snapshotBufferUsage;

	private volatile boolean running;
	private volatile boolean paused;
	private volatile boolean warpMode;
	private volatile boolean active;

	private int stickMask = 0x0;
	
	private Statistics emuStatistics = new Statistics();

	private static Emu globalInstance = null;

	private AudioControl audioControl = new AudioControl();
	
	public static Emu instance() {
	    return globalInstance;
	}
	
	public Emu() {
		globalInstance = this;
	}

	public void init() {

		active = false;
		paused = true;

		Preferences prefs = Preferences.instance();
		boolean textureCompressed = prefs.isTextureCompressionEnabled();

		int bitsPerPixels = textureCompressed ? 4 : 32;
		int videoBufferSize = DISPLAY_PIXELS * bitsPerPixels / 8;

		for (int i=0; i<NUM_VIDEO_BUFFERS; i++) {
			videoBuffers[i] = new byte[videoBufferSize];
		}

		audioControl.init();
	}

	public synchronized void start() {
		
		logger.info("starting emulator");

		if (running) return;

		running = true;

		Preferences prefs = Preferences.instance();

		textureBufferFilled = false;
		textureBufferIndex = 0;
		videoBufferIndex = 0;

		setStick(0x0);

		emuThread = new Thread(new Runnable() {

			@Override
			public void run() {
				emuLoop();
			}
		});
		emuThread.start();
	}

	private void emuLoop() {

		logger.info("started emulator process");

		if (!Thread.interrupted()) {
			emu = new NativeInterface();

			Log.d("emu", "initializing emulator kernel");

			StringBuilder prefsDocument = new StringBuilder();

			Preferences prefs = Preferences.instance();

			prefsDocument.append("JoystickSwap = " + (prefs.isJoystickSwapEnabled() ? "TRUE" : "FALSE") + "\n");

			if (0 != emu.init(prefsDocument.toString(), 0x0)) {
				Log.e("emu", "failed to initialize emulator kernel");
				return;
			}

			logger.info("initialized emulator kernel");

			/////// JUST FOR TESTING....
			// Image startImage = new Image("/sdcard/Download/droid2600/rom.bin");
			// startImage.load();
			// ImageManager.instance().setCurrent(startImage);

			Image currentDisk = ImageManager.instance().getCurrent();
			if (null != currentDisk) {
				if (attachImage(currentDisk)) {
					paused = false;
					active = true;
				}
			}

		}

		emuStatistics.reset();

		long nextUpdateTime = 0;
		long cycleTime = (long) NORMAL_SLEEP_TIME * 1000000;

		boolean vblankOccured = false;

		audioControl.start();
		audioControl.pause();

		Preferences prefs = Preferences.instance();

		while (running && !Thread.interrupted()) {

			if (paused || !active) {

				audioControl.pause();

				while (running && (paused || !active) && !Thread.interrupted()) {
					try {
						Thread.sleep(PAUSE_SLEEP_TIME);
					} catch (InterruptedException e) {
						break;
					}

					//logger.info("PAUSED...");
				}

				if (Thread.interrupted()) {
					break;
				}

				audioControl.reset();

			} else {

				// time sync on vblank
				if (vblankOccured) {

					emuStatistics.update();

					if (false == warpMode) {

						long currentTime = System.nanoTime();

						if (currentTime < nextUpdateTime) {
							long waitTime = (nextUpdateTime - currentTime);

							//LockSupport.parkNanos(waitTime);

							try {
								Thread.sleep((long) (waitTime / 1000000));
							} catch (InterruptedException e) {
								break;
							}
						}

						if (0 == nextUpdateTime) {
							nextUpdateTime = currentTime + cycleTime;
						} else {
							nextUpdateTime += cycleTime;
							if (nextUpdateTime < currentTime - cycleTime * 2) {
								//logger.info("Update limiter (" + (currentTime - nextUpdateTime) + ")");
								nextUpdateTime = currentTime - cycleTime * 2;
							}
						}
					}
				}

				byte[] videoBuffer = (byte[]) videoBuffers[videoBufferIndex];
				int flags = 0;

				if (!prefs.isTextureCompressionEnabled()) flags |= FLAG_UNPACK_GRAPHICS;
				if (prefs.isGammaCorrectionEnabled()) flags |= FLAG_USE_GAMMA_CORRECTION;
				if (prefs.isJoystickSwapEnabled()) flags |= FLAG_SWAP_JOYSTICK;

				/////////////////////////////////////////////////////////////////////////////////
				// UPDATE EMULATOR
				/////////////////////////////////////////////////////////////////////////////////

				int updateStatus = 0;

				synchronized(emuLock) {
					emu.updateInput(stickMask, flags);
					updateStatus = emu.updateVideo(videoBuffer, emuStats, flags);
				}

				if (1 == updateStatus) {

					int ofs =0;

					displayWidth = decodeInt(emuStats, ofs); ofs+=4;
					displayHeight = decodeInt(emuStats, ofs); ofs+=4;
					displayYStart = decodeInt(emuStats, ofs); ofs+=4;

					//logger.info("emu stats: " + displayWidth + " / " + displayHeight + " / " + displayYStart);
				}

				/////////////////////////////////////////////////////////////////////////////////
				/////////////////////////////////////////////////////////////////////////////////

				vblankOccured = (1 == updateStatus);

				if (vblankOccured) {

					if (false == textureBufferFilled) {
						textureBufferIndex = videoBufferIndex;
						videoBufferIndex = (videoBufferIndex+1) % NUM_VIDEO_BUFFERS;
						textureBufferFilled = true;
						//logger.info("texture buffer filled");
					}

				}

				updateAudio();

			}

		}

		audioControl.stop();

		logger.info("shutdown emulator");

		emu.shutdown();
		emu = null;

		running = false;
		active = false;

		logger.info("finished emulator process");
	}

	private void updateAudio() {

		/*
		if (audioControl.needInput()) {
			byte[] audioBuffer = audioControl.getInputBuffer();
			emu.updateAudio(audioBuffer, 0);
			audioControl.nextInputBuffer();
		}
		*/

	}

	public boolean updateAudioAsync(byte[] audioBuffer, int audioBufferSize) {

		NativeInterface e = emu;

		if (null == e || !active || !running) {
			return false;
		}

		if (0 == e.updateAudio(audioBuffer, audioBufferSize)) {
			return false;
		}

		return true;
	}

	private int decodeInt(byte[] buffer, int ofs) {
		int value =
				((buffer[ofs+3]&0xff) << 24) |
				((buffer[ofs+2]&0xff) << 16) |
				((buffer[ofs+1]&0xff) << 8) |
				((buffer[ofs+0]&0xff));

		return value;
	}

	public synchronized void stop() {

		if (!running) return;

	    running = false;
	    paused = false;
		active = false;

		if (null != emuThread) {
			emuThread.interrupt();
		}

		audioControl.stop();

		if (null != emuThread) {
			try {
				emuThread.join();
			} catch (InterruptedException e) {
				;
			}
			emuThread = null;

			logger.info("stopped emulator");
		}
	}
	
	public synchronized void pause() {
		paused = true;
		logger.info("paused emulator");
	}
	
	public synchronized void resume() {
		paused = false;
		logger.info("resumed emulator");
	}

	public boolean isPaused() {
		return paused;
	}
	public boolean isActive() {
		return active;
	}

	private void sendCommand(int command) {
		sendCommand(command, 0);
	}

	private void sendCommand(int command, int param) {

		if (null == emu) {
			return;
		}

		synchronized (emuLock) {
			emu.command(command, param);
		}
	}

	public boolean attachImage(Image image) {

		byte[] imageBuffer = image.load();
		if (null != imageBuffer) {
			ImageManager.instance().setCurrent(image);
			return attachImage(image.getType(), imageBuffer, imageBuffer.length,  image.getUrl());
		}

		return false;
	}

	private boolean attachImage(int type, byte[] data, int dataLength, String filename) {

		int status = 0;

		synchronized(emuLock) {
			status = emu.load(type, data, dataLength, filename);
		}

		if (0 != status) {
			logger.warning("failed to attach disk");
			return false;
		}

		paused = false; // auto-resume

		return true;
	}

	public String getImageInfo() {
		return emu.get("image.info");
	}

	public byte[] lockTextureData() {
		
		if (false == textureBufferFilled) {
			//logger.info("lock texture data: buffer not filled");
			return null;
		}

		byte[] buffer = (byte[]) videoBuffers[textureBufferIndex];

		if (null == buffer) {
			logger.warning("lock texture data: buffer is NULL");
		}

		//logger.info("lock texture data: successfull");
		
		return buffer;
	}
	
	public void unlockTextureData() {
		textureBufferFilled = false;
	}
	
	public void setStick(int stickMask) {
		if (this.stickMask != stickMask) {
			this.stickMask = stickMask;
            //logger.info("COMMAND setStick(" + stickMask + ")");
		}
	}
	
	public void setStickFlag(int flag) {
		setStick(this.stickMask | flag);
	}
	
	public void clearStickFlag(int flag) {
		setStick(this.stickMask & (~flag));
	}
	
	public int getStick() {
		return this.stickMask;
	}

    public double getUpdatesPerSecond() {
        return emuStatistics.getUpdatesPerSecond();
    }

	public void consoleReset() {
		sendCommand(NativeInterface.COMMAND_RESET);
	}

	public void consoleSelect() {
		sendCommand(NativeInterface.COMMAND_SELECT);
	}

	public void hardReset(Image autoStartImage) {
		stop();
		ImageManager.instance().setCurrent(autoStartImage);
		start();
	}

	public void setJoystickSwap(boolean enabled) {
		sendCommand(enabled ? NativeInterface.COMMAND_JOYSTICK_SWAP_ON : NativeInterface.COMMAND_JOYSTICK_SWAP_OFF);
	}

	public void setWarpMode(boolean warpEnabled) {
		warpMode = warpEnabled;
	}

	public void setReverbEnabled(boolean reverbEnabled) {
		if (null != audioControl) {
			audioControl.setReverb(reverbEnabled);
		}
	}

	public void setAudioVolume(int volume) {
		if (null != audioControl) {
			audioControl.setVolume(volume);
		}
	}

	public void storeSnapshot() {

		if (null == snapshotBuffer) {
			snapshotBuffer = new byte[128*1024];
		}

		int result = 0;

		synchronized (emuLock) {
			result = emu.store(Image.TYPE_SNAPSHOT, snapshotBuffer, snapshotBuffer.length);
		}

		if (result < 1) {
			logger.info("failed to store snapshot");
			snapshotBufferUsage = 0;
			return;
		}

		snapshotBufferUsage = result;

		logger.info("stored snapshot: " + snapshotBufferUsage + " bytes");

		ImageManager.instance().storeSnapshot(snapshotBuffer, snapshotBufferUsage);

	}

	public void restoreSnapshot() {
		if (null == snapshotBuffer || snapshotBufferUsage == 0) {
			return;
		}

		int result = 0;

		synchronized (emuLock) {
			result = emu.load(Image.TYPE_SNAPSHOT, snapshotBuffer, snapshotBufferUsage, "snapshot");
		}

		if (result != 0) {
			logger.info("failed to restore snapshot");
			return;
		}

		logger.info("restored snapshot");
	}

	public int getDisplayWidth() {
		return displayWidth;
	}

	public int getDisplayHeight() {
		return displayHeight;
	}

	public int getDisplayYStart() {
		return displayYStart;
	}

}

package ui;

import android.Manifest;
import android.annotation.TargetApi;
import android.app.ActivityManager;
import android.content.Context;
import android.content.pm.ActivityInfo;
import android.content.pm.ConfigurationInfo;
import android.content.pm.PackageManager;
import android.net.Uri;
import android.os.Bundle;
import android.support.v4.app.ActivityCompat;
import android.support.v4.app.FragmentActivity;
import android.support.v4.content.ContextCompat;
import android.view.InputDevice;
import android.view.KeyEvent;
import android.view.MotionEvent;
import android.view.View;
import android.view.ViewTreeObserver.OnGlobalLayoutListener;
import android.view.Window;
import android.view.WindowManager;
import android.widget.ArrayAdapter;
import android.widget.Toast;

import org.codewiz.droid2600.R;

import emu.Emu;
import emu.ImageFilter;
import emu.Image;
import emu.ImageManager;
import system.Preferences;
import system.GameController;
import system.GameControllerListener;
import system.SoundEffects;
import util.LogManager;
import util.Logger;
import util.SystemUiHider;

import java.util.HashMap;
import java.util.List;

/**
 * An example full-screen activity that shows and hides the system UI (i.e.
 * status bar and navigation/system bar) with user interaction.
 * 
 * @see SystemUiHider
 */
public class FullscreenActivity extends FragmentActivity implements FileDialog.OnDiskSelectHandler, EmuControlFragment.OnFragmentInteractionListener, EmuViewFragment.OnFragmentInteractionListener {

    private final static Logger logger = LogManager.getLogger(FullscreenActivity.class.getName());
    private final static boolean ENABLE_INTRO_SOUND = false;

    private static final int AREA_TOP = 0x1;
    private static final int AREA_MIDDLE = 0x2;
    private static final int AREA_BOTTOM = 0x4;
    private static final int AREA_LEFT = 0x10;
    private static final int AREA_CENTER = 0x20;
    private static final int AREA_RIGHT = 0x40;

    private static final int VBUTTON_LEFT_TOP = 1;
    private static final int VBUTTON_RIGHT_TOP = 2;
    private static final int VBUTTON_LEFT_BOTTOM = 3;
    private static final int VBUTTON_RIGHT_BOTTOM = 4;
    private static final int VBUTTON_CENTER_BOTTOM = 5;
    private static final int VBUTTON_CENTER_TOP = 6;

    private static final int COMMAND_CLICK_AREA = 15;
    private static final boolean EMU_PAUSED_WHILE_CONTROL = false;

    private Preferences emuPrefs;
    private Emu emuControl;

    private View emuView;
    private View controlsView;
    private android.app.DialogFragment settingsDialog;
    private android.app.DialogFragment fileDialog;

    private int eventSourceViewId = 0;

    private float mouseX = 0.0f;
    private float mouseY = 0.0f;

    private float mouseDownX = 0.0f;
    private float mouseDownY = 0.0f;

    private float screenWidth = 0.0f;
    private float screenHeight = 0.0f;

    private boolean mouseDown = false;

    private GameController gameController;
    private ImageManager diskManager;
    private SoundEffects introSound;

    private class StableArrayAdapter extends ArrayAdapter<String> {

        HashMap<String, Integer> mIdMap = new HashMap<String, Integer>();

        public StableArrayAdapter(Context context, int textViewResourceId, List<String> objects) {
            super(context, textViewResourceId, objects);
            
            for (int i = 0; i < objects.size(); ++i) {
                mIdMap.put(objects.get(i), i);
            }
        }

        @Override
        public long getItemId(int position) {
            String item = getItem(position);
            Integer itemId = mIdMap.get(item);
            return itemId;
        }

        @Override
        public boolean hasStableIds() {
            return true;
        }

    }

    private static final int REQUEST_READWRITE_STORAGE = 1234;

    public FullscreenActivity() {
        instantiateEmu();
    }

    private void instantiateEmu() {
        emuPrefs = Preferences.instance();
        if (null == emuPrefs) emuPrefs = new Preferences();

        diskManager = new ImageManager();

        emuControl = Emu.instance();
        if (null == emuControl) emuControl = new Emu();

        emuPrefs.init(this);
        emuControl.init();
    }

    private boolean checkPermissions() {

        int permissionCheck1 = ContextCompat.checkSelfPermission(this, Manifest.permission.READ_EXTERNAL_STORAGE);
        int permissionCheck2 = ContextCompat.checkSelfPermission(this, Manifest.permission.WRITE_EXTERNAL_STORAGE);

        logger.info("Permission check: " + permissionCheck1 + " / " + permissionCheck2);

        if (permissionCheck1 != PackageManager.PERMISSION_GRANTED ||
            permissionCheck2 != PackageManager.PERMISSION_GRANTED) {

            logger.info("Permission check failed - request permissions");

            ActivityCompat.requestPermissions(this,
                    new String[]{Manifest.permission.WRITE_EXTERNAL_STORAGE,
                            Manifest.permission.READ_EXTERNAL_STORAGE},
                    REQUEST_READWRITE_STORAGE);
        }

        return true;
    }

    @Override
    public void onRequestPermissionsResult(int requestCode,
                                           String[] permissions,
                                           int[] grantResults) {
        if (requestCode == REQUEST_READWRITE_STORAGE) {
            if ((grantResults.length > 0) && (grantResults[0] == PackageManager.PERMISSION_GRANTED)) {
                //finishCreationStep();

                logger.info("PERMISSION HAS BEEN GRANTED!!!");
            }
        }
    }

    boolean isEmuView(View v) {
        if (null == v) {
            return false;
        }

        int id = v.getId();

        if (id == eventSourceViewId) {
            return true;
        }

        return false;
    }

    private synchronized void startIntro() {

        if (ENABLE_INTRO_SOUND) return;

        if (null == introSound) {
            logger.info("start intro");
            introSound = new SoundEffects(this);
            introSound.start();
        }
    }

    private synchronized void endIntro() {

        if (ENABLE_INTRO_SOUND) return;

        if (null != introSound) {
            logger.info("end intro");
            introSound.stop();
            introSound = null;
        }
    }

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        logger.info("Activity.onCreate()");

        diskManager.bindContext(this.getApplicationContext());

        checkOpenGL();

        checkPermissions();

        setRequestedOrientation(ActivityInfo.SCREEN_ORIENTATION_LANDSCAPE);

        requestWindowFeature(Window.FEATURE_NO_TITLE);
        getWindow().setFlags(WindowManager.LayoutParams.FLAG_FULLSCREEN, WindowManager.LayoutParams.FLAG_FULLSCREEN);

        setContentView(R.layout.activity_fullscreen);

        emuPrefs.load();

        emuView = findViewById(R.id.view_fragment);
        eventSourceViewId = R.id.view_fragment;

        controlsView = findViewById(R.id.controls_fragment);

        emuView.getViewTreeObserver().addOnGlobalLayoutListener(new OnGlobalLayoutListener() {

            @Override
            public void onGlobalLayout() {
                if (0.0f == screenWidth && 0.0f == screenHeight) {
                    updateScreenSize();
                }
            }

        });

        emuView.setOnTouchListener(new View.OnTouchListener() {

            @Override
            public boolean onTouch(View v, MotionEvent e) {

                int action = e.getActionMasked();

                if (MotionEvent.ACTION_DOWN == action || MotionEvent.ACTION_POINTER_DOWN == action) {

                    if (isControlsVisible()) {
                        logger.info("Disabled controls");
                        setControlsVisible(false);
                        return true;
                    }

                    mouseDown = true;

                    mouseX = e.getX();
                    mouseY = e.getY();

                    mouseDownX = mouseX;
                    mouseDownY = mouseY;

                    int area = getTouchArea(mouseX, mouseY);
                    if (0 != (area & AREA_MIDDLE)) {
                        if (isEmuView(v)) {
                            VirtualGamepad.instance().onTouch(v, e);
                        } else {
                            return false; // wrong view
                        }
                    }

                } else if (MotionEvent.ACTION_UP == action || MotionEvent.ACTION_POINTER_UP == action) {

                    if (isControlsVisible()) {
                        logger.info("Disabled controls");
                        setControlsVisible(false);
                        return false;
                    }

                    if (!mouseDown) {
                        return false;
                    }

                    mouseDown = false;
                    mouseX = e.getX();
                    mouseY = e.getY();

                    int area = getTouchArea(mouseX, mouseY);
                    if (0 != (area & AREA_MIDDLE)) {
                        if (isEmuView(v)) {
                            //logger.info("Clicked to emuview");
                            VirtualGamepad.instance().onTouch(v, e);
                        } else {
                            return false; // wrong view
                        }
                    } else {

                        // logger.info("H DISTANCE: " + Math.abs(mouseX - mouseDownX));
                        // logger.info("V DISTANCE: " + Math.abs(mouseY - mouseDownY));

                        if (Math.abs(mouseX - mouseDownX) < (float) COMMAND_CLICK_AREA
                                && Math.abs(mouseY - mouseDownY) < (float) COMMAND_CLICK_AREA) {
                            handleCommandClick(area);
                        }
                    }

                } else if (MotionEvent.ACTION_MOVE == action) {

                    mouseX = e.getX();
                    mouseY = e.getY();

                    if (isControlsVisible()) {
                        return true;
                    }

                    int area = getTouchArea(mouseX, mouseY);
                    if (0 != (area & AREA_MIDDLE)) {
                        if (isEmuView(v)) {
                            VirtualGamepad.instance().onTouch(v, e);
                        } else {
                            return false; // wrong view
                        }
                    }

                } else {
                    return false;
                }

                return true;
            }
        });

        emuView.setOnFocusChangeListener(new View.OnFocusChangeListener() {
            @Override
            public void onFocusChange(View v, boolean hasFocus) {
                //logger.info("focus change: " + v.getId() + " status: " + hasFocus);

                if (isEmuView(v)) {
                    if (hasFocus && isControlsVisible()) {
                        setControlsVisible(false);
                    }
                }
            }
        });

        setControlsVisible(false);

        gameController = new GameController();
        gameController.addListener(new GameControllerListener() {
            @Override
            public void onButtonDown(int buttonId) {

                logger.info("game controller button down: 0x" + Integer.toHexString(buttonId));

                if (buttonId == GameController.ID_BUTTON_SELECT || buttonId == GameController.ID_BUTTON_MENU) {

                    setControlsVisible(!isControlsVisible());

                } else if (buttonId == GameController.ID_BUTTON_START) {

                    selectDisk();

                } else if (buttonId == GameController.ID_BUTTON_PLAY_PAUSE) {

                    if (emuControl.isPaused()) {
                        emuControl.resume();
                    } else {
                        emuControl.pause();
                    }

                } else if (buttonId == GameController.ID_BUTTON_R1) {

                    emuControl.consoleSelect(); // trigger console select event/key

                } else if (buttonId == GameController.ID_BUTTON_L1) {

                    emuControl.consoleReset(); // trigger console reset event/key

                } else if (buttonId == GameController.ID_BUTTON_THUMB_RIGHT) {

                    toggleZoomModes();

                } else if (buttonId == GameController.ID_BUTTON_REWIND) {

                    if (emuPrefs.isWarpEnabled()) {
                        emuPrefs.setWarpEnabled(false);
                    }

                } else if (buttonId == GameController.ID_BUTTON_FAST_FORWARD) {

                    if (emuControl.isPaused()) {
                        emuControl.resume();
                    }

                    emuPrefs.setWarpEnabled(!emuPrefs.isWarpEnabled());

                }
            }

            @Override
            public void onButtonUp(int buttonId) {
                ;
            }

        });

        VirtualGamepad.instance().init(this, emuView);
    }

    @Override
    public boolean dispatchGenericMotionEvent(MotionEvent event) {

        /*
        if (isControlsVisible()) {
            return super.dispatchGenericMotionEvent(event);
        }
        */

        if ((event.getSource() & InputDevice.SOURCE_JOYSTICK) == InputDevice.SOURCE_JOYSTICK && event.getAction() == MotionEvent.ACTION_MOVE) {
            if (!isControlsVisible()) {
                if (gameController.handleEvent(event)) {
                    logger.info("update emu stick");
                    emuControl.setStick(gameController.getState() & 0xff);
                }
                return true;
            }
        }

        return super.dispatchGenericMotionEvent(event);
    }

    private void updateScreenSize() {

        if (null == emuView) {
            return;
        }

        int receivedWidth = emuView.getWidth();
        int receivedHeight = emuView.getHeight();

        if (0 != receivedWidth && 0 != receivedHeight) {

            // check for rotation
            if (receivedWidth > receivedHeight) {
                screenWidth = receivedWidth;
                screenHeight = receivedHeight;
            } else {
                screenWidth = receivedHeight;
                screenHeight = receivedWidth;
            }
        }

    }

    private boolean handleKeyEvent(KeyEvent event) {

        boolean keyUp = event.getAction()== KeyEvent.ACTION_UP;
        boolean keyDown = event.getAction()== KeyEvent.ACTION_DOWN;

        logger.info("KEY: " + event.toString());

        int keyCode = event.getKeyCode();
        char keyChar = (char) event.getUnicodeChar();

        boolean handled = true;

        switch (keyCode) {
            case KeyEvent.KEYCODE_UNKNOWN: {
                handled = false;
                break;
            }
            case KeyEvent.KEYCODE_MENU: {
                if (keyDown) {
                    setControlsVisible(!isControlsVisible());
                }
                break;
            }
            default: {
                handled = false;
                break;
            }
        }

        return handled;

    }

    @Override
    public boolean onKeyDown(int keyCode, KeyEvent event) {

        if (isControlsVisible()) {
            return super.onKeyDown(keyCode, event);
        }

        if (!GameController.isGameControllerEvent(event)) {
            if (keyCode != KeyEvent.KEYCODE_ENTER && handleKeyEvent(event) == true) {
                return true;
            }
        }

        return super.onKeyDown(keyCode, event);
    }

    @Override
    public boolean onKeyUp(int keyCode, KeyEvent event) {

        if (isControlsVisible()) {
            return super.onKeyUp(keyCode, event);
        }

        if (!GameController.isGameControllerEvent(event)) {
            if (keyCode != KeyEvent.KEYCODE_ENTER && handleKeyEvent(event) == true) {
                return true;
            }
        }

        return super.onKeyUp(keyCode, event);
    }

    @Override
    public boolean dispatchKeyEvent(KeyEvent event) {

        if (isControlsVisible()) {

            if (event.getAction() == KeyEvent.ACTION_DOWN || event.getAction() != KeyEvent.ACTION_UP) {
                if (event.getKeyCode() == KeyEvent.KEYCODE_BACK) {
                    if (event.getAction() == KeyEvent.ACTION_DOWN) {
                        setControlsVisible(false);
                    }
                    return true;
                }
            }

            return super.dispatchKeyEvent(event);
        }

        if (null != gameController) {
            if (((event.getSource() & InputDevice.SOURCE_DPAD) == InputDevice.SOURCE_DPAD) ||
                    ((event.getSource() & InputDevice.SOURCE_GAMEPAD) == InputDevice.SOURCE_GAMEPAD)){
                if (event.getRepeatCount() == 0) {
                    if (gameController.handleEvent(event)) {
                        int stickMask = gameController.getState()&0xff;
                        logger.info("update emu stick mask: " + stickMask);
                        emuControl.setStick(stickMask);
                    }
                }
                return true;
            }
        }

        if (event.getAction() != KeyEvent.ACTION_DOWN && event.getAction() != KeyEvent.ACTION_UP) {
            int keyCode = event.getKeyCode();
            if (keyCode == KeyEvent.KEYCODE_ENTER) {
                return super.dispatchKeyEvent(event);
            }
        }

        if (handleKeyEvent(event)) {
            return true;
        }

        return super.dispatchKeyEvent(event);

    }

    private void toggleZoomModes() {

        boolean stretchZoom = emuPrefs.isStretchZoomEnabled();
        boolean maxZoom = emuPrefs.isMaxZoomEnabled();

        if (!maxZoom && !stretchZoom) {
            emuPrefs.setMaxZoomEnabled(true);
        } else if (maxZoom && !stretchZoom) {
            emuPrefs.setMaxZoomEnabled(false);
            emuPrefs.setStretchZoomEnabled(true);
        } else if (!maxZoom && stretchZoom) {
            emuPrefs.setMaxZoomEnabled(false);
            emuPrefs.setStretchZoomEnabled(false);
        }

    }

    private void setControlsVisible(boolean show) {

        logger.info("COMMAND: setControlsVisible(" + show + ")");

        if (EMU_PAUSED_WHILE_CONTROL) {
            if (show) {
                emuControl.pause();
            } else {
                emuControl.resume();
            }
        }

        if (show) {
            emuControl.setStick(0x0); // clear stick input
        }
        
        controlsView.setVisibility(show ? View.VISIBLE : View.GONE);

        if (show) {
            controlsView.requestFocus();
        } else {
            emuView.requestFocus();
        }

        updateScreenSize();

    }

    private boolean isControlsVisible() {
        return (View.VISIBLE == controlsView.getVisibility());
    }

    private int getTouchArea(float x, float y) {

        updateScreenSize();

        if (screenWidth < 1.0f || screenHeight < 1.0f)
            return 0x0;

        float xPercent = (x * 100.0f) / screenWidth;
        float yPercent = (y * 100.0f) / screenHeight;

        int area = 0x0;

        if (yPercent <= 10.0f) {
            area |= AREA_TOP;
        } else if (yPercent >= 20.0f && yPercent <= 80.0f) {
            area |= AREA_MIDDLE;
        } else if (yPercent >= 90.0f) {
            area |= AREA_BOTTOM;
        }

        if (xPercent <= 10.0f) {
            area |= AREA_LEFT;
        } else if (xPercent >= 20.0f && xPercent <= 80.0f) {
            area |= AREA_CENTER;
        } else if (xPercent >= 90.0f) {
            area |= AREA_RIGHT;
        }

        // logger.info("MOUSE: " + (int) x + "/" + (int) screenWidth + "  -  "
        // + (int) y + "/" + (int) screenHeight);
        // displayArea(area);

        return area;
    }

    @SuppressWarnings("unused")
    private void displayArea(int area) {
        if ((area & AREA_TOP) != 0)
            logger.info("AREA_TOP");
        if ((area & AREA_MIDDLE) != 0)
            logger.info("AREA_MIDDLE");
        if ((area & AREA_BOTTOM) != 0)
            logger.info("AREA_BOTTOM");
        if ((area & AREA_LEFT) != 0)
            logger.info("AREA_LEFT");
        if ((area & AREA_CENTER) != 0)
            logger.info("AREA_CENTER");
        if ((area & AREA_RIGHT) != 0)
            logger.info("AREA_RIGHT");
    }

    private void handleCommandClick(int area) {

        logger.info(System.currentTimeMillis() + "Handle command click: 0x" + Long.toHexString(area));

        if (0x0 == area) {
            return;
        }

        if (isControlsVisible() && (AREA_BOTTOM + AREA_RIGHT) != area) {
            logger.info("Disable control panel");
            setControlsVisible(false);
        }

        if ((AREA_BOTTOM + AREA_RIGHT) == area) {
            onVirtualButton(VBUTTON_RIGHT_BOTTOM);
        }
        else if ((AREA_TOP + AREA_RIGHT) == area) {
            onVirtualButton(VBUTTON_RIGHT_TOP);
        }
        else if ((AREA_BOTTOM + AREA_LEFT) == area) {
            onVirtualButton(VBUTTON_LEFT_BOTTOM);
        }
        else if ((AREA_TOP + AREA_LEFT) == area) {
            onVirtualButton(VBUTTON_LEFT_TOP);
        }
        else if ((AREA_TOP + AREA_CENTER) == area) {
            onVirtualButton(VBUTTON_CENTER_TOP);
        }
        else if ((AREA_BOTTOM + AREA_CENTER) == area) {
            onVirtualButton(VBUTTON_CENTER_BOTTOM);
        }
    }

    private void onVirtualButton(int buttonId) {
        switch (buttonId) {
            case VBUTTON_LEFT_TOP: {
                emuControl.consoleReset();
                break;
            }
            case VBUTTON_RIGHT_TOP: {
                logger.info("Select ROM image");
                selectDisk();
                break;
            }
            case VBUTTON_LEFT_BOTTOM: {
                break;
            }
            case VBUTTON_RIGHT_BOTTOM: {
                if (false == isControlsVisible()) {
                    logger.info("Enable control panel");
                    setControlsVisible(true);
                }
                break;
            }
            case VBUTTON_CENTER_TOP: {
                emuControl.consoleSelect();
                break;
            }
            case VBUTTON_CENTER_BOTTOM: {
                //toggleZoomModes();
                break;
            }
            default: {
                break;
            }
        }
    }

    @SuppressWarnings("deprecation")
    private void removeLayoutListenerPre16(OnGlobalLayoutListener listener) {
        emuView.getViewTreeObserver().removeGlobalOnLayoutListener(listener);
    }

    @TargetApi(16)
    private void removeLayoutListenerPost16(OnGlobalLayoutListener listener) {
        emuView.getViewTreeObserver().removeOnGlobalLayoutListener(listener);
    }

    public void onClickReset(View v) {
        logger.info("command: reset");
        setControlsVisible(false);
        //emuControl.softReset();
        emuControl.hardReset(null);
    }

    public void onClickShowSettings(View v) {
        setControlsVisible(false);
        if (null == settingsDialog) {
            settingsDialog = new SettingsDialog();
        }
        settingsDialog.show(getFragmentManager(), "emu_settings_dialog");
    }

    public void onClickSelectDisk(View v) {
        setControlsVisible(false);
        selectDisk();
    }

    private synchronized void selectDisk() {

        if (null == fileDialog) {
            fileDialog = new FileDialog();
        }

        if (!fileDialog.isVisible() && !fileDialog.isAdded()) {
            fileDialog.show(getFragmentManager(), "emu_file_dialog");
        } else {
            logger.info("file dialog fragment already added and visible!");
        }
    }

    @Override
    public void onDiskSelect(Image image, ImageFilter filter, boolean longClick) {

        Emu emuControl = Emu.instance();

        endIntro();

        emuControl.hardReset(image);
        //boolean status = emuControl.attachImage(image); // just insert disk

        gameController.init();

        if (image.getType() == Image.TYPE_SNAPSHOT) {
            setControlsVisible(false);
            Toast.makeText(getApplicationContext(), "Restore snapshot: " + image.getName(), Toast.LENGTH_SHORT).show();
            return;
        }

        setControlsVisible(false);

        Toast.makeText(getApplicationContext(), "Selected image: " + image.getName(), Toast.LENGTH_SHORT).show();
    }

    public void onClickExitApp(View v) {
        finish();
    }

    public void onClickPauseResume(View v) {
        if (!emuControl.isPaused()) {
            emuControl.pause();
        } else {
            emuControl.resume();
        }
    }

    private void checkOpenGL() {
        ActivityManager am = (ActivityManager) getSystemService(Context.ACTIVITY_SERVICE);
        ConfigurationInfo info = am.getDeviceConfigurationInfo();
        logger.info("GLES version: 0x" + Integer.toHexString(info.reqGlEsVersion)); // >= 0x20000;
    }

    @Override
    protected void onStart() {
        super.onStart();
        logger.info("Activity.onStart()");
        startIntro();
        emuControl.start();
        if (isControlsVisible()) {
            emuControl.pause();
        }
    }

    @Override
    protected void onPause() {
        super.onPause();
        logger.info("Activity.onPause()");
        emuControl.pause();
    }

    @Override
    protected void onResume() {
        super.onResume();
        logger.info("Activity.onResume()");
        if (isControlsVisible()) {
            emuControl.pause();
        } else {
            emuControl.resume();
        }
    }

    @Override
    protected void onRestart() {
        logger.info("Activity.onRestart()");
        emuControl.stop();
        emuControl.start();
        if (isControlsVisible()) {
            emuControl.pause();
        }
        super.onRestart();
    }

    @Override
    protected void onStop() {
        logger.info("Activity.onStop()");
        endIntro();
        emuControl.stop();
        super.onStop();
    }

    @Override
    protected void onDestroy() {
        logger.info("Activity.onDestroy()");
        endIntro();
        if (null != emuControl) {
            emuControl.stop();
            emuControl = null;
        }

        super.onDestroy();
    }

    public void onClickStoreSnapshot(View v) {
        logger.info("command: store snapshot");
        setControlsVisible(false);
        emuControl.storeSnapshot();
    }

    public void onClickRestoreSnapshot(View v) {
        logger.info("command: restore snapshot");
        setControlsVisible(false);
        emuControl.restoreSnapshot();
    }

    @Override
    public void onFragmentInteraction(Uri uri) {

    }
}

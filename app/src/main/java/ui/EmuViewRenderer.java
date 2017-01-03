package ui;

import android.content.Context;
import android.graphics.Rect;

import emu.Emu;
import system.Preferences;
import system.Statistics;
import util.LogManager;
import util.Logger;
import util.Util;

import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.text.DecimalFormat;
import java.util.Random;

import javax.microedition.khronos.opengles.GL10;

/**
 * Created by roland on 17.08.2016.
 */

public class EmuViewRenderer extends Renderer {

    private final static Logger logger = LogManager.getLogger(EmuViewRenderer.class.getName());

    private Rect renderRect = new Rect(0, 0, 0, 0);

    private boolean packedTextures = false;
    private Texture texture;
    private ByteBuffer textureUpdateBuffer = null;
    private int textureUpdateBufferSize = 0;

    private Texture markerTexture;

    private Texture lightTexture;
    private double circleAngle = 0.0;
    private double angleSpeed = 0.0;
    private double angleSpeedMin = 60.0;
    private double angleSpeedMax = 240.0;

    private Texture titleTexture;
    private Texture titleLightTexture;

    private DecimalFormat statisticsOutputFormat = new DecimalFormat("0.0");

    public EmuViewRenderer(Context context) {
        super(context);
    }

    @Override
    protected void init() {
        packedTextures = Preferences.instance().isTextureCompressionEnabled();
    }

    @Override
    protected void resize(int width, int height) {
    }

    @Override
    protected void loadResources() {
        markerTexture = loadTexture("button.png");
        lightTexture = loadTexture("light.png");
        titleTexture = loadTexture("title.png");
        titleLightTexture = loadTexture("title_light.png");
    }

    private void updateTexture() {

        byte[] rawData = Emu.instance().lockTextureData();
        if (null == rawData) {
            return;
        }

        if (textureUpdateBuffer == null || textureUpdateBufferSize != rawData.length) {
            textureUpdateBuffer = ByteBuffer.allocate(rawData.length);
            textureUpdateBufferSize = rawData.length;
        }

        textureUpdateBuffer.put(rawData);
        textureUpdateBuffer.position(0);

        boolean newTexture = false;

        if (null == texture) {
            int handle = allocTexture();
            texture = new Texture(handle, Emu.DISPLAY_X, Emu.DISPLAY_Y);
            newTexture = true;
        }

        gl.glBindTexture(GL10.GL_TEXTURE_2D, texture.getHandle());

        gl.glTexParameterf(GL10.GL_TEXTURE_2D, GL10.GL_TEXTURE_WRAP_S, GL10.GL_CLAMP_TO_EDGE);
        gl.glTexParameterf(GL10.GL_TEXTURE_2D, GL10.GL_TEXTURE_WRAP_T, GL10.GL_CLAMP_TO_EDGE);

        gl.glPixelStorei(GL10.GL_UNPACK_ALIGNMENT, 1);
        gl.glPixelStorei(GL10.GL_PACK_ALIGNMENT, 1);


        if (!packedTextures) {

            int internalFormat = GL10.GL_RGBA;
            int pixelFormat = GL10.GL_RGBA;

            if (newTexture) {
                gl.glTexImage2D(GL10.GL_TEXTURE_2D, 0, internalFormat, texture.getWidth(), texture.getHeight(), 0, pixelFormat, GL10.GL_UNSIGNED_BYTE, textureUpdateBuffer);
            } else {
                gl.glTexSubImage2D(GL10.GL_TEXTURE_2D, 0, 0, 0, texture.getWidth(), texture.getHeight(), pixelFormat, GL10.GL_UNSIGNED_BYTE, textureUpdateBuffer);
            }

        } else {

            // 4-bit indexed texture (16 color palette)

            /*

            int bufferSize = buffer.capacity();

            int internalFormat = GL10.GL_PALETTE8_RGB8_OES;
            int pixelFormat = 0;

            if (newTexture) {

                gl.glCompressedTexImage2D(GL10.GL_TEXTURE_2D, 0, internalFormat, textureWidth, textureHeight, 0, bufferSize, buffer);
            } else {
                gl.glCompressedTexSubImage2D(GL10.GL_TEXTURE_2D, 0, 0, 0, textureWidth, textureHeight, 0, bufferSize, buffer);
            }

            */

        }

        gl.glBindTexture(GL10.GL_TEXTURE_2D, 0);

        Emu.instance().unlockTextureData();

    }

    @Override
    protected void draw(float elapsed) {

        Emu emuControl = Emu.instance();
        if (null == emuControl) {
            return;
        }

        boolean active = emuControl.isActive();

        if (active) {
            drawScreen(elapsed);
        } else {
            drawScreenSaver(elapsed);
        }

    }

    private void drawScreenSaver(float elapsed) {
        drawScreenSaverTitle(elapsed);
        drawScreenSaverSpin(elapsed);
    }

    private Color spinColor = new Color(1.0f, 1.0f, 1.0f, 1.0f);

    private void drawScreenSaverSpin(float elapsed) {

        if (null == lightTexture ||
            null == titleTexture ||
            null == titleLightTexture) return;

        int canvasWidth = getWidth();
        int canvasHeight = getHeight();
        int sz = Math.min(canvasWidth / 6, canvasHeight / 6);

        double radius = Math.min(canvasWidth-sz/2, canvasHeight-sz/2) * 0.20;

        circleAngle += elapsed * angleSpeed;
        if (circleAngle > 360.0) circleAngle -= 360.0;

        double speedAcc = Math.abs(Math.sin(circleAngle*3.1415/180.0));

        if (circleAngle < 180.0) {
            angleSpeed += speedAcc * elapsed * 120.0;
        } else {
            angleSpeed -= speedAcc * elapsed * 120.0;
        }

        if (angleSpeed > angleSpeedMax) angleSpeed = angleSpeedMax;
        if (angleSpeed < angleSpeedMin) angleSpeed = angleSpeedMin;

        enableAlphaBlending();
        setBlendMode(Renderer.BLEND_MODE_ADDITIVE);

        float fade = 1.0f;
        double angle = circleAngle + 270.0;

        while (fade > 0.0f) {

            spinColor.set(1.0f-fade, fade*0.7f, 0.5f+fade/2.0f, fade);

            int x = canvasWidth / 2 + (int) (Math.cos(angle * 3.1415 / 180.0) * radius);
            int y = canvasHeight / 2 + (int) (Math.sin(angle * 3.1415 / 180.0) * radius);

            Quad.draw(gl,
                    lightTexture,
                    x - sz / 2.0f, y - sz / 2.0f, sz, sz,
                    spinColor,
                    true);

            fade -= 0.15f;
            angle -= angleSpeed * 0.15;
        }

        setBlendMode(Renderer.BLEND_MODE_NORMAL);
    }

    private double titleAngle = 0.0;
    private double zoomAngle = 0.0;
    private Color titleColor = new Color(1.0f, 1.0f, 1.0f, 0.3f);
    private Color titleLightColor = new Color(1.0f, 1.0f, 1.0f, 1.0f);

    private void drawScreenSaverTitle(float elapsed) {

        if (null == titleTexture || null == titleLightTexture) return;

        int canvasWidth = getWidth();
        int canvasHeight = getHeight();

        int tw = titleTexture.getWidth();
        int th = titleTexture.getHeight();

        int sz = titleTexture.getWidth();
        if (sz > canvasWidth / 3) sz = canvasWidth / 3;

        float zoom = (float) sz / (float) titleTexture.getWidth();
        float zoomOfsFactor = 1.0f + (float) (Math.cos(zoomAngle * 3.1415 / 180.0) * 0.02);
        zoomAngle += elapsed * 30.0;

        float w = (float) tw * zoom * zoomOfsFactor;
        float h = (float) th * zoom * zoomOfsFactor;

        float x = ((float) canvasWidth - w) / 2.0f;
        float y = ((float) canvasHeight - h) / 2.0f;

        double angle = titleAngle;
        titleAngle += elapsed * 60.0;

        enableAlphaBlending();
        setBlendMode(Renderer.BLEND_MODE_NORMAL);
        {
            Quad.draw(gl, titleTexture, x, y, w, h, titleColor, true);
        }
        setBlendMode(Renderer.BLEND_MODE_ADDITIVE);
        {
            float fade = 1.0f;

            while (fade > 0.0f) {

                titleLightColor.set(1.0f-fade, fade*0.7f, 0.5f+fade/2.0f, 0.5f + fade*0.5f);

                double radius = (1.0f - fade) * 8.0f;
                double angleOfs = - fade * 24.0f;

                int xofs = (int) (Math.cos((angle+angleOfs) * 3.1415 / 180.0) * radius);
                int yofs = (int) (Math.sin((angle+angleOfs) * 3.1415 / 180.0) * radius);

                Quad.draw(gl, titleLightTexture, x-xofs, y-yofs, w, h, titleLightColor, true);

                fade -= 0.25f;
            }

        }
        setBlendMode(Renderer.BLEND_MODE_NORMAL);
    }

    private Rect renderSrcRect = new Rect(0, 0, 0, 0);

    private void drawScreen(float elapsed) {

        disableAlphaBlending();
        updateTexture();

        Emu emuControl = Emu.instance();

        boolean textureFiltering = Preferences.instance().isAntialiasingEnabled();
        boolean stretchZoom = Preferences.instance().isStretchZoomEnabled();

        if (stretchZoom) {

            // scale to screen size
            renderRect.set(0, 0, getWidth(), getHeight());

        } else {

            // get emulator rendering info
            int contentWidth = emuControl.getDisplayWidth()*2;
            int contentHeight = emuControl.getDisplayHeight();

            double zoomFactorX = (double) getWidth() / (double) contentWidth;
            double zoomFactorY = (double) getHeight() / (double) contentHeight;

            boolean maxZoom = Preferences.instance().isMaxZoomEnabled();
            double zoomFactor = maxZoom ? Math.max(zoomFactorX, zoomFactorY) : Math.min(zoomFactorX, zoomFactorY);

            int w = (int) ((double) contentWidth * zoomFactor);
            int x = (getWidth() - w) / 2;

            int h = (int) ((double) contentHeight * zoomFactor);
            int y = (getHeight() - h) / 2;

            renderRect.set(x, y, x+w, y+h);
        }

        renderSrcRect. set(0, 0, emuControl.getDisplayWidth(), emuControl.getDisplayHeight());

        Quad.draw(gl, texture,
                renderRect,
                renderSrcRect,
                Color.WHITE,
                textureFiltering);
    }

    @Override
    protected void drawOverlay(float elapsed) {

        if (null == markerTexture) return;

        enableAlphaBlending();
        VirtualGamepad gamepad = VirtualGamepad.instance();

        float visibility = gamepad.getVisibility();
        if (visibility == 0.0f) {
            return;
        }

        Color c = new Color(1.0f, 1.0f, 1.0f, visibility);

        float x = gamepad.getStickX();
        float y = gamepad.getStickY();

        float w = gamepad.getStickXGap() / 2.0f;
        float h = gamepad.getStickYGap() / 2.0f;

        setBlendMode(Renderer.BLEND_MODE_ADDITIVE);

        Quad.draw(gl,
                markerTexture,
                x-w/2.0f, y-h/2.0f, w, h,
                c,
                true);

        setBlendMode(Renderer.BLEND_MODE_NORMAL);
    }

    @Override
    protected void outputStatistics(Statistics statistics) {

        double emuUpdatePerSec = Emu.instance().getUpdatesPerSecond();
        double renderUpdatePerSec = statistics.getUpdatesPerSecond();

        String text = "Statistics: " + statisticsOutputFormat.format(emuUpdatePerSec) + " ups / " + statisticsOutputFormat.format(renderUpdatePerSec) + " fps";

        logger.info(text);

    }

}

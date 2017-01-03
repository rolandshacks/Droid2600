package ui;

import android.graphics.Rect;

import javax.microedition.khronos.opengles.GL10;

/**
 * Created by roland on 18.08.2016.
 */

public class Quad
{

    private static final Vertex QUAD_COORDS = new Vertex (new float[]
        {
            1f,1f,0f,
            0f,1f,0f,
            1f,0f,0f,
            0f,0f,0f,
        });

    private static final Vertex TEXTURE_COORDS = new Vertex (new float[]
        {
            1.0f, 1.0f,
            0.0f, 1.0f,
            1.0f, 0.0f,
            0.0f, 0.0f,
        });

    private static Vertex textureCoordsTempVertex = new Vertex();
    private static float[] textureCoordsTempBuffer = Vertex.getRect(0.0f, 0.0f, 0.0f, 0.0f);

    public static void draw (GL10 gl, Texture texture, Rect outputRect, Rect srcRect, Color color, boolean textureFiltering) {

        if (null == texture || !texture.isValid()) return;

        int w = texture.getWidth();
        int h = texture.getHeight();

        float x1 = ((float) srcRect.left / (float) w);
        float y1 = ((float) srcRect.top / (float) h);

        float x2 = ((float) srcRect.right / (float) w);
        float y2 = ((float) srcRect.bottom / (float) h);

        Vertex.setRect(textureCoordsTempBuffer, x1, y1, x2, y2);
        textureCoordsTempVertex.set(textureCoordsTempBuffer);

        draw(gl, texture,
                outputRect.left, outputRect.top, outputRect.width(), outputRect.height(),
                textureCoordsTempVertex, color, textureFiltering);
    }

    public static void draw (GL10 gl, Texture texture, float x, float y, float width, float height, Color color, boolean textureFiltering) {
        draw(gl, texture, x, y, width, height, TEXTURE_COORDS, color, textureFiltering);
    }

    public static void draw (GL10 gl, Texture texture, float x, float y, float width, float height, Vertex textureCoords, Color color, boolean textureFiltering)
    {
        if (null == texture || !texture.isValid()) return;

        gl.glPushMatrix();

        gl.glTranslatef (x, y, 0.0f); //MOVE !!! 1f is size of figure if called after scaling, 1f is pixel if called before scaling
        gl.glScalef (width, height, 0.0f); // ADJUST SIZE !!!

        // bind the previously generated texture
        gl.glBindTexture(GL10.GL_TEXTURE_2D, texture.getHandle());

        // Point to our buffers
        gl.glEnableClientState(GL10.GL_VERTEX_ARRAY);
        gl.glEnableClientState(GL10.GL_TEXTURE_COORD_ARRAY);

        // set the colour for the square
        gl.glColor4f(color.getRed(), color.getGreen(), color.getBlue(), color.getAlpha());

        if (textureFiltering) {
            gl.glTexParameterf(GL10.GL_TEXTURE_2D, GL10.GL_TEXTURE_MAG_FILTER, GL10.GL_LINEAR);
            gl.glTexParameterf(GL10.GL_TEXTURE_2D, GL10.GL_TEXTURE_MIN_FILTER, GL10.GL_LINEAR);
        } else {
            gl.glTexParameterf(GL10.GL_TEXTURE_2D, GL10.GL_TEXTURE_MAG_FILTER, GL10.GL_NEAREST);
            gl.glTexParameterf(GL10.GL_TEXTURE_2D, GL10.GL_TEXTURE_MIN_FILTER, GL10.GL_NEAREST);
        }

        // Point to our vertex buffer
        gl.glVertexPointer (3, GL10.GL_FLOAT, 0, QUAD_COORDS.get());
        gl.glTexCoordPointer(2, GL10.GL_FLOAT, 0, textureCoords.get());

        // Draw the vertices as triangle strip
        gl.glDrawArrays (GL10.GL_TRIANGLE_STRIP, 0, QUAD_COORDS.length() / 3);

        // Disable the client state before leaving
        gl.glDisableClientState (GL10.GL_VERTEX_ARRAY);
        gl.glDisableClientState(GL10.GL_TEXTURE_COORD_ARRAY);

        gl.glBindTexture(GL10.GL_TEXTURE_2D, 0);

        gl.glPopMatrix();
    }

}

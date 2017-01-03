package ui;

import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.nio.FloatBuffer;

/**
 * Created by roland on 18.08.2016.
 */

public class Vertex
{
    private int bufferLength;
    private FloatBuffer buffer;

    public Vertex() {
        ;
    }

    public Vertex (float[] vertex) {
        set(vertex);
    }

    public void set(float[] vertex) {

        if (null == buffer || bufferLength != vertex.length) {
            ByteBuffer factory = ByteBuffer.allocateDirect(vertex.length * 4); // float size
            factory.order(ByteOrder.nativeOrder());
            buffer = factory.asFloatBuffer();
        } else {
            buffer.position(0);
        }

        buffer.put(vertex);
        buffer.position(0);

        bufferLength = vertex.length;
    }

    public FloatBuffer get() {
        return buffer;
    }

    public int length() {
        return bufferLength;
    }

    public void set(float x, float y, float width, float height) {

        float[] vertex = getRect(x, y, width, height);
        set(vertex);

    }

    public static void setRect(float[] vertex, float x, float y, float width, float height) {

        float x2 = x + width;
        float y2 = y + height;

        vertex[2] = vertex[6] = x;
        vertex[0] = vertex[4] = x2;
        vertex[5] = vertex[7] = y;
        vertex[1] = vertex[3] = y2;
    }

    public static float[] getRect(float x, float y, float width, float height) {

        float x2 = x + width;
        float y2 = y + height;

        float[] vertex = new float[]
                {
                        x2, y2,
                        x, y2,
                        x2, y,
                        x, y,
                };

        return vertex;

    }
}

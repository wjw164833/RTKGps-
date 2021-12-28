package gpsplus.rtkgps;

import android.content.Context;
import android.location.GnssClock;
import android.location.GnssMeasurement;
import android.location.GnssMeasurementsEvent;
import android.location.GpsSatellite;
import android.location.GpsStatus;
import android.location.Location;
import android.location.LocationListener;
import android.location.LocationManager;
import android.location.OnNmeaMessageListener;
import android.os.Bundle;
import android.util.Log;

import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.nio.ByteBuffer;
import java.nio.FloatBuffer;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collection;
import java.util.Iterator;
import java.util.List;

import javax.annotation.Nonnull;

import gpsplus.rtkgps.settings.StreamInternalFragment.Value;

@SuppressWarnings("ALL")
public class InternalReceiverToRtklib implements GpsStatus.Listener, LocationListener {

    private static final boolean DBG = BuildConfig.DEBUG & true;
    static final String TAG = InternalReceiverToRtklib.class.getSimpleName();

    final LocalSocketThread mLocalSocketThread;
    private static Context mParentContext = null;
    private Value mInternalReceiverSettings;
    LocationManager locationManager = null;
    FileOutputStream autoCaptureFileOutputStream = null;
    File autoCaptureFile = null;
    private int nbSat = 0;
    private boolean isStarting = false;
    private String mSessionCode;
    private int rawMeasurementStatus;
    List<byte[]> nmealist = new ArrayList<byte[]>();
    public InternalReceiverToRtklib(Context parentContext, @Nonnull Value internalReceiverSettings, String sessionCode) {
        InternalReceiverToRtklib.mParentContext = parentContext;
        mSessionCode = sessionCode;
        this.mInternalReceiverSettings = internalReceiverSettings;
        mLocalSocketThread = new LocalSocketThread(mInternalReceiverSettings.getPath());
        mLocalSocketThread.setBindpoint(mInternalReceiverSettings.getPath());
    }

    public void start() {
        isStarting = true;
        locationManager = (LocationManager) mParentContext.getSystemService(Context.LOCATION_SERVICE);
        locationManager.addGpsStatusListener(this);
        locationManager.requestLocationUpdates(
                LocationManager.GPS_PROVIDER, 1000, 1f, this);
        locationManager.addNmeaListener(nmealistener);
        locationManager.registerGnssMeasurementsCallback(mRawMeasurementListener);

    }

    public void stop() {
        locationManager.removeUpdates(this);
        locationManager.unregisterGnssMeasurementsCallback(mRawMeasurementListener);
        locationManager.removeNmeaListener(nmealistener);
        mLocalSocketThread.cancel();
    }
    OnNmeaMessageListener nmealistener=new OnNmeaMessageListener() {
        @Override
        public void onNmeaMessage(String message, long timestamp) {
            Log.d("NMEA",message);
            nmealist.add(message.getBytes());
        }
    };

    public boolean isRawMeasurementsSupported() {
        return (rawMeasurementStatus == GnssMeasurementsEvent.Callback.STATUS_READY);
    }

    private String isNaN(double number) {
        return Double.isNaN(number) ? "" : number + "";
    }

    private final GnssMeasurementsEvent.Callback mRawMeasurementListener =
            new GnssMeasurementsEvent.Callback() {
                @Override
                public void onStatusChanged(int status) {
                    Log.e("resuslt", "onStatusChanged===============>");
                    rawMeasurementStatus = status;
                }

                @Override
                public void onGnssMeasurementsReceived(GnssMeasurementsEvent event) {
                    Log.e("resuslt", "onGnssMeasurementsReceived===============>");
                    if (isStarting) // run only if starting
                    {
                        Log.i(TAG, "Starting streaming from internal receiver");
                        mLocalSocketThread.start();
                        isStarting = false;
                    }
                    List<byte[]> list = new ArrayList<byte[]>();
                    GnssClock c = event.getClock();
                    Collection<GnssMeasurement> measurements = event.getMeasurements();

                    byte[] headMessage = new byte[6];
                    byte[] check=new byte[2];
                    headMessage[0] = (byte) (0xB5);
                    headMessage[1] = (byte) (0x62);//同步头2bit
                    headMessage[2] = (byte) (0x02);//消息类别1bit
                    headMessage[3] = (byte) (0x15);//子消息长度1bit


                    //Parcel p = Parcel.obtain();
//                    Parcel p1 = Parcel.obtain();

                    list.add(headMessage);
                    String p = "";
                    String p2 = "";

                    try {
                        //byte[] syncWord = {0x00, 0x00};
                        //p.writeByteArray(syncWord);

                        list.add(doubleToByte(c.getBiasNanos()));
                        list.add(doubleToByte(c.getBiasUncertaintyNanos()));
//                        list.add(doubleToByte(c.getDriftNanosPerSecond()));
//                        list.add(doubleToByte(c.getDriftUncertaintyNanosPerSecond()));
                        list.add(longToByte(c.getFullBiasNanos()));
//                        list.add(int2Byte(c.getHardwareClockDiscontinuityCount()));
//                        list.add(int2Byte(c.getLeapSecond()));
                        list.add(longToByte(c.getTimeNanos()));
                        list.add(doubleToByte(c.getTimeUncertaintyNanos()));
                        list.add(int2Byte(c.hasBiasNanos() ? 1 : 0));
//                        list.add(int2Byte(c.hasBiasUncertaintyNanos() ? 1 : 0));
//                        list.add(int2Byte(c.hasDriftNanosPerSecond() ? 1 : 0));
//                        list.add(int2Byte(c.hasDriftUncertaintyNanosPerSecond() ? 1 : 0));
                        list.add(int2Byte(c.hasFullBiasNanos() ? 1 : 0));
//                        list.add(int2Byte(c.hasLeapSecond() ? 1 : 0));
//                        list.add(int2Byte(c.hasTimeUncertaintyNanos() ? 1 : 0));
                        list.add(int2Byte(measurements.size()));

                        for (GnssMeasurement m : measurements) {
                            list.add(doubleToByte(m.getAccumulatedDeltaRangeMeters()));
                            list.add(int2Byte(m.getAccumulatedDeltaRangeState()));
                            int nnn=m.getAccumulatedDeltaRangeState();
//                            list.add(doubleToByte(m.getAccumulatedDeltaRangeUncertaintyMeters()));
//                            list.add(doubleToByte(m.getAutomaticGainControlLevelDb()));
                            list.add(longToByte(m.getCarrierCycles()));
                            list.add(float2Byte(m.getCarrierFrequencyHz()));
//                            list.add(doubleToByte(m.getCarrierPhase()));
//                            list.add(doubleToByte(m.getCarrierPhaseUncertainty()));
                            list.add(doubleToByte(m.getCn0DbHz()));
                            list.add(int2Byte(m.getConstellationType()));
//                            list.add(int2Byte(m.getMultipathIndicator()));
                            list.add(doubleToByte(m.getPseudorangeRateMetersPerSecond()));
                            list.add(doubleToByte(m.getPseudorangeRateUncertaintyMetersPerSecond()));
                            list.add(longToByte(m.getReceivedSvTimeNanos()));
                            list.add(longToByte(m.getReceivedSvTimeUncertaintyNanos()));
//                            list.add(doubleToByte(m.getSnrInDb()));
                            list.add(int2Byte(m.getState()));
                            list.add(int2Byte(m.getSvid()));
                            list.add(doubleToByte(m.getTimeOffsetNanos()));
//                            list.add(int2Byte(m.hasAutomaticGainControlLevelDb() ? 1 : 0));
//                            list.add(int2Byte(m.hasCarrierCycles() ? 1 : 0));
//                            list.add(int2Byte(m.hasCarrierFrequencyHz() ? 1 : 0));
//                            list.add(int2Byte(m.hasCarrierPhase() ? 1 : 0));
//                            list.add(int2Byte(m.hasCarrierPhaseUncertainty() ? 1 : 0));
//                            list.add(int2Byte(m.hasSnrInDb() ? 1 : 0));

                        }

                        // p+='\n';
                        list.add(check);
                        byte[] resultBytes = new byte[0];
                        int index = 0;
                        for (byte[] bytes : list) {

                            resultBytes = Arrays.copyOf(resultBytes, resultBytes.length + bytes.length);
                            System.arraycopy(bytes, 0, resultBytes, index, bytes.length);
                            index += bytes.length;
                        }

                        resultBytes[4] = (byte) ((resultBytes.length - 8) & 0x00ff);
                        resultBytes[5] = (byte) ((resultBytes.length - 8) >> 8 & 0xff);//消息长度2bit

                        int cka=0,ckb=0;
                        for (int i=2;i< resultBytes.length-2;i++) {
                            cka+=resultBytes[i]; ckb+=cka;
                        }
                        resultBytes[resultBytes.length-2]=(byte)(cka& 0xff);
                        resultBytes[resultBytes.length-1]=(byte)(ckb& 0xff);
                        //byte[] packet = p.marshall();
                        //p1.writeString(p.toString());
                        //byte[] packet = p1.marshall();
                        byte[] nmeaBytes = new byte[0];
                        int nmeaindex = 0;
                        for (byte[] nmeabytes : nmealist) {

                            nmeaBytes = Arrays.copyOf(nmeaBytes, nmeaBytes.length + nmeabytes.length);
                            System.arraycopy(nmeabytes, 0, nmeaBytes, nmeaindex, nmeabytes.length);
                            nmeaindex += nmeabytes.length;
                        }

                        byte[] result=byteMerger(resultBytes,nmeaBytes);

                        //FileUtils.dumpToSDCard(RtkGps.getInstance().getGnssPath(), "GNSS_logger", p2);
                        //FileUtils.dumpToSDCard(RtkGps.getInstance().getGnssPath(), "GNSS",resultBytes.toString());
                        mLocalSocketThread.write(result, 0, result.length);
                        nmealist.clear();
                    } catch (IOException e) {
                        e.printStackTrace();
                    } finally {
                        //p.recycle();
                        //p1.recycle();
                    }
                }

            };
    public static byte[] byteMerger(byte[] byte_1, byte[] byte_2){
        byte[] byte_3 = new byte[byte_1.length+byte_2.length];
        System.arraycopy(byte_1, 0, byte_3, 0, byte_1.length);
        System.arraycopy(byte_2, 0, byte_3, byte_1.length, byte_2.length);
        return byte_3;
    }

    public static String bytes2HexString(byte[] b) {
        String ret = "";
        for (int i = 0; i < b.length; i++) {
            String hex = Integer.toHexString(b[i] & 0xFF);
            if (hex.length() == 1) {
                hex = '0' + hex;
            }
            ret += hex.toUpperCase();
        }
        return ret;
    }


    public static String bytesToHex(byte[] bytes) {
        StringBuffer sb = new StringBuffer();
        for (int i = 0; i < bytes.length; i++) {
            String hex = Integer.toHexString(bytes[i] & 0xFF);
            if (hex.length() < 2) {
                sb.append(0);
            }
            sb.append(hex);
        }
        return sb.toString();
    }

    public static byte[] int2Byte(int l) {
        byte[] b = new byte[4];
        for (int i = 0; i < b.length; i++) {
            b[i] = new Integer(l).byteValue();
            l = l >> 8;
        }
        return b;
    }

    public static int byte2Int(byte[] b) {
        int l = 0;
        l = b[0];
        l &= 0xff;
        l |= ((int) b[1] << 8);
        l &= 0xffff;
        l |= ((int) b[2] << 16);
        l &= 0xffffff;
        l |= ((int) b[3] << 24);
        l &= 0xffffffff;
        return l;
    }

    public static byte[] longToByte(long l) {
        byte[] b = new byte[8];
        for (int i = 0; i < b.length; i++) {
            b[i] = new Long(l).byteValue();
            l = l >> 8;
        }
        return b;
    }

    public static long byteToLong(byte[] b) {
        long l = 0;
        l |= (((long) b[7] & 0xff) << 56);
        l |= (((long) b[6] & 0xff) << 48);
        l |= (((long) b[5] & 0xff) << 40);
        l |= (((long) b[4] & 0xff) << 32);
        l |= (((long) b[3] & 0xff) << 24);
        l |= (((long) b[2] & 0xff) << 16);
        l |= (((long) b[1] & 0xff) << 8);
        l |= ((long) b[0] & 0xff);
        return l;
    }

    public static byte[] float2Byte(float f) {
        byte[] b = new byte[4];
        int l = Float.floatToIntBits(f);
        for (int i = 0; i < b.length; i++) {
            b[i] = new Integer(l).byteValue();
            l = l >> 8;
        }
        return b;
    }

    public static float byte2Float(byte[] b) {
        int l = 0;
        l = b[0];
        l &= 0xff;
        l |= ((int) b[1] << 8);
        l &= 0xffff;
        l |= ((int) b[2] << 16);
        l &= 0xffffff;
        l |= ((int) b[3] << 24);
        l &= 0xffffffffl;
        return Float.intBitsToFloat(l);
    }

    public static byte[] doubleToByte(double d) {
        byte[] b = new byte[8];
        long l = Double.doubleToLongBits(d);
        for (int i = 0; i < b.length; i++) {
            b[i] = new Long(l).byteValue();
            l = l >> 8;
        }
        return b;
    }


    public static char[] bytesToChars(byte[] bytes, int offset, int count) {
        char chars[] = new char[count];
        for (int i = 0; i < count; i++) {
            chars[i] = (char) bytes[i];
        }
        return chars;
    }

    public static byte[] charsToBytes(char[] chars, int offset, int count) {
        byte bytes[] = new byte[count];
        for (int i = 0; i < count; i++) {
            bytes[i] = (byte) chars[i];
        }
        return bytes;
    }

    public static byte[] floatToByte(float v) {
        ByteBuffer bb = ByteBuffer.allocate(4);
        byte[] ret = new byte[4];
        FloatBuffer fb = bb.asFloatBuffer();
        fb.put(v);
        bb.get(ret);
        return ret;
    }

    public static float byteToFloat(byte[] v) {
        ByteBuffer bb = ByteBuffer.wrap(v);
        FloatBuffer fb = bb.asFloatBuffer();
        return fb.get();
    }
    /**
     * 16进制bety[]转换String字符串.方法一
     *
     * @param data
     * @return String 返回字符串无空格
     */
    public static String bytesToString(byte[] data) {
        char[] hexArray = "0123456789ABCDEF".toCharArray();
        char[] hexChars = new char[data.length * 2];
        for (int j = 0; j < data.length; j++) {
            int v = data[j] & 0xFF;
            hexChars[j * 2] = hexArray[v >>> 4];
            hexChars[j * 2 + 1] = hexArray[v & 0x0F];
        }
        String result = new String(hexChars);
        return result;
    }

    @Override
    public void onGpsStatusChanged(int i) {
        GpsStatus gpsStatus = locationManager.getGpsStatus(null);
        if (DBG) {
            Log.d(TAG, "GPS Status changed");
        }
        Log.e("gpsStatus", gpsStatus + "");
        if (gpsStatus != null) {

            Iterable<GpsSatellite> satellites = gpsStatus.getSatellites();
            Iterator<GpsSatellite> sat = satellites.iterator();
            nbSat = 0;
            while (sat.hasNext()) {
                Log.e("gpsStatus", ".........1");
                GpsSatellite satellite = sat.next();
                if (satellite.usedInFix()) {
                    Log.e("gpsStatus", ".........2");
                    nbSat++;
                    Log.d(TAG, "PRN:" + satellite.getPrn() + ", SNR:" + satellite.getSnr() + ", AZI:" + satellite.getAzimuth() + ", ELE:" + satellite.getElevation());
                }
            }
        }
    }

    @Override
    public void onLocationChanged(Location location) {

    }

    @Override
    public void onStatusChanged(String s, int i, Bundle bundle) {

    }

    @Override
    public void onProviderEnabled(String s) {

    }

    @Override
    public void onProviderDisabled(String s) {

    }

    private final class LocalSocketThread extends RtklibLocalSocketThread {

        public LocalSocketThread(String socketPath) {
            super(socketPath);
        }

        @Override
        protected boolean isDeviceReady() {
            return isRawMeasurementsSupported();
        }

        @Override
        protected void waitDevice() {

        }

        @Override
        protected boolean onDataReceived(byte[] buffer, int offset, int count) {
            /*if (count <= 0) return true;
                   PoGoPin.writeDevice(BytesTool.get(buffer,offset), count);
                   */
            return true;
        }

        @Override
        protected void onLocalSocketConnected() {

        }
    }
}
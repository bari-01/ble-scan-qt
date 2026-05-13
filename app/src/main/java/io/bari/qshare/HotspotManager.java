package io.bari.qshare;

import android.Manifest;
import android.content.Context;
import android.content.pm.PackageManager;
import android.net.wifi.WifiManager;
import android.net.wifi.WifiConfiguration;
import android.net.ConnectivityManager;
import android.net.NetworkRequest;
import android.net.NetworkSpecifier;
import android.net.wifi.WifiNetworkSpecifier;
import android.os.Build;
import android.os.Handler;
import android.os.Looper;
import android.util.Log;

import androidx.core.app.ActivityCompat;

import java.net.InetAddress;
import java.net.NetworkInterface;
import java.util.Collections;

public class HotspotManager {
    private static final String TAG = "HotspotManager";

    private Context m_context;
    private WifiManager m_wifiManager;
    private WifiManager.LocalOnlyHotspotReservation m_reservation;

    // Callbacks back into C++ via JNI
    public native void onHotspotStarted(String ssid, String psk, String ip);
    public native void onHotspotFailed(String reason);
    public native void onHotspotStopped();

    public HotspotManager(Context context) {
        m_context = context;
        m_wifiManager = (WifiManager) context.getSystemService(Context.WIFI_SERVICE);
    }

    // Called from C++ to start the hotspot
    public void startHotspot() {
        if (Build.VERSION.SDK_INT < 26) {
            onHotspotFailed("API < 26 not supported");
            return;
        }

        Handler handler = new Handler(Looper.getMainLooper());

//        if (ActivityCompat.checkSelfPermission(this, Manifest.permission.ACCESS_FINE_LOCATION) != PackageManager.PERMISSION_GRANTED || ActivityCompat.checkSelfPermission(this, Manifest.permission.NEARBY_WIFI_DEVICES) != PackageManager.PERMISSION_GRANTED) {
//            // TODO: Consider calling
//            //    ActivityCompat#requestPermissions
//            // here to request the missing permissions, and then overriding
//            //   public void onRequestPermissionsResult(int requestCode, String[] permissions,
//            //                                          int[] grantResults)
//            // to handle the case where the user grants the permission. See the documentation
//            // for ActivityCompat#requestPermissions for more details.
//            return;
//        }
        m_wifiManager.startLocalOnlyHotspot(new WifiManager.LocalOnlyHotspotCallback() {
            @Override
            public void onStarted(WifiManager.LocalOnlyHotspotReservation reservation) {
                m_reservation = reservation;
                WifiConfiguration config = reservation.getWifiConfiguration();

                String ssid = "";
                String psk  = "";

                if (Build.VERSION.SDK_INT >= 30) {
                    // API 30+: WifiConfiguration deprecated, use SoftApConfiguration
                    android.net.wifi.SoftApConfiguration sac =
                            reservation.getSoftApConfiguration();
                    ssid = sac.getSsid() != null ? sac.getSsid() : "";
                    // passphrase only available for WPA2/WPA3
                    psk  = sac.getPassphrase() != null ? sac.getPassphrase() : "";
                } else {
                    ssid = config.SSID;
                    psk  = config.preSharedKey;
                }

                // Strip surrounding quotes Qt/Android sometimes add
                if (ssid.startsWith("\"")) ssid = ssid.substring(1, ssid.length()-1);
                if (psk.startsWith("\""))  psk  = psk.substring(1, psk.length()-1);

                // The hotspot gateway is always 192.168.43.1 on most Android builds,
                // but let's read it properly
                String ip = getHotspotIp();

                Log.d(TAG, "Hotspot started: " + ssid + " ip=" + ip);
                onHotspotStarted(ssid, psk, ip);
            }

            @Override
            public void onFailed(int reason) {
                onHotspotFailed("Reason code: " + reason);
            }

            @Override
            public void onStopped() {
                onHotspotStopped();
            }
        }, handler);
    }

    public void stopHotspot() {
        if (m_reservation != null) {
            m_reservation.close();
            m_reservation = null;
        }
    }

    // Reads the hotspot gateway IP from the wlan interface
    private String getHotspotIp() {
        try {
            for (NetworkInterface iface :
                    Collections.list(NetworkInterface.getNetworkInterfaces())) {
                // hotspot interface is usually "wlan0" or "ap0"
                if (!iface.getName().startsWith("wlan") &&
                        !iface.getName().startsWith("ap")) continue;
                for (InetAddress addr :
                        Collections.list(iface.getInetAddresses())) {
                    if (!addr.isLoopbackAddress() &&
                            addr.getHostAddress().contains(".")) {
                        return addr.getHostAddress();
                    }
                }
            }
        } catch (Exception e) {
            Log.e(TAG, "getHotspotIp failed", e);
        }
        return "192.168.43.1"; // safe fallback
    }

    // ── Client side ──────────────────────────────────────────────────────────
    // Android 10+ requires ConnectivityManager for joining a specific hotspot.
    // Below 10 you can write WifiConfiguration directly — but let's target 10+.

    private ConnectivityManager.NetworkCallback m_networkCallback;

    public void connectToHotspot(String ssid, String psk) {
        if (Build.VERSION.SDK_INT < 29) {
            connectLegacy(ssid, psk);
            return;
        }

        ConnectivityManager cm = (ConnectivityManager)
                m_context.getSystemService(Context.CONNECTIVITY_SERVICE);

        WifiNetworkSpecifier spec = new WifiNetworkSpecifier.Builder()
                .setSsid(ssid)
                .setWpa2Passphrase(psk)
                .build();

        NetworkRequest req = new NetworkRequest.Builder()
                .addTransportType(android.net.NetworkCapabilities.TRANSPORT_WIFI)
                .setNetworkSpecifier(spec)
                .build();

        m_networkCallback = new ConnectivityManager.NetworkCallback() {
            @Override
            public void onAvailable(android.net.Network network) {
                // Bind process to this network so sockets use it
                cm.bindProcessToNetwork(network);
                Log.d(TAG, "Connected to peer hotspot");
                // Notify C++ — host IP is always the gateway we got from BLE
                onHotspotStarted("CLIENT_CONNECTED", "", "");
            }
            @Override
            public void onUnavailable() {
                onHotspotFailed("Could not connect to peer hotspot");
            }
        };

        cm.requestNetwork(req, m_networkCallback);
    }

    @SuppressWarnings("deprecation")
    private void connectLegacy(String ssid, String psk) {
        WifiConfiguration conf = new WifiConfiguration();
        conf.SSID = "\"" + ssid + "\"";
        conf.preSharedKey = "\"" + psk + "\"";
        int netId = m_wifiManager.addNetwork(conf);
        m_wifiManager.disconnect();
        m_wifiManager.enableNetwork(netId, true);
        m_wifiManager.reconnect();
        onHotspotStarted("CLIENT_CONNECTED_LEGACY", "", "");
    }
}

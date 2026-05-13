package io.bari.qshare;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.net.wifi.p2p.*;
import android.net.wifi.p2p.WifiP2pManager.*;
import android.net.NetworkInfo;
import android.os.Handler;
import android.os.Looper;
import android.util.Log;

import java.net.InetAddress;
import java.net.NetworkInterface;
import java.util.Collections;

public class HotspotManager {
    private static final String TAG = "HotspotManager";

    private final Context         m_context;
    private WifiP2pManager        m_p2pManager;
    private WifiP2pManager.Channel m_channel;
    private BroadcastReceiver     m_receiver;
    private boolean               m_isGroupOwner = false;
    private String                m_groupOwnerIp = "";

    // ── JNI callbacks ─────────────────────────────────────────────────────────
    public native void onHotspotStarted(String ssid, String psk, String ip);
    public native void onHotspotFailed(String reason);
    public native void onHotspotStopped();

    public HotspotManager(Context context) {
        m_context   = context;
        m_p2pManager = (WifiP2pManager)
                context.getSystemService(Context.WIFI_P2P_SERVICE);
        m_channel   = m_p2pManager.initialize(
                context, Looper.getMainLooper(), null);
    }

    // ── Called by HOST via JNI ────────────────────────────────────────────────
    public void startHotspot() {
        registerReceiver();

        // Create a persistent P2P group — this device becomes Group Owner
        m_p2pManager.createGroup(m_channel, new ActionListener() {
            @Override public void onSuccess() {
                Log.d(TAG, "createGroup() queued");
                // Actual SSID/PSK arrive in the broadcast receiver below
            }
            @Override public void onFailure(int reason) {
                // reason 2 = BUSY (group already exists) — request info anyway
                if (reason == 2) {
                    Log.w(TAG, "createGroup busy — requesting existing group info");
                    requestGroupInfo();
                } else {
                    onHotspotFailed("createGroup failed: " + reason);
                }
            }
        });
    }

    // ── Called by CLIENT via JNI ──────────────────────────────────────────────
    // peerAddress = WiFi MAC of the host, from BLE payload
    public void connectToHotspot(String peerAddress, String unusedPsk) {
        registerReceiver();

        WifiP2pConfig config = new WifiP2pConfig();
        config.deviceAddress = peerAddress.toLowerCase();
        config.wps.setup     = android.net.wifi.WpsInfo.PBC;
        config.groupOwnerIntent = 0; // hint: we prefer NOT to be owner

        m_p2pManager.connect(m_channel, config, new ActionListener() {
            @Override public void onSuccess() {
                Log.d(TAG, "connect() queued — waiting for CONNECTION_CHANGED");
            }
            @Override public void onFailure(int reason) {
                onHotspotFailed("P2P connect failed: " + reason);
            }
        });
    }

    public void stopHotspot() {
        m_p2pManager.removeGroup(m_channel, new ActionListener() {
            @Override public void onSuccess() { Log.d(TAG, "Group removed"); }
            @Override public void onFailure(int r) { Log.w(TAG, "removeGroup: " + r); }
        });
        unregisterReceiver();
        onHotspotStopped();
    }

    // ── Broadcast receiver ────────────────────────────────────────────────────
    private void registerReceiver() {
        if (m_receiver != null) return;

        IntentFilter filter = new IntentFilter();
        filter.addAction(WifiP2pManager.WIFI_P2P_STATE_CHANGED_ACTION);
        filter.addAction(WifiP2pManager.WIFI_P2P_CONNECTION_CHANGED_ACTION);
        filter.addAction(WifiP2pManager.WIFI_P2P_THIS_DEVICE_CHANGED_ACTION);

        m_receiver = new BroadcastReceiver() {
            @Override
            public void onReceive(Context ctx, Intent intent) {
                String action = intent.getAction();
                if (action == null) return;

                if (WifiP2pManager.WIFI_P2P_CONNECTION_CHANGED_ACTION.equals(action)) {
                    NetworkInfo netInfo = intent.getParcelableExtra(
                            WifiP2pManager.EXTRA_NETWORK_INFO);
                    if (netInfo != null && netInfo.isConnected()) {
                        requestGroupInfo();
                    }
                }
            }
        };

        m_context.registerReceiver(m_receiver, filter);
    }

    private void unregisterReceiver() {
        if (m_receiver != null) {
            try { m_context.unregisterReceiver(m_receiver); }
            catch (Exception e) { /* already unregistered */ }
            m_receiver = null;
        }
    }

    private void requestGroupInfo() {
        m_p2pManager.requestGroupInfo(m_channel, group -> {
            if (group == null) {
                // Not ready yet — retry once after 500ms
                new Handler(Looper.getMainLooper()).postDelayed(
                        this::requestGroupInfo, 500);
                return;
            }

            m_isGroupOwner = group.isGroupOwner();
            String ssid    = group.getNetworkName();
            String psk     = group.getPassphrase();

            Log.d(TAG, "Group info: owner=" + m_isGroupOwner
                    + " ssid=" + ssid);

            if (m_isGroupOwner) {
                // My P2P interface IP is always 192.168.49.1 on Android
                onHotspotStarted(ssid, psk, "192.168.49.1");
            } else {
                // Client: group owner IP is in the WifiP2pInfo
                m_p2pManager.requestConnectionInfo(m_channel, info -> {
                    String ownerIp = info.groupOwnerAddress != null
                            ? info.groupOwnerAddress.getHostAddress()
                            : "192.168.49.1";
                    // Signal CLIENT_CONNECTED — C++ uses the ip from BLE payload
                    onHotspotStarted("CLIENT_CONNECTED", "", ownerIp);
                });
            }
        });
    }

    // Call this from C++ before startHotspot() to get our P2P MAC
    public void getP2pMacAddress() {
        m_p2pManager.requestDeviceInfo(m_channel, device -> {
            if (device != null) {
                onHotspotStarted("MAC", "", device.deviceAddress);
                // reusing onHotspotStarted as a callback channel —
                // C++ checks ssid=="MAC" and extracts ip field as the MAC
            }
        });
    }
}

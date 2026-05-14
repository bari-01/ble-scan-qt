package io.bari.qshare;

import android.Manifest;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.pm.PackageManager;
import android.net.NetworkInfo;
import android.net.wifi.p2p.*;
import android.net.wifi.p2p.WifiP2pManager.*;
import android.os.Handler;
import android.os.Looper;
import android.util.Log;

import androidx.core.app.ActivityCompat;

import java.util.Collection;

public class HotspotManager {
    private static final String TAG = "HotspotManager";

    private final Context          m_context;
    private WifiP2pManager         m_p2pManager;
    private WifiP2pManager.Channel m_channel;
    private BroadcastReceiver      m_receiver;
    private boolean                m_groupInfoRequested = false;

    public native void onHotspotStarted(String ssid, String psk, String ip);
    public native void onHotspotFailed(String reason);
    public native void onHotspotStopped();
    public native void onPeerFound(String deviceName, String deviceAddress);
    private boolean m_connecting = false;

    public HotspotManager(Context context) {
        m_context    = context;
        m_p2pManager = (WifiP2pManager)
                context.getSystemService(Context.WIFI_P2P_SERVICE);
        m_channel    = m_p2pManager.initialize(
                context, Looper.getMainLooper(), null);
    }

    // ── HOST: create the group ────────────────────────────────────────────────
    public void startHotspot() {
        registerReceiver();
        if (ActivityCompat.checkSelfPermission(m_context, Manifest.permission.ACCESS_FINE_LOCATION) != PackageManager.PERMISSION_GRANTED || ActivityCompat.checkSelfPermission(m_context, Manifest.permission.NEARBY_WIFI_DEVICES) != PackageManager.PERMISSION_GRANTED) {
            return;
        }
        m_p2pManager.createGroup(m_channel, new ActionListener() {
            @Override public void onSuccess() {
                Log.d(TAG, "createGroup queued");
                new Handler(Looper.getMainLooper()).postDelayed(
                        () -> requestGroupInfo(), 300);
            }
            @Override public void onFailure(int reason) {
                if (reason == 2) { // BUSY = group already exists
                    Log.w(TAG, "createGroup busy — using existing group");
                    requestGroupInfo();
                } else {
                    onHotspotFailed("createGroup failed: " + reason);
                }
            }
        });
    }

    // ── CLIENT: discover peers then connect ───────────────────────────────────
    public void discoverAndConnect() {
        registerReceiver();
        if (ActivityCompat.checkSelfPermission(m_context, Manifest.permission.ACCESS_FINE_LOCATION) != PackageManager.PERMISSION_GRANTED || ActivityCompat.checkSelfPermission(m_context, Manifest.permission.NEARBY_WIFI_DEVICES) != PackageManager.PERMISSION_GRANTED) {
            return;
        }
        m_p2pManager.discoverPeers(m_channel, new ActionListener() {
            @Override public void onSuccess() {
                Log.d(TAG, "discoverPeers started");
            }
            @Override public void onFailure(int reason) {
                onHotspotFailed("discoverPeers failed: " + reason);
            }
        });
        // Results arrive in broadcast receiver → WIFI_P2P_PEERS_CHANGED_ACTION
    }

    // Called from C++ once the user/app picks which peer to connect to
    public void connectToPeer(String deviceAddress) {
        if (m_connecting) {
            return;
        }
        m_connecting = true;

        WifiP2pConfig config = new WifiP2pConfig();
        config.deviceAddress   = deviceAddress;
        config.wps.setup       = android.net.wifi.WpsInfo.PBC;
        config.groupOwnerIntent = 0;

        if (ActivityCompat.checkSelfPermission(m_context, Manifest.permission.ACCESS_FINE_LOCATION) != PackageManager.PERMISSION_GRANTED || ActivityCompat.checkSelfPermission(m_context, Manifest.permission.NEARBY_WIFI_DEVICES) != PackageManager.PERMISSION_GRANTED) {
            return;
        }
        m_p2pManager.connect(m_channel, config, new ActionListener() {
            @Override public void onSuccess() {
                Log.d(TAG, "connect() queued for " + deviceAddress);
            }
            @Override public void onFailure(int reason) {
                m_connecting = false; // allow retry
                onHotspotFailed("connect failed: " + reason);
            }
        });
    }

    public void stopHotspot() {
        m_groupInfoRequested = false;
        m_connecting = false;

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
        filter.addAction(WifiP2pManager.WIFI_P2P_PEERS_CHANGED_ACTION);
        filter.addAction(WifiP2pManager.WIFI_P2P_CONNECTION_CHANGED_ACTION);
        filter.addAction(WifiP2pManager.WIFI_P2P_THIS_DEVICE_CHANGED_ACTION);

        m_receiver = new BroadcastReceiver() {
            @Override
            public void onReceive(Context ctx, Intent intent) {
                String action = intent.getAction();
                if (action == null) return;

                switch (action) {
                    case WifiP2pManager.WIFI_P2P_PEERS_CHANGED_ACTION:
                        // Peer list ready — request it

                        if (ActivityCompat.checkSelfPermission(m_context, Manifest.permission.ACCESS_FINE_LOCATION) != PackageManager.PERMISSION_GRANTED || ActivityCompat.checkSelfPermission(m_context, Manifest.permission.NEARBY_WIFI_DEVICES) != PackageManager.PERMISSION_GRANTED) {
                            // TODO: Consider calling
                            //    ActivityCompat#requestPermissions
                            // here to request the missing permissions, and then overriding
                            //   public void onRequestPermissionsResult(int requestCode, String[] permissions,
                            //                                          int[] grantResults)
                            // to handle the case where the user grants the permission. See the documentation
                            // for ActivityCompat#requestPermissions for more details.
                            return;
                        }
                        m_p2pManager.requestPeers(m_channel, peerList -> {
                            Collection<WifiP2pDevice> peers =
                                    peerList.getDeviceList();
                            Log.d(TAG, "Peers available: " + peers.size());
                            for (WifiP2pDevice peer : peers) {
                                Log.d(TAG, "  Peer: " + peer.deviceName
                                        + " addr=" + peer.deviceAddress
                                        + " status=" + peer.status);
                                // Fire callback to C++ for each peer
                                // C++ will pick the right one (e.g. by name
                                // matching the BLE advertised name "P2PNode")
                                onPeerFound(peer.deviceName,
                                        peer.deviceAddress);
                            }
                        });
                        break;

                    case WifiP2pManager.WIFI_P2P_CONNECTION_CHANGED_ACTION:
                        NetworkInfo netInfo = intent.getParcelableExtra(
                                WifiP2pManager.EXTRA_NETWORK_INFO);
                        if (netInfo != null && netInfo.isConnected()) {
                            requestGroupInfo();
                        }
                        break;
                }
            }
        };
        m_context.registerReceiver(m_receiver, filter);
    }

    private void unregisterReceiver() {
        if (m_receiver != null) {
            try { m_context.unregisterReceiver(m_receiver); }
            catch (Exception ignored) {}
            m_receiver = null;
        }
    }

    private void requestGroupInfo() {
        if (m_groupInfoRequested) return;
        m_groupInfoRequested = true;

        if (ActivityCompat.checkSelfPermission(m_context, Manifest.permission.ACCESS_FINE_LOCATION) != PackageManager.PERMISSION_GRANTED || ActivityCompat.checkSelfPermission(m_context, Manifest.permission.NEARBY_WIFI_DEVICES) != PackageManager.PERMISSION_GRANTED) {
            return;
        }
        m_p2pManager.requestGroupInfo(m_channel, group -> {
            if (group == null) {
                m_groupInfoRequested = false;
                new Handler(Looper.getMainLooper()).postDelayed(
                        this::requestGroupInfo, 500);
                return;
            }

            boolean isOwner = group.isGroupOwner();
            String  ssid    = group.getNetworkName();
            String  psk     = group.getPassphrase();
            Log.d(TAG, "Group: owner=" + isOwner + " ssid=" + ssid);

            if (isOwner) {
                onHotspotStarted(ssid, psk, "192.168.49.1");
            } else {
                m_p2pManager.requestConnectionInfo(m_channel, info -> {
                    String ip = info.groupOwnerAddress != null
                            ? info.groupOwnerAddress.getHostAddress()
                            : "192.168.49.1";
                    onHotspotStarted("CLIENT_CONNECTED", "", ip);
                });
            }
        });
    }
}
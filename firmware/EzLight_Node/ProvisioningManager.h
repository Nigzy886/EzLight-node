#pragma once

#include <Arduino.h>
#include <Preferences.h>
#include <WiFi.h>
#include <esp_now.h>
#include <esp_arduino_version.h>

class ProvisioningManager {
public:
  ProvisioningManager();

  void begin();
  void update();

  const String& nodeId() const;
  const String& hostName() const;
  const String& friendlyName() const;
  bool provisioned() const;
  bool connected() const;
  String ipAddress() const;
  int32_t wifiRssi() const;
  int wifiChannel() const;

private:
  static ProvisioningManager* _instance;

  Preferences _prefs;
  String _nodeId;
  String _hostName;
  String _friendlyName;
  String _ssid;
  String _pass;
  String _hubMacText;
  uint8_t _hubMac[6];
  bool _hubKnown;
  bool _provisioned;
  bool _espNowStarted;
  bool _mdnsStarted;
  uint8_t _scanChannel;
  int _savedWifiChannel;
  unsigned long _lastChannelHopMs;
  unsigned long _lastReconnectMs;
  unsigned long _lastHelloMs;

  void loadProvisioning();
  bool saveProvisioning(const String& ssid, const String& pass, const String& hubMacText, int wifiChannel);
  bool startEspNow();
  bool connectWifi(uint32_t timeoutMs);
  void startMdns();
  void hopCommissioningChannel();
  bool addPeer(const uint8_t* mac);
  bool sendJson(const uint8_t* mac, const String& json);
  void sendHello(const uint8_t* mac, bool includeAddress);
  void sendWifiCfgAck(const uint8_t* mac, const String& msgId, bool ok, const String& error);
  void handleMessage(const uint8_t* senderMac, const uint8_t* data, int len);

  static String macToString(const uint8_t* mac);
  static bool parseMac(const String& text, uint8_t* out);

#if defined(ESP_ARDUINO_VERSION_MAJOR) && ESP_ARDUINO_VERSION_MAJOR >= 3
  static void onEspNowRecv(const esp_now_recv_info_t* info, const uint8_t* data, int len);
#else
  static void onEspNowRecv(const uint8_t* mac, const uint8_t* data, int len);
#endif
};

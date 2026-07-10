#include "ProvisioningManager.h"

#include <ArduinoJson.h>
#include <ESPmDNS.h>
#include <esp_wifi.h>

#include "EzLightTypes.h"

namespace {
String jsonEscape(String value) {
  value.replace("\\", "\\\\");
  value.replace("\"", "\\\"");
  value.replace("\n", "\\n");
  value.replace("\r", "\\r");
  return value;
}

String quote(const String& value) {
  return "\"" + jsonEscape(value) + "\"";
}

String stringField(JsonDocument& doc, const char* canonical, const char* alias = nullptr) {
  if (doc[canonical].is<const char*>()) {
    return doc[canonical].as<const char*>();
  }
  if (alias != nullptr && doc[alias].is<const char*>()) {
    return doc[alias].as<const char*>();
  }
  return "";
}
}  // namespace

ProvisioningManager* ProvisioningManager::_instance = nullptr;

ProvisioningManager::ProvisioningManager()
  : _nodeId(EZLIGHT_NODE_ID),
    _hostName("ezlight-001"),
    _friendlyName("Outdoor Lights"),
    _hubMac{0, 0, 0, 0, 0, 0},
    _hubKnown(false),
    _provisioned(false),
    _espNowStarted(false),
    _mdnsStarted(false),
    _scanChannel(1),
    _savedWifiChannel(0),
    _lastChannelHopMs(0),
    _lastReconnectMs(0),
    _lastHelloMs(0) {}

void ProvisioningManager::begin() {
  _instance = this;
  loadProvisioning();

  WiFi.mode(WIFI_STA);
  WiFi.setHostname(_hostName.c_str());
  startEspNow();

  if (_provisioned) {
    connectWifi(30000UL);
  }
}

void ProvisioningManager::update() {
  const bool wifiConnected = WiFi.status() == WL_CONNECTED;

  if (!wifiConnected && _mdnsStarted) {
    MDNS.end();
    _mdnsStarted = false;
  }

  if (!_provisioned) {
    hopCommissioningChannel();
    return;
  }

  if (!wifiConnected) {
    if (millis() - _lastReconnectMs >= 30000UL) {
      _lastReconnectMs = millis();
      connectWifi(10000UL);
    }
    return;
  }

  startMdns();
  if (_hubKnown && millis() - _lastHelloMs >= 60000UL) {
    sendHello(_hubMac, true);
  }
}

const String& ProvisioningManager::nodeId() const { return _nodeId; }
const String& ProvisioningManager::hostName() const { return _hostName; }
const String& ProvisioningManager::friendlyName() const { return _friendlyName; }
bool ProvisioningManager::provisioned() const { return _provisioned; }
bool ProvisioningManager::connected() const { return WiFi.status() == WL_CONNECTED; }
String ProvisioningManager::ipAddress() const { return connected() ? WiFi.localIP().toString() : ""; }
int32_t ProvisioningManager::wifiRssi() const { return connected() ? WiFi.RSSI() : 0; }
int ProvisioningManager::wifiChannel() const { return connected() ? WiFi.channel() : _savedWifiChannel; }

void ProvisioningManager::loadProvisioning() {
  if (!_prefs.begin("ezlight", false)) {
    _provisioned = false;
    return;
  }

  _nodeId = _prefs.getString("node_id", EZLIGHT_NODE_ID);
  if (_nodeId.length() == 0) {
    _nodeId = EZLIGHT_NODE_ID;
  }
  _hostName = _prefs.getString("host", "ezlight-001");
  if (_hostName.length() == 0) {
    _hostName = "ezlight-001";
  }
  _friendlyName = _prefs.getString("name", "Outdoor Lights");
  _ssid = _prefs.getString("ssid", "");
  _pass = _prefs.getString("pass", "");
  _hubMacText = _prefs.getString("hub_mac", "");
  _savedWifiChannel = _prefs.getInt("wifi_channel", 0);
  _hubKnown = parseMac(_hubMacText, _hubMac);
  _provisioned = _prefs.getBool("provisioned", false) && _ssid.length() > 0 && _hubKnown;
}

bool ProvisioningManager::saveProvisioning(const String& ssid, const String& pass, const String& hubMacText, int wifiChannel) {
  uint8_t parsedHubMac[6];
  if (ssid.length() == 0 || wifiChannel < 1 || wifiChannel > 13 || !parseMac(hubMacText, parsedHubMac)) {
    return false;
  }

  _ssid = ssid;
  _pass = pass;
  _hubMacText = hubMacText;
  _savedWifiChannel = wifiChannel;
  memcpy(_hubMac, parsedHubMac, sizeof(_hubMac));
  _hubKnown = true;

  _prefs.putString("node_id", _nodeId);
  _prefs.putString("host", _hostName);
  _prefs.putString("name", _friendlyName);
  _prefs.putString("ssid", _ssid);
  _prefs.putString("pass", _pass);
  _prefs.putString("hub_mac", _hubMacText);
  _prefs.putInt("wifi_channel", _savedWifiChannel);
  _prefs.putBool("provisioned", true);

  _provisioned = true;
  return true;
}

bool ProvisioningManager::startEspNow() {
  if (_espNowStarted) {
    return true;
  }

  WiFi.mode(WIFI_STA);
  if (esp_now_init() != ESP_OK) {
    Serial.println("ESP-NOW init failed");
    return false;
  }

  if (esp_now_register_recv_cb(onEspNowRecv) != ESP_OK) {
    Serial.println("ESP-NOW receive callback registration failed");
    esp_now_deinit();
    return false;
  }

  _espNowStarted = true;
  if (_hubKnown) {
    addPeer(_hubMac);
  }
  return true;
}

bool ProvisioningManager::connectWifi(uint32_t timeoutMs) {
  if (!_provisioned || _ssid.length() == 0) {
    return false;
  }

  WiFi.mode(WIFI_STA);
  WiFi.setHostname(_hostName.c_str());
  WiFi.begin(_ssid.c_str(), _pass.c_str());

  const unsigned long startedAt = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - startedAt < timeoutMs) {
    delay(250);
  }

  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("Provisioned Wi-Fi connection failed");
    return false;
  }

  const int liveChannel = WiFi.channel();
  if (liveChannel > 0 && liveChannel != _savedWifiChannel) {
    _savedWifiChannel = liveChannel;
    _prefs.putInt("wifi_channel", _savedWifiChannel);
  }

  startMdns();
  startEspNow();
  if (_hubKnown) {
    addPeer(_hubMac);
    sendHello(_hubMac, true);
  }

  Serial.print("Wi-Fi connected: ");
  Serial.println(WiFi.localIP());
  return true;
}

void ProvisioningManager::startMdns() {
  if (_mdnsStarted || WiFi.status() != WL_CONNECTED) {
    return;
  }

  if (MDNS.begin(_hostName.c_str())) {
    MDNS.addService("http", "tcp", 80);
    MDNS.addServiceTxt("http", "tcp", "node_id", _nodeId);
    MDNS.addServiceTxt("http", "tcp", "profile", EZLIGHT_PROFILE);
    _mdnsStarted = true;
  }
}

void ProvisioningManager::hopCommissioningChannel() {
  if (WiFi.status() == WL_CONNECTED || millis() - _lastChannelHopMs < 350UL) {
    return;
  }

  _lastChannelHopMs = millis();
  esp_wifi_set_channel(_scanChannel, WIFI_SECOND_CHAN_NONE);
  _scanChannel++;
  if (_scanChannel > 13) {
    _scanChannel = 1;
  }
}

bool ProvisioningManager::addPeer(const uint8_t* mac) {
  if (!_espNowStarted || mac == nullptr) {
    return false;
  }
  if (esp_now_is_peer_exist(mac)) {
    return true;
  }

  esp_now_peer_info_t peer = {};
  memcpy(peer.peer_addr, mac, 6);
  peer.channel = 0;
  peer.encrypt = false;
  peer.ifidx = WIFI_IF_STA;
  return esp_now_add_peer(&peer) == ESP_OK;
}

bool ProvisioningManager::sendJson(const uint8_t* mac, const String& json) {
  if (!_espNowStarted || mac == nullptr || json.length() > 249) {
    return false;
  }
  if (!addPeer(mac)) {
    return false;
  }
  return esp_now_send(mac, reinterpret_cast<const uint8_t*>(json.c_str()), json.length()) == ESP_OK;
}

void ProvisioningManager::sendHello(const uint8_t* mac, bool includeAddress) {
  String json = "{";
  json += "\"e\":\"easysystems\",";
  json += "\"t\":\"hello\",";
  json += "\"nid\":" + quote(_nodeId) + ",";
  json += "\"mdl\":" + quote(EZLIGHT_MODEL) + ",";
  json += "\"fw\":" + quote(EZLIGHT_FW) + ",";
  json += "\"ver\":" + quote(EZLIGHT_VERSION) + ",";
  json += "\"pf\":" + quote(EZLIGHT_PROFILE) + ",";
  json += "\"transport\":\"wifi_primary\",";
  json += "\"provisioned\":" + String(_provisioned ? "true" : "false");
  if (includeAddress && connected()) {
    json += ",\"ip\":" + quote(WiFi.localIP().toString());
    json += ",\"host\":" + quote(_hostName);
  }
  json += "}";

  if (sendJson(mac, json)) {
    _lastHelloMs = millis();
  }
}

void ProvisioningManager::sendWifiCfgAck(const uint8_t* mac, const String& msgId, bool ok, const String& error) {
  String json = "{";
  json += "\"e\":\"easysystems\",";
  json += "\"t\":\"ack\",";
  json += "\"nid\":" + quote(_nodeId) + ",";
  json += "\"mid\":" + quote(msgId) + ",";
  json += "\"for\":\"wifi_cfg\",";
  json += "\"ok\":" + String(ok ? "true" : "false") + ",";
  json += "\"status\":\"" + String(ok ? "accepted" : "rejected") + "\"";
  if (!ok) {
    json += ",\"error\":" + quote(error);
  }
  json += "}";
  sendJson(mac, json);
}

void ProvisioningManager::handleMessage(const uint8_t* senderMac, const uint8_t* data, int len) {
  if (senderMac == nullptr || data == nullptr || len <= 0 || len > 249) {
    return;
  }

  JsonDocument doc;
  const DeserializationError parseError = deserializeJson(doc, data, len);
  if (parseError || !doc.is<JsonObject>()) {
    return;
  }

  const String eco = stringField(doc, "eco", "e");
  const String type = stringField(doc, "type", "t");
  if (eco != "easysystems") {
    return;
  }

  if (type == "pair_beacon" || type == "hello") {
    if (_provisioned && (!_hubKnown || memcmp(senderMac, _hubMac, 6) != 0)) {
      return;
    }
    addPeer(senderMac);
    sendHello(senderMac, connected());
    return;
  }

  if (type != "wifi_cfg" || _provisioned) {
    return;
  }

  const String msgId = stringField(doc, "msg_id", "mid");
  const String targetNodeId = stringField(doc, "node_id", "nid");
  if (msgId.length() == 0) {
    sendWifiCfgAck(senderMac, "", false, "invalid_args");
    return;
  }
  if (targetNodeId != _nodeId) {
    sendWifiCfgAck(senderMac, msgId, false, "wrong_target");
    return;
  }

  if (!doc["ssid"].is<const char*>() || !doc["pass"].is<const char*>() || !doc["wifi_channel"].is<int>()) {
    sendWifiCfgAck(senderMac, msgId, false, "invalid_args");
    return;
  }

  const String ssid = doc["ssid"].as<const char*>();
  const String pass = doc["pass"].as<const char*>();
  const int wifiChannel = doc["wifi_channel"].as<int>();
  String declaredHubMac = stringField(doc, "hub_mac");
  if (declaredHubMac.length() == 0) {
    declaredHubMac = macToString(senderMac);
  }

  uint8_t parsedHubMac[6];
  if (!parseMac(declaredHubMac, parsedHubMac) || memcmp(parsedHubMac, senderMac, 6) != 0) {
    sendWifiCfgAck(senderMac, msgId, false, "wrong_target");
    return;
  }

  if (!saveProvisioning(ssid, pass, macToString(senderMac), wifiChannel)) {
    sendWifiCfgAck(senderMac, msgId, false, "invalid_args");
    return;
  }

  sendWifiCfgAck(senderMac, msgId, true, "");
  delay(200);
  WiFi.disconnect(true, true);
  delay(50);
  ESP.restart();
}

String ProvisioningManager::macToString(const uint8_t* mac) {
  char output[18];
  snprintf(output, sizeof(output), "%02X:%02X:%02X:%02X:%02X:%02X", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
  return String(output);
}

bool ProvisioningManager::parseMac(const String& text, uint8_t* out) {
  if (out == nullptr || text.length() != 17) {
    return false;
  }

  int values[6];
  if (sscanf(text.c_str(), "%x:%x:%x:%x:%x:%x", &values[0], &values[1], &values[2], &values[3], &values[4], &values[5]) != 6) {
    return false;
  }

  for (uint8_t i = 0; i < 6; ++i) {
    if (values[i] < 0 || values[i] > 255) {
      return false;
    }
    out[i] = static_cast<uint8_t>(values[i]);
  }
  return true;
}

#if defined(ESP_ARDUINO_VERSION_MAJOR) && ESP_ARDUINO_VERSION_MAJOR >= 3
void ProvisioningManager::onEspNowRecv(const esp_now_recv_info_t* info, const uint8_t* data, int len) {
  if (_instance != nullptr && info != nullptr) {
    _instance->handleMessage(info->src_addr, data, len);
  }
}
#else
void ProvisioningManager::onEspNowRecv(const uint8_t* mac, const uint8_t* data, int len) {
  if (_instance != nullptr) {
    _instance->handleMessage(mac, data, len);
  }
}
#endif

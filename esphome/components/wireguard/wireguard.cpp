#include "wireguard.h"

#include <functional>
#include "esphome/core/log.h"
#include "esp_err.h"
#include "esp_wireguard.h"

namespace esphome {
namespace wireguard {

static const char * const TAG = "wireguard";

void Wireguard::setup() {
    ESP_LOGD(TAG, "initializing...");

    this->wg_config_.allowed_ip = this->address_.c_str();
    this->wg_config_.private_key = this->private_key_.c_str();
    this->wg_config_.endpoint = this->peer_endpoint_.c_str();
    this->wg_config_.public_key = this->peer_public_key_.c_str();
    this->wg_config_.port = this->peer_port_;
    this->wg_config_.allowed_ip_mask = this->netmask_.c_str();
    this->wg_config_.persistent_keepalive = this->keepalive_;

    if(preshared_key_.length() > 0)
        this->wg_config_.preshared_key = this->preshared_key_.c_str();

    wg_initialized_ = esp_wireguard_init(&(this->wg_config_), &(this->wg_ctx_));

    if(wg_initialized_ == ESP_OK) {
        ESP_LOGI(TAG, "initialized");
        this->srctime_->add_on_time_sync_callback(std::bind(&Wireguard::start_connection_, this));
        this->start_connection_();
    } else {
        ESP_LOGE(TAG, "cannot initialize, error code %d", this->wg_initialized_);
        this->mark_failed();
    }
}

void Wireguard::update() {
    if(this->wg_initialized_ == ESP_OK && this->wg_connected_ == ESP_OK) {
        this->wg_peer_up_ = (esp_wireguardif_peer_is_up(&(this->wg_ctx_)) == ESP_OK);
    } else {
        ESP_LOGD(TAG, "initialized: %s (error %d)", (this->wg_initialized_ == ESP_OK ? "yes" : "no"), this->wg_initialized_);
        ESP_LOGD(TAG, "connection: %s (error %d)", (this->wg_connected_ == ESP_OK ? "active" : "inactive"), this->wg_connected_);
        this->wg_peer_up_ = false;
    }

    if(this->wg_peer_up_) {
        if(this->wg_last_peer_up_ == 0) {
            ESP_LOGI(TAG, "peer online");
            this->wg_last_peer_up_ = this->srctime_->utcnow().timestamp;
        }
    } else {
        ESP_LOGD(TAG, "peer offline");
        this->wg_last_peer_up_ = 0;
    }
}

void Wireguard::dump_config() {
    ESP_LOGCONFIG(TAG, "Configuration");
    ESP_LOGCONFIG(TAG, "  address: %s", this->address_.c_str());
    ESP_LOGCONFIG(TAG, "  netmask: %s", this->netmask_.c_str());
    ESP_LOGCONFIG(TAG, "  private key: %s...CUT...=", this->private_key_.substr(0,5).c_str());
    ESP_LOGCONFIG(TAG, "  peer endpoint: %s", this->peer_endpoint_.c_str());
    ESP_LOGCONFIG(TAG, "  peer port: %d", this->peer_port_);
    ESP_LOGCONFIG(TAG, "  peer public key: %s", this->peer_public_key_.c_str());
    ESP_LOGCONFIG(TAG, "  peer preshared key: %s...CUT...=", this->preshared_key_.substr(0,5).c_str());
    ESP_LOGCONFIG(TAG, "  peer persistent keepalive: %d", this->keepalive_);
}

void Wireguard::on_shutdown() {
    if(this->wg_initialized_ == ESP_OK && this->wg_connected_ == ESP_OK) {
        ESP_LOGD(TAG, "disconnecting...");
        esp_wireguard_disconnect(&(this->wg_ctx_));
    }
}

void Wireguard::set_address(const std::string& address) { this->address_ = std::move(address); }
void Wireguard::set_netmask(const std::string& netmask) { this->netmask_ = std::move(netmask); }
void Wireguard::set_private_key(const std::string& key) { this->private_key_ = std::move(key); }
void Wireguard::set_peer_endpoint(const std::string& endpoint) { this->peer_endpoint_ = std::move(endpoint); }
void Wireguard::set_peer_public_key(const std::string& key) { this->peer_public_key_ = std::move(key); }
void Wireguard::set_peer_port(const uint16_t port) { this->peer_port_ = port; }
void Wireguard::set_preshared_key(const std::string& key) { this->preshared_key_ = std::move(key); }

void Wireguard::set_keepalive(const uint16_t seconds) { this->keepalive_ = seconds; }
void Wireguard::set_srctime(time::RealTimeClock* srctime) { this->srctime_ = srctime; }

void Wireguard::start_connection_() {
    if(this->wg_initialized_ != ESP_OK) {
        ESP_LOGE(TAG, "cannot start connection, initialization in error with code %d", this->wg_initialized_);
        return;
    }

    if(!this->srctime_->now().is_valid()) {
        ESP_LOGI(TAG, "waiting for system time to be synchronized");
        return;
    }

    if(this->wg_connected_ == ESP_OK) {
        ESP_LOGD(TAG, "connection already started");
        return;
    }

    ESP_LOGD(TAG, "connecting...");
    this->wg_connected_ = esp_wireguard_connect(&(this->wg_ctx_));
    if(this->wg_connected_ == ESP_OK) {
        ESP_LOGI(TAG, "connection started");
    } else {
        ESP_LOGW(TAG, "cannot start connection, error code %d", this->wg_connected_);
    }
}

}  // namespace wireguard
}  // namespace esphome

// vim: tabstop=4 shiftwidth=4 expandtab

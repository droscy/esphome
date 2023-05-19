# Wireguard component
Allows connecting esphome devices to VPN managed by [WireGuard](https://www.wireguard.com/)

```yaml
# Setup a time source.
# Do not use 'homeassistant' platform if Home Assistant is on the remote
# peer because the time synchronization is a prerequisite to establish
# the vpn link.
time:
  - platform: sntp

# Setup WireGuard
wireguard:
  address: x.y.z.w
  private_key: private_key=
  peer_endpoint: wg.server.example
  peer_public_key: public_key=

  # optional netmask (this is the default if omitted)
  netmask: 255.255.255.255

  # optional custom port (this is the wireguard default)
  peer_port: 51820

  # optional pre-shared key
  peer_preshared_key: shared_key=

  # optional keepalive in seconds (disabled by default)
  peer_persistent_keepalive: 25

  # optional list of allowed ip/mask (default to any host)
  peer_allowed_ips:
    - x.y.z.0/24
    - l.m.n.o/32  # the /32 can be omitted for single host
    - [...]
```

## Sensors

If you give an id to the wireguard component you can manually create a binary
sensor to check if remote peer is online:

```yaml
wireguard:
  id: vpn
  [...]

binary_sensor:
  - platform: template
    name: 'WireGuard Status'
    device_class: connectivity
    lambda: |-
      return id(vpn).is_peer_up();
```

The `wireguard_handshake` sensor can be used to track the timestamp of the
latest completed handshake:

```yaml
sensor:
  - platform: wireguard_handshake
    name: 'WireGuard Latest Handshake'

    # optional (default to 60s)
    update_interval: 60s
```

from homeassistant.components.fan import FanEntity, FanEntityFeature
from homeassistant.config_entries import ConfigEntry
from homeassistant.helpers.device_registry import DeviceInfo
from homeassistant.core import HomeAssistant
from homeassistant.exceptions import HomeAssistantError, ServiceValidationError

import requests

from .const import DOMAIN

async def async_setup_entry(hass: HomeAssistant, entry: ConfigEntry, async_add_entities):
    """Set up the swamp cooler fan from a config entry."""
    ip_address = entry.data["ip_address"]
    async_add_entities([SwampCoolerFan(entry.entry_id, hass, ip_address)])

    return True

class SwampCoolerFan(FanEntity):
    def __init__(self, id, hass: HomeAssistant, ip_address: str):
        self._id = id
        self._hass = hass
        self._ip_address = ip_address
        self.supported_features = FanEntityFeature.PRESET_MODE | FanEntityFeature.TURN_OFF | FanEntityFeature.TURN_ON
        self.preset_modes = ["OFF", "LO", "HI"]

        self._state = None

        self.update()

    @property
    def name(self):
        return "Swamp Cooler Fan"
    
    @property
    def unique_id(self):
        return self._id
    
    @property
    def device_info(self):
        return DeviceInfo(
            identifiers={
                (DOMAIN, self.unique_id)
            },
            name = "Swamp Cooler"
        )

    @property
    def is_on(self):
        return self._state != "OFF"

    @property
    def speed(self):
        return self._state

    def set_preset_mode(self, preset_mode: str) -> None:
        if preset_mode not in self.preset_modes:
            raise ServiceValidationError(f"Fan mode {preset_mode} invalid")

        self.set_state(preset_mode)

    def turn_on(self, **kwargs):
        self.set_state("LO")

    def turn_off(self, **kwargs):
        self.set_state("OFF")

    def set_state(self, state: str):
        try:
            data = {
                "fan": state.lower()
            }
            
            requests.patch(f"http://{self._ip_address}/api/cooler", data)
            self._state = state
        except requests.RequestException as e:
            raise HomeAssistantError() from e

    def update(self):
        try:
            response = requests.get(f"http://{self._ip_address}/api/cooler").json()
            self._state = response.get("fan").upper()
        except requests.RequestException as e:
            raise HomeAssistantError() from e

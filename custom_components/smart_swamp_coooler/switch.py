from typing import Any
from homeassistant.components.switch import SwitchEntity
from homeassistant.config_entries import ConfigEntry
from homeassistant.core import HomeAssistant, HomeAssistantError
from homeassistant.helpers.device_registry import DeviceInfo

import requests

from .const import DOMAIN

async def async_setup_entry(hass: HomeAssistant, entry: ConfigEntry, async_add_entities):
    ip_address = entry.data["ip_address"]
    async_add_entities([SwampCoolerPump(entry.entry_id, hass, ip_address)])

    return True

class SwampCoolerPump(SwitchEntity):
    def __init__(self, id, hass: HomeAssistant, ip_address: str):
        self._id = id
        self._hass = hass
        self._ip_address = ip_address

        self._state = None

    @property
    def name(self):
        return "Swamp Cooler Pump"
    
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
        return self._state
    
    def turn_on(self):
        self.set_state(True)
    
    def turn_off(self):
        self.set_state(False)

    def set_state(self, state: bool):
        try:
            data = {
                "pump": "on" if state else "off"
            }

            requests.patch(f"http://{self._ip_address}/api/cooler", data)
        except requests.RequestException as e:
            raise HomeAssistantError from e

    def update(self):
        try:
            response = requests.get(f"http://{self._ip_address}/api/cooler").json()
            pump_state = response.get("pump")
            self._state = pump_state == "ON"
        except requests.RequestException as e:
            raise HomeAssistantError from e
    
    async def async_added_to_hass(self) -> None:
        # force an update so we get the correct entity state immediately
        await self.async_update_ha_state(force_refresh=True)

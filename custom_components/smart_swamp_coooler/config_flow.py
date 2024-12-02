import voluptuous as vol
from homeassistant import config_entries
from homeassistant.const import CONF_IP_ADDRESS

from .const import DOMAIN

class SwampCoolerConfigFlow(config_entries.ConfigFlow, domain=DOMAIN):
    
    VERSION = 1

    async def async_step_user(self, user_input=None):
        errors = {}

        # get input from user
        if user_input is None:
            return self.async_show_form(
                data_schema=vol.Schema({
                    vol.Required(CONF_IP_ADDRESS): str,
                }),
                errors=errors
            )

        return self.async_create_entry(title=user_input[CONF_IP_ADDRESS], data=user_input)
    
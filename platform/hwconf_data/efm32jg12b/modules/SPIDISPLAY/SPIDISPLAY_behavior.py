from . import ExporterModel
from . import SPIDISPLAY_model
from . import RuntimeModel


class SPIDISPLAY(ExporterModel.Module):
    def __init__(self, name=None):
        if not name:
            name = self.__class__.__name__
        super(SPIDISPLAY, self).__init__(name, visible=True)
        self.model = SPIDISPLAY_model

    def post_load(self):
        # Hide some properties by default
        self.get_property("HAL_SPIDISPLAY_EXTMODE_SPI").set_visibility(False)
        self.get_property("HAL_SPIDISPLAY_EXTCOMIN_CALLBACK").set_visibility(False)

        # Gide all extcomin prs options by default
        prs = self.get_property("BSP_SPIDISPLAY_EXTCOMIN_CHANNEL")
        prs.set_visibility(False)
        for key, value in prs.values.items():
            if value.value == "Disabled":
                self.get_property("prs_disabled_chn_SPIDISPLAY_EXTCOMIN_pin").set_visibility(False)
            else:
                self.get_property("BSP_SPIDISPLAY_EXTCOMIN{}".format(value.value.replace('CH',''))).set_visibility(False)

    def set_runtime_hooks(self):
        # EXTCOMIN use enable/disable
        RuntimeModel.set_change_handler(self.get_property("HAL_SPIDISPLAY_EXTMODE_EXTCOMIN"), SPIDISPLAY.extmode_changed)
        # EXTCOMIN prs enable/disable
        RuntimeModel.set_change_handler(self.get_property("HAL_SPIDISPLAY_EXTCOMIN_USE_PRS"), SPIDISPLAY.extmode_prs)
        # EXTCOMIN callback enable/disable
        RuntimeModel.set_change_handler(self.get_property("HAL_SPIDISPLAY_EXTCOMIN_USE_CALLBACK"), SPIDISPLAY.extmode_callback)

    @staticmethod
    def extmode_changed(studio_module, prop, state):
        """
        Use of EXTCOMIN was enabled or disabled
        :param studio_module:
        :param prop:
        :return:
        """
        if RuntimeModel.get_property_value(prop, module=studio_module) == '0':
            # EXTCOMIN use is disabled, show option for SPI EXTMODE
            RuntimeModel.set_property_hidden("HAL_SPIDISPLAY_EXTMODE_SPI", False, module=studio_module, state=state)
            # Hide all EXTCOMIN options
            RuntimeModel.set_property_hidden("HAL_SPIDISPLAY_EXTCOMIN_USE_CALLBACK", True, module=studio_module, state=state)
            SPIDISPLAY.extmode_callback(studio_module, None, state, hide=True)
        else:
            # EXTCOMIN use is enabled, hide option for SPI EXTMODE
            RuntimeModel.set_property_hidden("HAL_SPIDISPLAY_EXTMODE_SPI", True, module=studio_module, state=state)
            # Show EXTCOMIN options
            RuntimeModel.set_property_hidden("HAL_SPIDISPLAY_EXTCOMIN_USE_CALLBACK", False, module=studio_module, state=state)
            SPIDISPLAY.extmode_callback(studio_module, None, state, hide=False)

    @staticmethod
    def hide_extcomin_prs(studio_module, hide, state):
        """
        Hide or show the EXTCOMIN PRS settings
        :param studio_module:
        :param hide:
        :return:
        """
        RuntimeModel.set_property_hidden("BSP_SPIDISPLAY_EXTCOMIN_CHANNEL",
                                         hide,
                                         module=studio_module,
                                         state=state)
        current_prs_channel = RuntimeModel.get_property_value('BSP_SPIDISPLAY_EXTCOMIN_CHANNEL', module=studio_module)
        for p in RuntimeModel.get_property_values('BSP_SPIDISPLAY_EXTCOMIN_CHANNEL', module=studio_module):
            if hide:
                hidden = True
            else:
                hidden = p != current_prs_channel
            if p == "Disabled":
                RuntimeModel.set_property_hidden("prs_disabled_chn_SPIDISPLAY_EXTCOMIN_pin",
                                                 hidden,
                                                 module=studio_module,
                                                 state=state)
            else:
                RuntimeModel.set_property_hidden("BSP_SPIDISPLAY_EXTCOMIN{}".format(p),
                                                 hidden,
                                                 module=studio_module,
                                                 state=state)

    @staticmethod
    def extmode_prs(studio_module, prop, state, hide=False):
        """
        Configure EXTCOMIN PRS -- show PRS settings if PRS autotoggle is enabled
        :param studio_module:
        :param prop:
        :param hide: Override PRS autotoggle setting (used for hiding everything if "parent" property gets hidden)
        :return:
        """
        if not prop:
            prop = RuntimeModel.get_property_model("HAL_SPIDISPLAY_EXTCOMIN_USE_PRS", module=studio_module)
        RuntimeModel.set_property_hidden("HAL_SPIDISPLAY_EXTCOMIN_USE_PRS", hide, module=studio_module, state=state)
        if hide or RuntimeModel.get_property_value(prop, module=studio_module) == '0':
            RuntimeModel.set_property_hidden("BSP_SPIDISPLAY_EXTCOMIN", hide, module=studio_module, state=state)
            SPIDISPLAY.hide_extcomin_prs(studio_module, True, state)
        else:
            RuntimeModel.set_property_hidden("BSP_SPIDISPLAY_EXTCOMIN", True, module=studio_module, state=state)
            SPIDISPLAY.hide_extcomin_prs(studio_module, False, state)

    @staticmethod
    def extmode_callback(studio_module, prop, state, hide=False):
        """
        Configure EXTCOMIN callback function. Show EXTCOMIN PRS settings if callback is disabled
        :param studio_module:
        :param prop:
        :param hide: Override setting, hide everything
        :return:
        """
        if not prop:
            prop = RuntimeModel.get_property_model("HAL_SPIDISPLAY_EXTCOMIN_USE_CALLBACK", module=studio_module)
        if not hide and RuntimeModel.get_property_value(prop, module=studio_module) == '1':
            RuntimeModel.set_property_hidden("HAL_SPIDISPLAY_EXTCOMIN_CALLBACK", hide, module=studio_module, state=state)
            SPIDISPLAY.extmode_prs(studio_module, None, state, hide=True)
        else:
            RuntimeModel.set_property_hidden("HAL_SPIDISPLAY_EXTCOMIN_CALLBACK", True, module=studio_module, state=state)
            SPIDISPLAY.extmode_prs(studio_module, None, state, hide=hide)

def get_prs_chan_count(family_name=""):
        """
        Returns the number of available PRS channels for the given family
            :param family_name: string representation of the family name
        :return: integer representing the number of available PRS channels
        """

        return 12

def get_prs_chan_with_gpio_count(family_name=""):
        """
        Returns the number of available PRS channels for the given family
            :param family_name: string representation of the family name
        :return: integer representing the number of available PRS channels
        """

        return 4

available_modules_for_family = [
    "USB",
    "CMU",
    "LESENSE",
    "USARTRF0",
    "USART1",
    "USART2",
    "UART0",
    "UART1",
    "TIMER0",
    "TIMER1",
    "TIMER2",
    "TIMER3",
    "ACMP0",
    "ACMP1",
    "LEUART0",
    "LEUART1",
    "LETIMER0",
    "PCNT0",
    "PCNT1",
    "PCNT2",
    "I2C0",
    "I2C1",
    "PRS",
    "GPIO",
    "MSC",
    "ADC0",
    "DAC0",
    "LCD",
    "LFXO",
]

em4_pin_to_loc = {
    "PA0": {
        "number": 0,
        "define": "GPIO_EM4WUEN_EM4WUEN_A0",
    },
    "PF1": {
        "number": 3,
        "define": "GPIO_EM4WUEN_EM4WUEN_F1",
    },
    "PF2": {
        "number": 4,
        "define": "GPIO_EM4WUEN_EM4WUEN_F2",
    },
    "PE13": {
        "number": 5,
        "define": "GPIO_EM4WUEN_EM4WUEN_E13",
    },
}

stacked_flash = {
}

def allowed_route_conflicts(route):
    allowed_conflicts = {
     "BSP_BTL_BUTTON": ['BSP_LED', 'BSP_BUTTON'],
     "BSP_BUTTON_COUNT": ['BSP_LED', 'BSP_BTL_BUTTON'],
     "BSP_BUTTON0": ['BSP_LED', 'BSP_BTL_BUTTON'],
     "BSP_BUTTON1": ['BSP_LED', 'BSP_BTL_BUTTON'],
     "BSP_BUTTON2": ['BSP_LED', 'BSP_BTL_BUTTON'],
     "BSP_BUTTON3": ['BSP_LED', 'BSP_BTL_BUTTON'],
     "BSP_BUTTON4": ['BSP_LED', 'BSP_BTL_BUTTON'],
     "BSP_BUTTON5": ['BSP_LED', 'BSP_BTL_BUTTON'],
     "BSP_BUTTON6": ['BSP_LED', 'BSP_BTL_BUTTON'],
     "BSP_BUTTON7": ['BSP_LED', 'BSP_BTL_BUTTON'],
     "BSP_LED_COUNT": ['BSP_BUTTON', 'BSP_BTL_BUTTON'],
     "BSP_LED0": ['BSP_BUTTON', 'BSP_BTL_BUTTON'],
     "BSP_LED1": ['BSP_BUTTON', 'BSP_BTL_BUTTON'],
     "BSP_LED2": ['BSP_BUTTON', 'BSP_BTL_BUTTON'],
     "BSP_LED3": ['BSP_BUTTON', 'BSP_BTL_BUTTON'],
     "BSP_LED4": ['BSP_BUTTON', 'BSP_BTL_BUTTON'],
     "BSP_LED5": ['BSP_BUTTON', 'BSP_BTL_BUTTON'],
     "BSP_LED6": ['BSP_BUTTON', 'BSP_BTL_BUTTON'],
     "BSP_LED7": ['BSP_BUTTON', 'BSP_BTL_BUTTON'],
     "BSP_SPIDISPLAY_EXTCOMIN": ['PRS_CH'],
    }

    return allowed_conflicts.get(route, [])
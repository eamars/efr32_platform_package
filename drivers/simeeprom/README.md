# Simulated EEPROM
`simeeprom` stripped out unnecessary hardware abstraction layer binary from Silab's code base. 
It provides a clean and portable way to access FLASH based Simulated EEPROM for Wireless Guard
applications. 

Tokens
------
The application is built based on a wear-leveling algorithm provided by Silab. Data are accessed
adn stored via token. The token is associated to the size of the application at compile time. To
use EEPROM, you need to do some preparations. 

Customized tokens must be defined and declared in the header specified by `APPLICATION_TOKEN_HEADER`.
Following snippiest shows how a simple structure can be stored. For more information please refer to
[UG103.7](https://www.silabs.com/documents/public/user-guides/UG103-07-AppDevFundamentals-Tokens.pdf).
```C
/**
* Custom Application Tokens
*/
// Define token names here
#define CREATOR_IMU_PERESISTENT_DATA (0x0000)

#ifdef DEFINETYPES
// Include or define any typedef for tokens here
typedef struct
{
    uint16_t x;
    uint16_t y;
    uint16_t w;
    uint16_t t;
} axis_t;

#endif //DEFINETYPES


#ifdef DEFINETOKENS
// Define the actual token storage information here
DEFINE_BASIC_TOKEN(IMU_PERESISTENT_DATA, axis_t, {0})

#endif //DEFINETOKENS
```

Bookkeeping
-----------
`simeeprom_bookkeeping()` need to be called frequently to erase/free the unused data. 

Validation
----------
The `simeeprom` module do not validate the user data. Including a CRC byte in the data structure
might be a good idea. 

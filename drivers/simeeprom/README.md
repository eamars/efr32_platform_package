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
```c
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

Usage
-----
All token related storage/names are generated at compile time inside `simeeprom` module. Only token names 
and sizes are exposed externally. By including `simeeprom.h` you will be given the access to `TOKEN
<name>` and `TOKEN_<name>_SIZE`. Following example will give a brief overview of how Token system works. 

- Name: `IMU_PERESISTENT_DATA`.
- Creator: `CREATOR_IMU_PERESISTENT_DATA`, used to create unique token id.
- Token Name: `TOKEN_PERESISTENT_DATA`. Used to access tokens with `simeeprom` API.
- Token Size: `TOKEN_PERESISTENT_DATA_SIZE`. Essentially the sizeof the struct you defined. 

To write to the token you can call:

    simeeprom_write(&simeeprom, TOKEN_PERESISTENT_DATA, &data, TOKEN_PERESISTENT_DATA_SIZE);
    
To retrieve data, you can:
    
    simeeprom_read(&simeeprom, TOKEN_PERESISTENT_DATA, &data, TOKEN_PERESISTENT_DATA_SIZE);

Bookkeeping
-----------
`simeeprom_bookkeeping()` need to be called frequently to erase/free the unused data. 

Validation
----------
The `simeeprom` module do not validate the user data. Including a CRC byte in the data structure
might be a good idea. 

Note
----
If you have encountered tones of undefined error, you might need to add following header search
path to your main `CMakeLists.txt`:

```cmake
INCLUDE_DIRECTORIES(
		${TARGET_ROOT_DIRECTORY}/protocol/flex_2.0/connect/plugins
		${TARGET_ROOT_DIRECTORY}/platform/base/hal/plugin/sim-eeprom
)
```

/**
 * Custom Application Tokens
 */

#ifdef EMBER_AF_PLUGIN_SCENES_USE_TOKENS

#define CREATOR_SCENES_NUM_ENTRIES (0x8723)

#define CREATOR_SCENES_TABLE (0x8724)

#ifdef DEFINETYPES
// Include or define any typedef for tokens here
#endif //DEFINETYPES
#ifdef DEFINETOKENS
// Define the actual token storage information here

DEFINE_BASIC_TOKEN(SCENES_NUM_ENTRIES, uint8_t, 0x00)
DEFINE_INDEXED_TOKEN(SCENES_TABLE,
                     EmberAfSceneTableEntry,
                     EMBER_AF_PLUGIN_SCENES_TABLE_SIZE,
                     { EMBER_AF_SCENE_TABLE_UNUSED_ENDPOINT_ID })
#endif //DEFINETOKENS

#endif // EMBER_AF_PLUGIN_SCENES_USE_TOKENS

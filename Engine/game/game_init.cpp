//=============================================================================
//
// Adventure Game Studio (AGS)
//
// Copyright (C) 1999-2011 Chris Jones and 2011-20xx others
// The full list of copyright holders can be found in the Copyright.txt
// file, which is part of this source code distribution.
//
// The AGS source code is provided under the Artistic License 2.0.
// A copy of this license can be found in the file License.txt and at
// http://www.opensource.org/licenses/artistic-license-2.0.php
//
//=============================================================================

#include "ac/character.h"
#include "ac/charactercache.h"
#include "ac/draw.h"
#include "ac/file.h"
#include "ac/game.h"
#include "ac/gamesetup.h"
#include "ac/gamesetupstruct.h"
#include "ac/gamestate.h"
#include "ac/gui.h"
#include "ac/movelist.h"
#include "ac/dynobj/all_dynamicclasses.h"
#include "ac/dynobj/all_scriptclasses.h"
#include "ac/statobj/agsstaticobject.h"
#include "ac/statobj/staticarray.h"
#include "debug/debug_log.h"
#include "debug/out.h"
#include "font/fonts.h"
#include "game/game_init.h"
#include "gfx/bitmap.h"
#include "gfx/ddb.h"
#include "gui/guilabel.h"
#include "media/audio/audio.h"
#include "plugin/agsplugin.h"
#include "script/cc_error.h"
#include "script/exports.h"
#include "script/script.h"
#include "script/script_runtime.h"
#include "util/string_utils.h"

using namespace Common;
using namespace Engine;

extern GameSetupStruct game;
extern int actSpsCount;
extern Bitmap **actsps;
extern IDriverDependantBitmap* *actspsbmp;
extern Bitmap **actspswb;
extern IDriverDependantBitmap* *actspswbbmp;
extern CachedActSpsData* actspswbcache;
extern CharacterCache *charcache;

extern CCGUIObject ccDynamicGUIObject;
extern CCCharacter ccDynamicCharacter;
extern CCHotspot   ccDynamicHotspot;
extern CCRegion    ccDynamicRegion;
extern CCInventory ccDynamicInv;
extern CCGUI       ccDynamicGUI;
extern CCObject    ccDynamicObject;
extern CCDialog    ccDynamicDialog;
extern CCAudioChannel ccDynamicAudio;
extern CCAudioClip ccDynamicAudioClip;
extern ScriptString myScriptStringImpl;
extern ScriptObject scrObj[MAX_INIT_SPR];
extern ScriptGUI    *scrGui;
extern ScriptHotspot scrHotspot[MAX_HOTSPOTS];
extern ScriptRegion scrRegion[MAX_REGIONS];
extern ScriptInvItem scrInv[MAX_INV];
extern ScriptDialog scrDialog[MAX_DIALOG];
extern ScriptAudioChannel scrAudioChannel[MAX_SOUND_CHANNELS + 1];

extern ScriptDialogOptionsRendering ccDialogOptionsRendering;
extern ScriptDrawingSurface* dialogOptionsRenderingSurface;

extern AGSStaticObject GlobalStaticManager;

extern StaticArray StaticCharacterArray;
extern StaticArray StaticObjectArray;
extern StaticArray StaticGUIArray;
extern StaticArray StaticHotspotArray;
extern StaticArray StaticRegionArray;
extern StaticArray StaticInventoryArray;
extern StaticArray StaticDialogArray;

extern std::vector<ccInstance *> moduleInst;
extern std::vector<ccInstance *> moduleInstFork;
extern std::vector<RuntimeScriptValue> moduleRepExecAddr;

// Old dialog support (defined in ac/dialog)
extern std::vector< stdtr1compat::shared_ptr<unsigned char> > old_dialog_scripts;
extern std::vector<String> old_speech_lines;

StaticArray StaticCharacterArray;
StaticArray StaticObjectArray;
StaticArray StaticGUIArray;
StaticArray StaticHotspotArray;
StaticArray StaticRegionArray;
StaticArray StaticInventoryArray;
StaticArray StaticDialogArray;


namespace AGS
{
namespace Engine
{

String GetGameInitErrorText(GameInitError err)
{
    switch (err)
    {
    case kGameInitErr_NoError:
        return "No error";
    case kGameInitErr_NoFonts:
        return "No fonts specified to be used in this game";
    case kGameInitErr_TooManyAudioTypes:
        return "Too many audio types for this engine to handle";
    case kGameInitErr_TooManyPlugins:
        return "Too many plugins for this engine to handle";
    case kGameInitErr_PluginNameInvalid:
        return "Plugin name is invalid";
    case kGameInitErr_ScriptLinkFailed:
        return String::FromFormat("Script link failed: %s", ccErrorString);
    }
    return "Unknown error";
}

// Initializes audio channels and clips and registers them in the script system
void InitAndRegisterAudioObjects()
{
    for (int i = 0; i <= MAX_SOUND_CHANNELS; ++i) 
    {
        scrAudioChannel[i].id = i;
        ccRegisterManagedObject(&scrAudioChannel[i], &ccDynamicAudio);
    }

    for (int i = 0; i < game.audioClipCount; ++i)
    {
        game.audioClips[i].id = i;
        ccRegisterManagedObject(&game.audioClips[i], &ccDynamicAudioClip);
        ccAddExternalDynamicObject(game.audioClips[i].scriptName, &game.audioClips[i], &ccDynamicAudioClip);
    }
}

// Initializes characters and registers them in the script system
void InitAndRegisterCharacters()
{
    characterScriptObjNames = (char**)malloc(sizeof(char*) * game.numcharacters);

    for (int i = 0; i < game.numcharacters; ++i)
    {
        game.chars[i].walking = 0;
        game.chars[i].animating = 0;
        game.chars[i].pic_xoffs = 0;
        game.chars[i].pic_yoffs = 0;
        game.chars[i].blinkinterval = 140;
        game.chars[i].blinktimer = game.chars[i].blinkinterval;
        game.chars[i].index_id = i;
        game.chars[i].blocking_width = 0;
        game.chars[i].blocking_height = 0;
        game.chars[i].prevroom = -1;
        game.chars[i].loop = 0;
        game.chars[i].frame = 0;
        game.chars[i].walkwait = -1;
        ccRegisterManagedObject(&game.chars[i], &ccDynamicCharacter);

        // export the character's script object
        characterScriptObjNames[i] = strdup(game.chars[i].scrname);
        ccAddExternalDynamicObject(characterScriptObjNames[i], &game.chars[i], &ccDynamicCharacter);
    }
}

// Initializes dialog and registers them in the script system
void InitAndRegisterDialogs()
{
    for (int i = 0; i < game.numdialog; ++i)
    {
        scrDialog[i].id = i;
        scrDialog[i].reserved = 0;
        ccRegisterManagedObject(&scrDialog[i], &ccDynamicDialog);

        if (game.dialogScriptNames[i][0] != 0)
            ccAddExternalDynamicObject(game.dialogScriptNames[i], &scrDialog[i], &ccDynamicDialog);
    }
}

// Initializes dialog options rendering objects and registers them in the script system
void InitAndRegisterDialogOptions()
{
    ccRegisterManagedObject(&ccDialogOptionsRendering, &ccDialogOptionsRendering);

    dialogOptionsRenderingSurface = new ScriptDrawingSurface();
    dialogOptionsRenderingSurface->isLinkedBitmapOnly = true;
    long dorsHandle = ccRegisterManagedObject(dialogOptionsRenderingSurface, dialogOptionsRenderingSurface);
    ccAddObjectReference(dorsHandle);
}

// Initializes gui and registers them in the script system
void InitAndRegisterGUI()
{
    scrGui = (ScriptGUI*)malloc(sizeof(ScriptGUI) * game.numgui);
    for (int i = 0; i < game.numgui; ++i)
    {
        scrGui[i].id = -1;
    }

    guiScriptObjNames = (char**)malloc(sizeof(char*) * game.numgui);
    for (int i = 0; i < game.numgui; ++i)
    {
        guis[i].RebuildArray();
        if ((guis[i].PopupStyle == kGUIPopupNone) || (guis[i].PopupStyle == kGUIPopupNoAutoRemove))
            guis[i].SetVisibility(kGUIVisibility_On);
        else
            guis[i].SetVisibility(kGUIVisibility_Off);

        // export all the GUI's controls
        export_gui_controls(i);
        // copy the script name to its own memory location
        // because ccAddExtSymbol only keeps a reference
        guiScriptObjNames[i] = strdup(guis[i].Name);
        scrGui[i].id = i;
        ccAddExternalDynamicObject(guiScriptObjNames[i], &scrGui[i], &ccDynamicGUI);
        ccRegisterManagedObject(&scrGui[i], &ccDynamicGUI);
    }
}

// Initializes inventory items and registers them in the script system
void InitAndRegisterInvItems()
{
    for (int i = 0; i < MAX_INV; ++i)
    {
        scrInv[i].id = i;
        scrInv[i].reserved = 0;
        ccRegisterManagedObject(&scrInv[i], &ccDynamicInv);

        if (game.invScriptNames[i][0] != 0)
            ccAddExternalDynamicObject(game.invScriptNames[i], &scrInv[i], &ccDynamicInv);
    }
}

// Initializes room hotspots and registers them in the script system
void InitAndRegisterHotspots()
{
    for (int i = 0; i < MAX_HOTSPOTS; ++i)
    {
        scrHotspot[i].id = i;
        scrHotspot[i].reserved = 0;
        ccRegisterManagedObject(&scrHotspot[i], &ccDynamicHotspot);
    }
}

// Initializes room objects and registers them in the script system
void InitAndRegisterRoomObjects()
{
    for (int i = 0; i < MAX_INIT_SPR; ++i)
    {
        ccRegisterManagedObject(&scrObj[i], &ccDynamicObject);
    }
}

// Initializes room regions and registers them in the script system
void InitAndRegisterRegions()
{
    for (int i = 0; i < MAX_REGIONS; ++i)
    {
        scrRegion[i].id = i;
        scrRegion[i].reserved = 0;
        ccRegisterManagedObject(&scrRegion[i], &ccDynamicRegion);
    }
}

// Registers static entity arrays in the script system
void RegisterStaticArrays()
{
    StaticCharacterArray.Create(&ccDynamicCharacter, sizeof(CharacterInfo), sizeof(CharacterInfo));
    StaticObjectArray.Create(&ccDynamicObject, sizeof(ScriptObject), sizeof(ScriptObject));
    StaticGUIArray.Create(&ccDynamicGUI, sizeof(ScriptGUI), sizeof(ScriptGUI));
    StaticHotspotArray.Create(&ccDynamicHotspot, sizeof(ScriptHotspot), sizeof(ScriptHotspot));
    StaticRegionArray.Create(&ccDynamicRegion, sizeof(ScriptRegion), sizeof(ScriptRegion));
    StaticInventoryArray.Create(&ccDynamicInv, sizeof(ScriptInvItem), sizeof(ScriptInvItem));
    StaticDialogArray.Create(&ccDynamicDialog, sizeof(ScriptDialog), sizeof(ScriptDialog));

    ccAddExternalStaticArray("character",&game.chars[0], &StaticCharacterArray);
    ccAddExternalStaticArray("object",&scrObj[0], &StaticObjectArray);
    ccAddExternalStaticArray("gui",&scrGui[0], &StaticGUIArray);
    ccAddExternalStaticArray("hotspot",&scrHotspot[0], &StaticHotspotArray);
    ccAddExternalStaticArray("region",&scrRegion[0], &StaticRegionArray);
    ccAddExternalStaticArray("inventory",&scrInv[0], &StaticInventoryArray);
    ccAddExternalStaticArray("dialog", &scrDialog[0], &StaticDialogArray);
}

// Initializes various game entities and registers them in the script system
void InitAndRegisterGameEntities()
{
    InitAndRegisterAudioObjects();
    InitAndRegisterCharacters();
    InitAndRegisterDialogs();
    InitAndRegisterDialogOptions();
    InitAndRegisterGUI();
    InitAndRegisterInvItems();

    InitAndRegisterHotspots();
    InitAndRegisterRegions();
    InitAndRegisterRoomObjects();

    RegisterStaticArrays();

    setup_player_character(game.playercharacter);
    if (loaded_game_file_version >= kGameVersion_270)
        ccAddExternalStaticObject("player", &_sc_PlayerCharPtr, &GlobalStaticManager);
}

void LoadFonts()
{
    for (int i = 0; i < game.numfonts; ++i) 
    {
        int fontsize = game.fontflags[i] & FFLG_SIZEMASK;
        if (fontsize == 0)
            fontsize = 8;

        if ((game.options[OPT_NOSCALEFNT] == 0) && game.IsHiRes())
            fontsize *= 2;

        if (!wloadfont_size(i, fontsize))
            quitprintf("Unable to load font %d, no renderer could load a matching file", i);
    }
}

void AllocScriptModules()
{
    moduleInst.resize(numScriptModules, NULL);
    moduleInstFork.resize(numScriptModules, NULL);
    moduleRepExecAddr.resize(numScriptModules);
    repExecAlways.moduleHasFunction.resize(numScriptModules, true);
    lateRepExecAlways.moduleHasFunction.resize(numScriptModules, true);
    getDialogOptionsDimensionsFunc.moduleHasFunction.resize(numScriptModules, true);
    renderDialogOptionsFunc.moduleHasFunction.resize(numScriptModules, true);
    getDialogOptionUnderCursorFunc.moduleHasFunction.resize(numScriptModules, true);
    runDialogOptionMouseClickHandlerFunc.moduleHasFunction.resize(numScriptModules, true);
    runDialogOptionKeyPressHandlerFunc.moduleHasFunction.resize(numScriptModules, true);
    runDialogOptionRepExecFunc.moduleHasFunction.resize(numScriptModules, true);
    for (int i = 0; i < numScriptModules; ++i)
    {
        moduleRepExecAddr[i].Invalidate();
    }
}

GameInitError InitGameState(const LoadedGameEntities &ents, GameDataVersion data_ver)
{
    //
    // 1. Check that the loaded data is valid and compatible with the current
    // engine capabilities.
    //
    if (game.numfonts == 0)
        return kGameInitErr_NoFonts;
    if (game.audioClipTypeCount > MAX_AUDIO_TYPES)
        return kGameInitErr_TooManyAudioTypes;

    //
    // 2. Apply overriding config settings
    //
    // The earlier versions of AGS provided support for "upscaling" low-res
    // games (320x200 and 320x240) to hi-res (640x400 and 640x480
    // respectively). The script API has means for detecting if the game is
    // running upscaled, and game developer could use this opportunity to setup
    // game accordingly (e.g. assign hi-res fonts, etc).
    // This feature is officially deprecated since 3.1.0, however the engine
    // itself still supports it, technically.
    // This overriding option re-enables "upscaling". It works ONLY for low-res
    // resolutions, such as 320x200 and 320x240.
    if (usetup.override_upscale)
    {
        if (game.GetDefaultResolution() == kGameResolution_320x200)
            game.SetDefaultResolution(kGameResolution_640x400);
        else if (game.GetDefaultResolution() == kGameResolution_320x240)
            game.SetDefaultResolution(kGameResolution_640x480);
    }

    //
    // 3. Allocate and init game objects
    //
    charextra = (CharacterExtras*)calloc(game.numcharacters, sizeof(CharacterExtras));
    charcache = (CharacterCache*)calloc(1,sizeof(CharacterCache)*game.numcharacters+5);
    mls = (MoveList*)calloc(game.numcharacters + MAX_INIT_SPR + 1, sizeof(MoveList));
    actSpsCount = game.numcharacters + MAX_INIT_SPR + 2;
    actsps = (Bitmap **)calloc(actSpsCount, sizeof(Bitmap *));
    actspsbmp = (IDriverDependantBitmap**)calloc(actSpsCount, sizeof(IDriverDependantBitmap*));
    actspswb = (Bitmap **)calloc(actSpsCount, sizeof(Bitmap *));
    actspswbbmp = (IDriverDependantBitmap**)calloc(actSpsCount, sizeof(IDriverDependantBitmap*));
    actspswbcache = (CachedActSpsData*)calloc(actSpsCount, sizeof(CachedActSpsData));
    play.charProps.resize(game.numcharacters);
    old_dialog_scripts = ents.OldDialogScripts;
    old_speech_lines = ents.OldSpeechLines;
    InitAndRegisterGameEntities();
    LoadFonts();

    //
    // 4. Create game scripts
    //
    gamescript = ents.GlobalScript;
    dialogScriptsScript = ents.DialogScript;
    numScriptModules = ents.ScriptModules.size();
    scriptModules = ents.ScriptModules;

    ccSetScriptAliveTimer(150000);
    ccSetStringClassImpl(&myScriptStringImpl);
    setup_script_exports();
    AllocScriptModules();
    if (create_global_script())
        return kGameInitErr_ScriptLinkFailed;

    //
    // 5. Initialize certain runtime variables
    //
    game_paused = 0;  // reset the game paused flag
    ifacepopped = -1;

    if (game.saveGameFileExtension[0] != 0)
        saveGameSuffix.Format(".%s", game.saveGameFileExtension);
    else
        saveGameSuffix = "";

    play.score_sound = game.scoreClipID;
    play.fade_effect = game.options[OPT_FADETYPE];

    //
    // 7. Initialize runtime state of certain game objects
    //
    for (int i = 0; i < numguilabels; ++i)
    {
        // labels are not clickable by default
        guilabels[i].SetClickable(false);
    }
    play.gui_draw_order = (int*)calloc(game.numgui * sizeof(int), 1);
    update_gui_zorder();
    calculate_reserved_channel_count();

    //
    // 8. Start up plugins
    //
    pl_register_plugins(ents.PluginInfos);
    pl_startup_plugins();
    return kGameInitErr_NoError;
}

} // namespace Engine
} // namespace AGS

// RINGRACERS-NOIRE
//-----------------------------------------------------------------------------
// Copyright (C) 2024 by NepDisk
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------

#ifndef __N_MENU__
#define __N_MENU__

#include "../k_menu.h"
#include "../m_cond.h"
#include "../command.h"
#include "../console.h"
#include "../g_state.h" //For the tripwire toggle

// Noire
#include "n_cvar.h"

#ifdef __cplusplus
extern "C" {
#endif

extern menuitem_t OPTIONS_NoireGameplay[];
extern menuitem_t OPTIONS_NoireGameplayRings[];
extern menuitem_t OPTIONS_NoireGameplayItems[];
extern menuitem_t OPTIONS_NoireGameplayMechanics[];
extern menuitem_t OPTIONS_NoireGameplayInstawhip[];
extern menuitem_t OPTIONS_NoireGameplaySpindash[];
extern menuitem_t OPTIONS_NoireGameplayDriving[];
extern menuitem_t OPTIONS_NoireGameplayBots[];
extern menuitem_t OPTIONS_NoireGameplayRivals[];
extern menu_t OPTIONS_NoireGameplayDef;
extern menu_t OPTIONS_NoireGameplayRingsDef;
extern menu_t OPTIONS_NoireGameplayItemsDef;
extern menu_t OPTIONS_NoireGameplayMechanicsDef;
extern menu_t OPTIONS_NoireGameplayInstawhipDef;
extern menu_t OPTIONS_NoireGameplaySpindashDef;
extern menu_t OPTIONS_NoireGameplayDrivingDef;
extern menu_t OPTIONS_NoireGameplayBotsDef;
extern menu_t OPTIONS_NoireGameplayRivalsDef;

extern menuitem_t OPTIONS_Noire[];
extern menu_t OPTIONS_NoireDef;

//Character Menu stuff
//Defs
extern menu_t PLAY_CharSelect1PDef;
void M_Character1PSelect(INT32);
void M_DrawCharacter1PSelect(void);
void M_Character1PSelectTick(void);
void M_Character1PSelectInit(void);
boolean M_Character1PSelectQuit(void);
boolean M_Character1PSelectHandler(INT32);

//Functions:
void M_ResetDrawingList(void);
UINT8 M_GetSkinIndexGivenPos(setup_player_t* p);

#define CHARSEL_MAX_COLUMNS 9

typedef enum setup_sortMode {
    DEFAULT_ID = 1,
    NAME,
    REALNAME,
    PREFCOLOR,
    WEIGHT,
    SPEED,
    ENGINECLASS
} setup_sortMode_e;

typedef struct setup_clonelist {
    UINT8 numClones;                    // Amount of items in cloneIds
    UINT8 *cloneIds;                    //Skin IDs that will be hold in this instance. Needs to be dynamically allocated!
} setup_clonelist_s;

typedef union parentclone {
    setup_clonelist_s *clones;         //Only used if this is a parent
    UINT8 parentID;                     //Only used if this is a clone
} parentclone_u;

typedef struct parentorclone {
    boolean isParent;
    parentclone_u uniondata;
} parentorclone_s;

typedef struct setup_flatchargrid_t {
    setup_sortMode_e sortingMode; 		// How we are currently sorting
	boolean isExtended; 				// Is SkinList expanded right now or not, showing clones as individual items outside of their parents.
    parentorclone_s *skinList;            // Skins that we'll have. Parallel list to skins, but it will be the same size as numskins. WILL NOT CONTAIN UNUSABLE SKINS
	UINT8 *drawingList;					// List of skinList indexes that we will draw. This will be resized and shuffled and whatever.
    UINT8 drawingListCount;
} setup_flatchargrid_s;

extern setup_flatchargrid_s setup_flatchargrid;

#ifdef __cplusplus
} // extern "C"
#endif

#endif

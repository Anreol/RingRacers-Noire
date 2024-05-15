// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2024 by Sally "TehRealSalt" Cochenour.
// Copyright (C) 2024 by "Lat'".
// Copyright (C) 2024 by Vivian "toastergrl" Grannell.
// Copyright (C) 2024 by Kart Krew.

// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  menus/play-char-select.c
/// \brief Character Select

#include "../../i_time.h"
#include "../../k_color.h"
#include "../../k_grandprix.h" // K_CanChangeRules
#include "../../k_menu.h"
#include "../../m_cond.h" // Condition Sets
#include "../../r_skins.h"
#include "../../s_sound.h"
#include "../../z_zone.h"
#include "../n_menu.h"


// #define CHARSELECT_DEVICEDEBUG

menuitem_t PLAY_CharSelect1P[] = {
	{IT_NOTHING, NULL, NULL, NULL, {NULL}, 0, 0},
};

static void M_DrawCharacter1PBack(void)
{
	if (!optionsmenu.profile)
	{
		if (gamestate == GS_MENU)
			M_DrawMenuBackground();
		return;
	}

	M_DrawOptionsCogs();
}

menu_t PLAY_CharSelect1PDef = {
	sizeof(PLAY_CharSelect1P) / sizeof(menuitem_t), // # of menu items
	&MainDef,										// previous menu
	0,												// last item user was on in menu
	PLAY_CharSelect1P,								// menu items
	0,
	0, // x, y of menu
	SKINCOLOR_SUPERORANGE5,
	0,	  // Can be whatever really! Options menu uses extra1 for bg colour.
	0,	  // menubehaviourflags_t
	NULL, // Track to play in M_PlayMenuJam. NULL for default, "." to stop
	2,
	5, // matches OPTIONS_EditProfileDef				// only transition if IDs match AND tics for transitions out
	M_DrawCharacter1PSelect,   // draw routine
	M_DrawCharacter1PBack,	   // draw routine, but, like, for the background
	M_Character1PSelectTick,   // ticker routine
	M_Character1PSelectInit,   // called when starting a new menu
	M_Character1PSelectQuit,   // called before quit a menu return true if we can
	M_Character1PSelectHandler // if set, called every frame in the input handler. Returning true overwrites normal
							   // input handling.
};

setup_flatchargrid_s setup_flatchargrid;

static void M_PushMenuColor(setup_player_colors_t* colors, UINT16 newColor)
{
	if (colors->listLen >= colors->listCap)
	{
		if (colors->listCap == 0)
		{
			colors->listCap = 64;
		}
		else
		{
			colors->listCap *= 2;
		}

		colors->list = Z_ReallocAlign(colors->list, sizeof(UINT16) * colors->listCap, PU_STATIC, NULL, sizeof(UINT16) * 8);
	}

	colors->list[colors->listLen] = newColor;
	colors->listLen++;
}

static void M_ClearMenuColors(setup_player_colors_t* colors)
{
	if (colors->list != NULL)
	{
		Z_Free(colors->list);
		colors->list = NULL;
	}

	colors->listLen = colors->listCap = 0;
}

/*
UINT16 M_GetColorAfter(setup_player_colors_t *colors, UINT16 value, INT32 amount)
{
	const INT32 sign = (amount < 0) ? -1 : 1;
	INT32 index = -1;
	size_t i = SIZE_MAX;

	if (amount == 0 || colors->listLen == 0)
	{
		return value;
	}

	for (i = 0; i < colors->listLen; i++)
	{
		if (colors->list[i] == value)
		{
			index = i;
			break;
		}
	}

	if (index == -1)
	{
		// Not in list, so no real correct behavior here.
		// Just try to get back to the list.
		return colors->list[0];
	}

	amount = abs(amount);

	while (amount > 0)
	{
		index += sign;

		if (index < 0)
		{
			index = colors->listLen - 1;
		}
		else if (index >= (INT32)colors->listLen)
		{
			index = 0;
		}

		amount--;
	}

	return colors->list[index];
}*/

// Given a skin ID and a player, put the player's gridx and gridy after finding that skin in setup_flatchargrid.drawingList
static void M_PositionPlayerInGrid(setup_player_t* p, UINT8 skin) {
    UINT8 skinIndex = 0;
    // Find the index of the skin in the drawing list
    for (UINT8 i = 0; i < CHARSEL_MAX_COLUMNS; i++) {
        if (setup_flatchargrid.drawingList[i] == skin) {
            skinIndex = i;
            break;
        }
    }

    // Calculate gridx and gridy
    p->gridx = skinIndex % CHARSEL_MAX_COLUMNS;
    p->gridy = skinIndex / CHARSEL_MAX_COLUMNS;
}

UINT8 M_GetSkinIndexGivenPos(setup_player_t* p) {
    // Calculate the index of the skin based on player's gridx and gridy
    UINT8 skinIndex = p->gridy * CHARSEL_MAX_COLUMNS + p->gridx;
	if(skinIndex >= setup_flatchargrid.drawingListCount) {
		// uh
		return setup_flatchargrid.drawingList[setup_flatchargrid.drawingListCount - 1];
	}
    return setup_flatchargrid.drawingList[skinIndex];
}

#define STRESSTESTGRID
//Reset the memory in drawingList, 
////set its size to UINT8 * numskins, then proceed to fill it to the indexes of then proceed to fill it to the indexes of setup_flatchargrid.skinList (which is a parallel list of skins, where the skin_t are) with the following rules:
//if setup_flatchargrid.isExtended is false, do not put in ids that are part of a parent in the array
//Then after filling it, sort depending on setup_nestedchar_s.sortingMode:
//	EFAULT_ID sort by their character id (index in skins[])
//	NAME sort by skin.name
//	REALNAME sort by skin.realname
//	PREFCOLOR sort by skin.prefcolor
//	WEIGHT sort by skin.kartweight
//	SPEED sort by skin.kartspeed
//	ENGINECLASS sort by calling R_GetEngineClass, and sort up to down (ENGINECLASS_A, ENGINECLASS_B...)
void M_ResetDrawingList(void){
    if (setup_flatchargrid.drawingList) {
        Z_Free(setup_flatchargrid.drawingList);
    }
    setup_flatchargrid.drawingList = NULL;
	setup_flatchargrid.drawingListCount = 0;

    setup_flatchargrid.drawingList = (UINT8 *)Z_Malloc(sizeof(UINT8) * numskins, PU_STATIC, NULL);
    for (UINT8 i = 0; i < numskins; i++) {
        if (setup_flatchargrid.isExtended && !setup_flatchargrid.skinList[i].isParent) {
			setup_flatchargrid.drawingList[setup_flatchargrid.drawingListCount] = setup_flatchargrid.skinList[i].uniondata.parentID;
        } else {
            setup_flatchargrid.drawingList[setup_flatchargrid.drawingListCount] = i;
        }
        setup_flatchargrid.drawingListCount++;
	}

	#ifdef STRESSTESTGRID
	// Reallocate drawingList to accommodate MAXSKINS
	setup_flatchargrid.drawingList = (UINT8 *)Z_Realloc(setup_flatchargrid.drawingList, MAXSKINS * sizeof(UINT8), PU_STATIC, NULL);

	// Fill the additional space with repeating skin indices
	for (UINT8 i = 0, sgs = 0; i < MAXSKINS - setup_flatchargrid.drawingListCount; i++) {
        setup_flatchargrid.drawingList[setup_flatchargrid.drawingListCount] = sgs;
        setup_flatchargrid.drawingListCount++;
        sgs++;
        if (sgs >= numskins) {
            sgs = 0;
        }
	}
	#endif
}

static void M_NewPlayerColors(setup_player_t* p)
{
	const boolean follower = (p->mdepth >= CSSTEP_FOLLOWER);
	INT32 i = INT32_MAX;

	M_ClearMenuColors(&p->colors);

	// Add all unlocked colors
	for (i = SKINCOLOR_NONE + 1; i < numskincolors; i++)
	{
		if (K_ColorUsable(i, follower, true) == true)
		{
			M_PushMenuColor(&p->colors, i);
		}
	}

	// Add special colors
	M_PushMenuColor(&p->colors, SKINCOLOR_NONE);

	if (follower == true)
	{
		// Add special follower colors
		M_PushMenuColor(&p->colors, FOLLOWERCOLOR_MATCH);
		M_PushMenuColor(&p->colors, FOLLOWERCOLOR_OPPOSITE);
	}
}

static INT16 M_GetMenuCategoryFromFollower(setup_player_t* p)
{
	if (p->followern < 0 || p->followern >= numfollowers || !K_FollowerUsable(p->followern))
		return -1;

	INT16 i;

	for (i = 0; i < setup_numfollowercategories; i++)
	{
		if (followers[p->followern].category != setup_followercategories[i][1])
			continue;

		break;
	}

	if (i >= setup_numfollowercategories)
		return -1;

	return i;
}

// sets up the grid pos for the skin used by the profile.
static void M_SetupProfileGridPos(setup_player_t* p)
{
	profile_t* pr = PR_GetProfile(p->profilen);
	INT32 skinId = R_SkinAvailableEx(pr->skinname, false);
	INT32 alt = 0; // Hey it's my character's name!

	if (skinId == -1)
		skinId = 0;

	// While we're here, read follower values.
	p->followern = K_FollowerAvailable(pr->follower);

	p->followercategory = M_GetMenuCategoryFromFollower(p);
	if (p->followercategory == -1) // unlock gate failed?
		p->followern = -1;

	p->followercolor = pr->followercolor;
	if (K_ColorUsable(p->followercolor, true, true) == false)
	{
		p->followercolor = SKINCOLOR_NONE;
	}

	if (!R_SkinUsable(g_localplayers[0], skinId, false))
	{
		skinId = GetSkinNumClosestToStats(skins[skinId].kartspeed, skins[skinId].kartweight, skins[skinId].flags, false);
	}

	INT32 parentSkinId = skinId;

	// Check if the grid is collapsed and check if this skinId we found is truly a parent
	if (setup_flatchargrid.isExtended == false && setup_flatchargrid.skinList[parentSkinId].isParent)
	{
		// If it is, we need to get the "depth" (child id) of it for p.clonenum
		for (size_t y = 0; y < setup_flatchargrid.skinList[parentSkinId].uniondata.clones->numClones; y++)
		{
			if (setup_flatchargrid.skinList[parentSkinId].uniondata.clones->cloneIds[y] == skinId)
			{
				alt = y; // asign the depth
				break;
			}
		}
	}

	// Now position the player's grid for skin
	M_PositionPlayerInGrid(p, parentSkinId);

	p->clonenum = alt;
	p->color = pr->color;

	if (K_ColorUsable(p->color, false, true) == false)
	{
		p->color = SKINCOLOR_NONE;
	}
}

static void M_SetupMidGameGridPos(setup_player_t* p, UINT8 num)
{
	INT32 skinId = R_SkinAvailableEx(cv_skin[num].zstring, false);
	INT32 alt = 0; // Hey it's my character's name!

	if (skinId == -1)
		skinId = 0;

	// While we're here, read follower values.
	p->followern = cv_follower[num].value;
	p->followercolor = cv_followercolor[num].value;

	p->followercategory = M_GetMenuCategoryFromFollower(p);
	if (p->followercategory == -1) // unlock gate failed?
		p->followern = -1;

	if (K_ColorUsable(p->followercolor, true, true) == false)
	{
		p->followercolor = SKINCOLOR_NONE;
	}

	if (!R_SkinUsable(g_localplayers[0], skinId, false))
	{
		skinId = GetSkinNumClosestToStats(skins[skinId].kartspeed, skins[skinId].kartweight, skins[skinId].flags, false);
	}

	INT32 parentSkinId = skinId;

	// Check if the grid is collapsed and check if this skinId we found is truly a parent
	if (setup_flatchargrid.isExtended == false && setup_flatchargrid.skinList[parentSkinId].isParent)
	{
		// If it is, we need to get the "depth" (child id) of it for p.clonenum
		for (size_t y = 0; y < setup_flatchargrid.skinList[parentSkinId].uniondata.clones->numClones; y++)
		{
			if (setup_flatchargrid.skinList[parentSkinId].uniondata.clones->cloneIds[y] == skinId)
			{
				alt = y; // asign the depth
				break;
			}
		}
	}

	// Now position the player's grid for skin
	M_PositionPlayerInGrid(p, parentSkinId);

	p->clonenum = alt;
	p->color = cv_playercolor[num].value;
}

/// Setup all the data that will be used in the character select
void M_Character1PSelectInit(void)
{
	UINT8 i, j;
	setup_maxpage = 0;

	memset(&setup_flatchargrid, -1, sizeof(setup_flatchargrid_s));
	setup_flatchargrid.sortingMode = 1;
	setup_flatchargrid.isExtended = false;
	setup_flatchargrid.skinList = Z_Malloc(sizeof(setup_clonelist_s) * numskins, PU_STATIC, NULL);

	memset(setup_player, 0, sizeof(setup_player));
	setup_numplayers = 0;

	memset(setup_explosions, 0, sizeof(setup_explosions));
	setup_animcounter = 0;

	for (i = 0; i < MAXSPLITSCREENPLAYERS; i++)
	{
		// Default to no follower / match colour.
		setup_player[i].followern = -1;
		setup_player[i].followercategory = -1;
		setup_player[i].followercolor = SKINCOLOR_NONE;

		setup_player[i].profilen_slide.start = 0;
		setup_player[i].profilen_slide.dist = 0;

		// If we're on prpfile select, skip straight to CSSTEP_CHARS
		// do the same if we're midgame, but make sure to consider splitscreen properly.
		if (optionsmenu.profile && i == 0)
		{
			setup_player[i].profilen = optionsmenu.profilen;
			// PR_ApplyProfileLight(setup_player[i].profilen, 0);
			M_SetupProfileGridPos(&setup_player[i]);
			setup_player[i].mdepth = CSSTEP_CHARS;
		}
		else
		{
			// Set default selected profile to the last used profile for each player:
			// (Make sure we don't overshoot it somehow if we deleted profiles or whatnot)
			setup_player[i].profilen = min(cv_lastprofile[i].value, PR_GetNumProfiles());

			if (gamestate != GS_MENU && i <= splitscreen)
			{
				M_SetupMidGameGridPos(&setup_player[i], i);
				setup_player[i].mdepth = CSSTEP_CHARS;
			}
			else
			{
				// Un-set devices
				G_SetDeviceForPlayer(i, -1);
#ifdef CHARSELECT_DEVICEDEBUG
				CONS_Printf("M_Character1PSelectInit: Device for %d set to %d\n", i, -1);
#endif
			}
		}
	}

	// Cycle through all skins... woah...
	for (i = 0; i < numskins; i++)
	{
		if (!R_SkinUsable(g_localplayers[0], i, false))
			continue; // SKIP

		int32_t parentNum;
		for (j = 0; j < SKINPARENTS; j++)
		{
			const char* parentName = skins[i].parentnames[j];
			parentNum = R_SkinAvailableEx(parentName, false); // See if the parent is available.

			if (parentNum == -1)
				continue; // Doesn't match! Continue.

			//if(skins[parentNum].parentnames)
			// Exit!
			break;
		}

		if (parentNum != -1) // We have a parent
		{
			// Check if the not defined yet, as its id might be ahead of us
			if (!&setup_flatchargrid.skinList[parentNum])
			{
				//We need to look up that skin, to see if it is a parent or not..
			}
			else //Parent is defined
			{
				parentclone_u *parentclone = &setup_flatchargrid.skinList[parentNum].uniondata;
				if(&parentclone->parentID != NULL) { //ParentID did get set, so this is a clone.
					//Figure out how to access THAT parent's parentclone, and repeat that until we find a parent's parentclone that does have a *clones. This requires recursion...
					continue;
				}

				if (parentclone->clones->cloneIds == NULL) { //This is a parent, but its clones didn't get alloc yet
					parentclone->clones->numClones++;
					parentclone->clones->cloneIds = Z_Malloc(sizeof(UINT8), PU_STATIC, NULL);
					parentclone->clones->cloneIds[0] = i;
				} else {
					parentclone->clones->numClones++;
					UINT8 *clones = parentclone->clones->cloneIds;
					clones = Z_Realloc(clones, sizeof(UINT8) * (parentclone->clones->numClones + 1), PU_STATIC, &i);
					clones[parentclone->clones->numClones - 1] = i;
				}
				//After that, we need to make sure this gets added to the array as a clone.
				parentclone_u *newUnion = malloc(sizeof(parentclone_u));
				newUnion->parentID = i;

				parentorclone_s *clone = malloc(sizeof(parentorclone_s));
				clone->uniondata = *newUnion;
				clone->isParent = false;

				setup_flatchargrid.skinList[i] = *clone;
			}
			continue;
		}

		// Otherwise this is a parent.
		setup_clonelist_s *newCloneList = malloc(sizeof(setup_clonelist_s));
		newCloneList->numClones = 0;
		newCloneList->cloneIds = NULL; //Nothing yet

		parentclone_u *newUnion = malloc(sizeof(parentclone_u));
		newUnion->clones = newCloneList;

		parentorclone_s *parent = malloc(sizeof(parentorclone_s));
		parent->uniondata = *newUnion;
		parent->isParent = true;

		setup_flatchargrid.skinList[i] = *parent;
	}

	setup_numfollowercategories = 0;
	for (i = 0; i < numfollowercategories; i++)
	{
		if (followercategories[i].numincategory == 0)
			continue;

		setup_followercategories[setup_numfollowercategories][0] = 0;

		for (j = 0; j < numfollowers; j++)
		{
			if (followers[j].category != i)
				continue;

			if (!K_FollowerUsable(j))
				continue;

			setup_followercategories[setup_numfollowercategories][0]++;
			setup_followercategories[setup_numfollowercategories][1] = i;
		}

		if (!setup_followercategories[setup_numfollowercategories][0])
			continue;

		setup_numfollowercategories++;
	}

	M_ResetDrawingList();
	setup_page = 0;
}

void M_Character1PSelect(INT32 choice)
{
	(void) choice;
	PLAY_CharSelect1PDef.music = "."; // currentMenu->music;
	PLAY_CharSelect1PDef.prevMenu = currentMenu;
	M_SetupNextMenu(&PLAY_CharSelect1PDef, false);
}

// Gets the selected follower's state for a given setup player.
static void M_GetFollowerState(setup_player_t* p)
{
	p->follower_state = &states[followers[p->followern].followstate];

	if (p->follower_state->frame & FF_ANIMATE)
		p->follower_tics = p->follower_state->var2; // support for FF_ANIMATE
	else
		p->follower_tics = p->follower_state->tics;

	p->follower_frame = p->follower_state->frame & FF_FRAMEMASK;
}

static boolean M_DeviceAvailable(INT32 deviceID, UINT8 numPlayers)
{
	INT32 i;

	if (numPlayers == 0)
	{
		// All of them are available!
		return true;
	}

	for (i = 0; i < numPlayers; i++)
	{
		int player_device = G_GetDeviceForPlayer(i);
		if (player_device == -1)
		{
			continue;
		}
		if (player_device == deviceID)
		{
			// This one's already being used.
			return false;
		}
	}

	// This device is good to go.
	return true;
}

// TODO: Nuke this shit. If we detect a new player, move to the vanilla character selection screen
static boolean M_HandlePressStart(setup_player_t* p, UINT8 num)
{
	INT32 i, j;
	INT32 num_gamepads_available;

	if (optionsmenu.profile)
		return false; // Don't allow for the possibility of SOMEHOW another player joining in.

	// Detect B press first ... this means P1 can actually exit out of the menu.
	if (M_MenuBackPressed(num))
	{
		M_SetMenuDelay(num);

		if (num == 0)
		{
			// We're done here.
			memset(setup_player, 0, sizeof(setup_player)); // Reset this to avoid funky things with profile display.
			M_GoBack(0);
			return true;
		}

		// Don't allow this press to ever count as "start".
		return false;
	}

	if (num != setup_numplayers)
	{
		// Only detect devices for the last player.
		return false;
	}

	// Now detect new devices trying to join.
	num_gamepads_available = G_GetNumAvailableGamepads();
	for (i = 0; i < num_gamepads_available + 1; i++)
	{
		INT32 device = 0;

		if (i > 0)
		{
			device = G_GetAvailableGamepadDevice(i - 1);
		}

		if (device == KEYBOARD_MOUSE_DEVICE && num != 0)
		{
			// Only player 1 can be assigned to the KBM device.
			continue;
		}

		if (G_IsDeviceResponding(device) != true)
		{
			// No buttons are being pushed.
			continue;
		}

		if (M_DeviceAvailable(device, setup_numplayers) == true)
		{
			// Available!! Let's use this one!!

			// if P1 is setting up using keyboard (device 0), save their last used device.
			// this is to allow them to retain controller usage when they play alone.
			// Because let's face it, when you test mods, you're often lazy to grab your controller for menuing :)
			if (i == 0 && num == 0)
			{
				setup_player[num].ponedevice = G_GetDeviceForPlayer(num);
			}
			else if (num)
			{
				// For any player past player 1, set controls to default profile controls, otherwise it's generally
				// awful to do any menuing...
				G_ApplyControlScheme(num, gamecontroldefault);
			}

			G_SetDeviceForPlayer(num, device);
#ifdef CHARSELECT_DEVICEDEBUG
			CONS_Printf("M_HandlePressStart: Device for %d set to %d\n", num, device);
#endif

			for (j = num + 1; j < MAXSPLITSCREENPLAYERS; j++)
			{
				// Un-set devices for other players.
				G_SetDeviceForPlayer(j, -1);
#ifdef CHARSELECT_DEVICEDEBUG
				CONS_Printf("M_HandlePressStart: Device for %d set to %d\n", j, -1);
#endif
			}

			// setup_numplayers++;
			p->mdepth = CSSTEP_PROFILE;
			S_StartSound(NULL, sfx_s3k65);

			// Prevent quick presses for multiple players
			for (j = 0; j < MAXSPLITSCREENPLAYERS; j++)
			{
				setup_player[j].delay = MENUDELAYTIME;
				M_SetMenuDelay(j);
				menucmd[j].buttonsHeld |= MBT_X;
			}

			G_ResetAllDeviceResponding();
			return true;
		}
	}

	return false;
}

static boolean M_HandleCSelectProfile(setup_player_t* p, UINT8 num)
{
	const UINT8 maxp = PR_GetNumProfiles() - 1;
	UINT8 realnum = num; // Used for profile when using splitdevice.
	UINT8 i;

	if (cv_splitdevice.value)
		num = 0;

	if (menucmd[num].dpad_ud > 0)
	{
		UINT8 oldn = p->profilen;
		p->profilen++;
		if (p->profilen > maxp)
			p->profilen = 0;
		p->profilen_slide.dist = p->profilen - oldn;
		p->profilen_slide.start = I_GetTime();

		S_StartSound(NULL, sfx_s3k5b);
		M_SetMenuDelay(num);
	}
	else if (menucmd[num].dpad_ud < 0)
	{
		UINT8 oldn = p->profilen;
		if (p->profilen == 0)
			p->profilen = maxp;
		else
			p->profilen--;
		p->profilen_slide.dist = p->profilen - oldn;
		p->profilen_slide.start = I_GetTime();

		S_StartSound(NULL, sfx_s3k5b);
		M_SetMenuDelay(num);
	}
	else if (M_MenuBackPressed(num))
	{
		if (num == setup_numplayers - 1)
		{
			p->mdepth = CSSTEP_NONE;
			S_StartSound(NULL, sfx_s3k5b);

			// Prevent quick presses for multiple players
			for (i = 0; i < MAXSPLITSCREENPLAYERS; i++)
			{
				setup_player[i].delay = MENUDELAYTIME;
				M_SetMenuDelay(i);
				menucmd[i].buttonsHeld |= MBT_X;
			}

			G_SetDeviceForPlayer(num, -1);
#ifdef CHARSELECT_DEVICEDEBUG
			CONS_Printf("M_HandleCSelectProfile: Device for %d set to %d\n", num, -1);
#endif

			return true;
		}
		else
		{
			S_StartSound(NULL, sfx_s3kb2);
		}

		M_SetMenuDelay(num);
	}
	else if (M_MenuConfirmPressed(num))
	{
		SINT8 belongsTo = -1;

		if (p->profilen != PROFILE_GUEST)
		{
			for (i = 0; i < setup_numplayers; i++)
			{
				if (setup_player[i].mdepth > CSSTEP_PROFILE && setup_player[i].profilen == p->profilen)
				{
					belongsTo = i;
					break;
				}
			}
		}

		if (belongsTo != -1 && belongsTo != num)
		{
			S_StartSound(NULL, sfx_s3k7b);
			M_SetMenuDelay(num);
			return false;
		}

		// Apply the profile.
		PR_ApplyProfile(
			p->profilen,
			realnum
		); // Otherwise P1 would inherit the last player's profile in splitdevice and that's not what we want...
		M_SetupProfileGridPos(p);

		p->changeselect = 0;

		if (p->profilen == PROFILE_GUEST)
		{
			// Guest profile, always ask for options.
			p->mdepth = CSSTEP_CHARS;
		}
		else
		{
			p->mdepth = CSSTEP_ASKCHANGES;
			M_GetFollowerState(p);
		}

		S_StartSound(NULL, sfx_s3k63);
	}
	else if (M_MenuExtraPressed(num))
	{
		UINT8 oldn = p->profilen;
		UINT8 yourprofile = min(cv_lastprofile[realnum].value, PR_GetNumProfiles());
		if (p->profilen == yourprofile)
			p->profilen = PROFILE_GUEST;
		else
			p->profilen = yourprofile;
		p->profilen_slide.dist = p->profilen - oldn;
		p->profilen_slide.start = I_GetTime();
		S_StartSound(NULL, sfx_s3k7b); // sfx_s3kc3s
		M_SetMenuDelay(num);
	}

	return false;
}

static void M_HandlePlayerFinalise(setup_player_t* p)
{
	p->mdepth = CSSTEP_READY;
	p->delay = TICRATE;
	M_SetupReadyExplosions(true, p->gridx, p->gridy, p->color);
	S_StartSound(NULL, sfx_s3k4e);
}

static void M_HandleCharAskChange(setup_player_t* p, UINT8 num)
{
	if (cv_splitdevice.value)
		num = 0;

	// there's only 2 options so lol
	if (menucmd[num].dpad_ud != 0)
	{
		p->changeselect = (p->changeselect == 0) ? 1 : 0;

		S_StartSound(NULL, sfx_s3k5b);
		M_SetMenuDelay(num);
	}
	else if (M_MenuBackPressed(num))
	{
		p->changeselect = 0;
		p->mdepth = CSSTEP_PROFILE;

		S_StartSound(NULL, sfx_s3k5b);
		M_SetMenuDelay(num);
	}
	else if (M_MenuConfirmPressed(num))
	{
		if (!p->changeselect)
		{
			// no changes
			M_HandlePlayerFinalise(p);
		}
		else
		{
			// changes
			p->mdepth = CSSTEP_CHARS;
			S_StartSound(NULL, sfx_s3k63);
		}

		M_SetMenuDelay(num);
	}
}

/*
boolean M_CharacterSelectForceInAction(void)
{
	if (!Playing())
		return false;

	if (K_CanChangeRules(true) == false)
		return false;

	return (cv_forceskin.value != -1);
}*/

static void M_HandleBackToChars(setup_player_t* p)
{
	boolean forceskin = M_CharacterSelectForceInAction();
	if (forceskin || !setup_flatchargrid.skinList[p->skin].isParent)
	{
		p->mdepth = CSSTEP_CHARS; // Skip clones menu
		return;
	}

	//Otherwise we are in clones.
	p->mdepth = CSSTEP_ALTS;
}

static boolean M_HandleBeginningColors(setup_player_t* p)
{
	p->mdepth = CSSTEP_COLORS;
	M_NewPlayerColors(p);
	if (p->colors.listLen != 1)
		return true;

	p->color = p->colors.list[0];
	return false;
}

static void M_HandleBeginningFollowers(setup_player_t* p)
{
	if (setup_numfollowercategories == 0)
	{
		p->followern = -1;
		M_HandlePlayerFinalise(p);
	}
	else
	{
		p->mdepth = CSSTEP_FOLLOWERCATEGORY;
		S_StartSound(NULL, sfx_s3k63);
	}
}

static void M_HandleBeginningColorsOrFollowers(setup_player_t* p)
{
	if (p->skin != -1)
		S_StartSound(NULL, skins[p->skin].soundsid[S_sfx[sfx_kattk1].skinsound]);
	if (M_HandleBeginningColors(p))
		S_StartSound(NULL, sfx_s3k63);
	else
		M_HandleBeginningFollowers(p);
}

static boolean M_HandleCharacterGrid(setup_player_t* p, UINT8 num)
{
	UINT8 numclones;
	INT32 skin;
	boolean forceskin = M_CharacterSelectForceInAction();

	if (cv_splitdevice.value)
		num = 0;

	if (menucmd[num].dpad_ud > 0)
	{
		p->gridy++;
		if (p->gridy > 8)
			p->gridy = 0;
		S_StartSound(NULL, sfx_s3k5b);
		M_SetMenuDelay(num);
	}
	else if (menucmd[num].dpad_ud < 0)
	{
		p->gridy--;
		if (p->gridy < 0)
			p->gridy = 8;
		S_StartSound(NULL, sfx_s3k5b);
		M_SetMenuDelay(num);
	}

	if (menucmd[num].dpad_lr > 0)
	{
		p->gridx++;
		if (p->gridx > 8)
			p->gridx = 0;
		S_StartSound(NULL, sfx_s3k5b);
		M_SetMenuDelay(num);
	}
	else if (menucmd[num].dpad_lr < 0)
	{
		p->gridx--;
		if (p->gridx < 0)
			p->gridx = 8;
		S_StartSound(NULL, sfx_s3k5b);
		M_SetMenuDelay(num);
	}
	else if (M_MenuExtraPressed(num))
	{
		p->gridx /= 3;
		p->gridx = (3 * p->gridx) + 1;
		p->gridy /= 3;
		p->gridy = (3 * p->gridy) + 1;
		S_StartSound(NULL, sfx_s3k7b); // sfx_s3kc3s
		M_SetMenuDelay(num);
	}

	// try to set the clone num to the page # if possible.
	p->clonenum = setup_page;

	// Process this after possible pad movement,
	// this makes sure we don't have a weird ghost hover on a character with no clones.
	UINT8 skinIndexInPos = M_GetSkinIndexGivenPos(p);
	numclones = setup_flatchargrid.skinList[skinIndexInPos].isParent ? setup_flatchargrid.skinList[skinIndexInPos].uniondata.clones->numClones : 0;

	if (p->clonenum >= numclones)
		p->clonenum = 0;

	if (M_MenuConfirmPressed(num) /*|| M_MenuButtonPressed(num, MBT_START)*/)
	{
		if (forceskin)
		{
			if ((p->gridx != skins[cv_forceskin.value].kartspeed - 1) ||
				(p->gridy != skins[cv_forceskin.value].kartweight - 1))
			{
				S_StartSound(NULL, sfx_s3k7b); // sfx_s3kb2
			}
			else
			{
				M_HandleBeginningColorsOrFollowers(p);
			}
		}
		else
		{
			/* TODO: FIX THIS SHIT!
			skin = setup_flatchargrid.skinList[skinIndexInPos].skinlist[setup_page];
			if (setup_page >= setup_flatchargrid.skinList[skinIndexInPos].childNum || skin == -1)
			{
				S_StartSound(NULL, sfx_s3k7b); // sfx_s3kb2
			}
			else
			{
				if (setup_page + 1 == setup_flatchargrid[p->gridx][p->gridy].numskins)
				{
					M_HandleBeginningColorsOrFollowers(p);
				}
				else
				{
					p->mdepth = CSSTEP_ALTS;
					S_StartSound(NULL, sfx_s3k63);
				}
			}*/
		}

		M_SetMenuDelay(num);
	}
	else if (M_MenuBackPressed(num))
	{
		// for profiles / gameplay, exit out of the menu instantly,
		// we don't want to go to the input detection menu.
		if (optionsmenu.profile || gamestate != GS_MENU)
		{
			memset(
				setup_player,
				0,
				sizeof(setup_player)
			); // Reset setup_player otherwise it does some VERY funky things.
			M_SetMenuDelay(0);
			M_GoBack(0);
			return true;
		}
		else // in main menu
		{
			p->mdepth = CSSTEP_PROFILE;
			S_StartSound(NULL, sfx_s3k5b);
		}
		M_SetMenuDelay(num);
	}

	if (num == 0 && setup_numplayers == 1 && setup_maxpage && !forceskin) // ONLY one player.
	{
		if (M_MenuButtonPressed(num, MBT_L))
		{
			if (setup_page == setup_maxpage)
				setup_page = 0;
			else
				setup_page++;

			S_StartSound(NULL, sfx_s3k63);
			M_SetMenuDelay(num);
		}
	}

	return false;
}

static void M_HandleCharRotate(setup_player_t* p, UINT8 num)
{
	UINT8 skinIndexInPos = M_GetSkinIndexGivenPos(p);
	UINT8 numclones = setup_flatchargrid.skinList[skinIndexInPos].isParent ? setup_flatchargrid.skinList[skinIndexInPos].uniondata.clones->numClones : 0;

	if (cv_splitdevice.value)
		num = 0;

	if (menucmd[num].dpad_lr > 0)
	{
		p->clonenum++;
		if (p->clonenum >= numclones)
			p->clonenum = 0;
		p->rotate = CSROTATETICS;
		p->delay = CSROTATETICS;
		S_StartSound(NULL, sfx_s3kc3s);
	}
	else if (menucmd[num].dpad_lr < 0)
	{
		p->clonenum--;
		if (p->clonenum < 0)
			p->clonenum = numclones - 1;
		p->rotate = -CSROTATETICS;
		p->delay = CSROTATETICS;
		S_StartSound(NULL, sfx_s3kc3s);
	}

	if (M_MenuConfirmPressed(num) /*|| M_MenuButtonPressed(num, MBT_START)*/)
	{
		M_HandleBeginningColorsOrFollowers(p);
		M_SetMenuDelay(num);
	}
	else if (M_MenuBackPressed(num))
	{
		p->mdepth = CSSTEP_CHARS;
		S_StartSound(NULL, sfx_s3k5b);
		M_SetMenuDelay(num);
	}
	else if (M_MenuExtraPressed(num))
	{
		p->clonenum = 0;
		p->rotate = CSROTATETICS;
		p->hitlag = true;
		S_StartSound(NULL, sfx_s3k7b); // sfx_s3kc3s
		M_SetMenuDelay(num);
	}
}

static void M_HandleColorRotate(setup_player_t* p, UINT8 num)
{
	if (cv_splitdevice.value)
		num = 0;

	if (menucmd[num].dpad_lr > 0)
	{
		p->color = M_GetColorAfter(&p->colors, p->color, 1);
		p->rotate = CSROTATETICS;
		M_SetMenuDelay(num);		   // CSROTATETICS
		S_StartSound(NULL, sfx_s3k5b); // sfx_s3kc3s
	}
	else if (menucmd[num].dpad_lr < 0)
	{
		p->color = M_GetColorBefore(&p->colors, p->color, 1);
		p->rotate = -CSROTATETICS;
		M_SetMenuDelay(num);		   // CSROTATETICS
		S_StartSound(NULL, sfx_s3k5b); // sfx_s3kc3s
	}

	if (M_MenuConfirmPressed(num) /*|| M_MenuButtonPressed(num, MBT_START)*/)
	{
		M_HandleBeginningFollowers(p);
		M_SetMenuDelay(num);
	}
	else if (M_MenuBackPressed(num))
	{
		M_HandleBackToChars(p);

		S_StartSound(NULL, sfx_s3k5b);
		M_SetMenuDelay(num);
	}
	else if (M_MenuExtraPressed(num))
	{
		if (p->skin >= 0)
		{
			p->color = SKINCOLOR_NONE;
			p->rotate = CSROTATETICS;
			p->hitlag = true;
			S_StartSound(NULL, sfx_s3k7b); // sfx_s3kc3s
			M_SetMenuDelay(num);
		}
	}
}

static void M_AnimateFollower(setup_player_t* p)
{
	if (--p->follower_tics <= 0)
	{
		// FF_ANIMATE; cycle through FRAMES and get back afterwards. This will be prominent amongst followers hence why
		// it's being supported here.
		if (p->follower_state->frame & FF_ANIMATE)
		{
			p->follower_frame++;
			p->follower_tics = p->follower_state->var2;
			if (p->follower_frame >
				(p->follower_state->frame & FF_FRAMEMASK) + p->follower_state->var1) // that's how it works, right?
				p->follower_frame = p->follower_state->frame & FF_FRAMEMASK;
		}
		else
		{
			if (p->follower_state->nextstate != S_NULL)
				p->follower_state = &states[p->follower_state->nextstate];
			p->follower_tics = p->follower_state->tics;
			/*if (p->follower_tics == -1)
				p->follower_tics = 15;	// er, what?*/
			// get spritedef:
			p->follower_frame = p->follower_state->frame & FF_FRAMEMASK;
		}
	}

	p->follower_timer++;
}

static void M_HandleFollowerCategoryRotate(setup_player_t* p, UINT8 num)
{
	if (cv_splitdevice.value)
		num = 0;

	if (menucmd[num].dpad_lr > 0)
	{
		p->followercategory++;
		if (p->followercategory >= setup_numfollowercategories)
			p->followercategory = -1;

		p->rotate = CSROTATETICS;
		p->delay = CSROTATETICS;
		S_StartSound(NULL, sfx_s3kc3s);
	}
	else if (menucmd[num].dpad_lr < 0)
	{
		p->followercategory--;
		if (p->followercategory < -1)
			p->followercategory = setup_numfollowercategories - 1;

		p->rotate = -CSROTATETICS;
		p->delay = CSROTATETICS;
		S_StartSound(NULL, sfx_s3kc3s);
	}

	if (M_MenuConfirmPressed(num) /*|| M_MenuButtonPressed(num, MBT_START)*/)
	{
		if (p->followercategory < 0)
		{
			p->followern = -1;
			M_HandlePlayerFinalise(p);
		}
		else
		{
			if (p->followern < 0 ||
				followers[p->followern].category != setup_followercategories[p->followercategory][1])
			{
				p->followern = 0;
				while (p->followern < numfollowers)
				{
					if (followers[p->followern].category == setup_followercategories[p->followercategory][1] &&
						K_FollowerUsable(p->followern))
						break;
					p->followern++;
				}
			}

			if (p->followern < numfollowers)
			{
				M_GetFollowerState(p);
				p->mdepth = CSSTEP_FOLLOWER;
				S_StartSound(NULL, sfx_s3k63);
			}
			else
			{
				p->followern = -1;
				S_StartSound(NULL, sfx_s3kb2);
			}
		}

		M_SetMenuDelay(num);
	}
	else if (M_MenuBackPressed(num))
	{
		if (!M_HandleBeginningColors(p))
			M_HandleBackToChars(p);

		S_StartSound(NULL, sfx_s3k5b);
		M_SetMenuDelay(num);
	}
	else if (M_MenuExtraPressed(num))
	{
		p->followercategory = (p->followercategory == -1) ? M_GetMenuCategoryFromFollower(p) : -1;

		p->rotate = CSROTATETICS;
		p->hitlag = true;
		S_StartSound(NULL, sfx_s3k7b); // sfx_s3kc3s
		M_SetMenuDelay(num);
	}
}

static void M_HandleFollowerRotate(setup_player_t* p, UINT8 num)
{
	INT16 startfollowern = p->followern;

	if (cv_splitdevice.value)
		num = 0;

	if (menucmd[num].dpad_lr > 0)
	{
		do
		{
			p->followern++;
			if (p->followern >= numfollowers)
				p->followern = 0;
			if (p->followern == startfollowern)
				break;
		} while (followers[p->followern].category != setup_followercategories[p->followercategory][1] ||
				 !K_FollowerUsable(p->followern));

		M_GetFollowerState(p);

		p->rotate = CSROTATETICS;
		p->delay = CSROTATETICS;
		S_StartSound(NULL, sfx_s3kc3s);
	}
	else if (menucmd[num].dpad_lr < 0)
	{
		do
		{
			p->followern--;
			if (p->followern < 0)
				p->followern = numfollowers - 1;
			if (p->followern == startfollowern)
				break;
		} while (followers[p->followern].category != setup_followercategories[p->followercategory][1] ||
				 !K_FollowerUsable(p->followern));

		M_GetFollowerState(p);

		p->rotate = -CSROTATETICS;
		p->delay = CSROTATETICS;
		S_StartSound(NULL, sfx_s3kc3s);
	}

	if (M_MenuConfirmPressed(num) /*|| M_MenuButtonPressed(num, MBT_START)*/)
	{
		if (p->followern > -1)
		{
			p->mdepth = CSSTEP_FOLLOWERCOLORS;
			M_NewPlayerColors(p);
			S_StartSound(NULL, sfx_s3k63);
			if (p->followern != -1)
				S_StartSound(NULL, followers[p->followern].hornsound);
		}
		else
		{
			M_HandlePlayerFinalise(p);
		}

		M_SetMenuDelay(num);
	}
	else if (M_MenuBackPressed(num))
	{
		p->mdepth = CSSTEP_FOLLOWERCATEGORY;
		S_StartSound(NULL, sfx_s3k5b);
		M_SetMenuDelay(num);
	}
	else if (M_MenuExtraPressed(num))
	{
		p->mdepth = CSSTEP_FOLLOWERCATEGORY;
		p->followercategory = -1;
		p->rotate = CSROTATETICS;
		p->hitlag = true;
		S_StartSound(NULL, sfx_s3k7b); // sfx_s3kc3s
		M_SetMenuDelay(num);
	}
}

static void M_HandleFollowerColorRotate(setup_player_t* p, UINT8 num)
{
	if (cv_splitdevice.value)
		num = 0;

	if (menucmd[num].dpad_lr > 0)
	{
		p->followercolor = M_GetColorAfter(&p->colors, p->followercolor, 1);
		p->rotate = CSROTATETICS;
		M_SetMenuDelay(num);		   // CSROTATETICS
		S_StartSound(NULL, sfx_s3k5b); // sfx_s3kc3s
	}
	else if (menucmd[num].dpad_lr < 0)
	{
		p->followercolor = M_GetColorBefore(&p->colors, p->followercolor, 1);
		p->rotate = -CSROTATETICS;
		M_SetMenuDelay(num);		   // CSROTATETICS
		S_StartSound(NULL, sfx_s3k5b); // sfx_s3kc3s
	}

	if (M_MenuConfirmPressed(num) /*|| M_MenuButtonPressed(num, MBT_START)*/)
	{
		M_HandlePlayerFinalise(p);
		M_SetMenuDelay(num);
	}
	else if (M_MenuBackPressed(num))
	{
		M_GetFollowerState(p);
		p->mdepth = CSSTEP_FOLLOWER;
		S_StartSound(NULL, sfx_s3k5b);
		M_SetMenuDelay(num);
	}
	else if (M_MenuExtraPressed(num))
	{
		if (p->followercolor == FOLLOWERCOLOR_MATCH)
			p->followercolor = FOLLOWERCOLOR_OPPOSITE;
		else if (p->followercolor == SKINCOLOR_NONE)
			p->followercolor = FOLLOWERCOLOR_MATCH;
		else
			p->followercolor = SKINCOLOR_NONE;
		p->rotate = CSROTATETICS;
		p->hitlag = true;
		S_StartSound(NULL, sfx_s3k7b); // sfx_s3kc3s
		M_SetMenuDelay(num);
	}
}

// Handler of steps. This handles all 4 players, I couldn't see a reason to remove that unless player 1 handling broke
boolean M_Character1PSelectHandler(INT32 choice)
{
	INT32 i;
	boolean forceskin = M_CharacterSelectForceInAction();

	(void) choice;

	for (i = MAXSPLITSCREENPLAYERS - 1; i >= 0; i--)
	{
		setup_player_t* p = &setup_player[i];
		boolean playersChanged = false;

		if (p->mdepth > CSSTEP_FOLLOWER)
		{
			M_AnimateFollower(p);
		}

		if (p->delay == 0 && menucmd[i].delay == 0)
		{
			if (!optionsmenu.profile)
			{
				// If splitdevice is true, only do the last non-ready setups.
				if (cv_splitdevice.value)
				{
					// Previous setup isn't ready, go there.
					// In any case, do setup 0 first.
					if (i > 0 && setup_player[i - 1].mdepth < CSSTEP_READY)
						continue;
				}

				if (M_MenuButtonPressed(i, MBT_R))
				{
					p->showextra ^= true;
				}
			}

			switch (p->mdepth)
			{
			case CSSTEP_NONE:			  // Enter Game
				if (gamestate == GS_MENU) // do NOT handle that outside of GS_MENU.
					playersChanged = M_HandlePressStart(p, i);
				break;
			case CSSTEP_PROFILE:
				playersChanged = M_HandleCSelectProfile(p, i);
				break;
			case CSSTEP_ASKCHANGES:
				M_HandleCharAskChange(p, i);
				break;
			case CSSTEP_CHARS: // Character Select grid
				M_HandleCharacterGrid(p, i);
				break;
			case CSSTEP_ALTS: // Select clone
				M_HandleCharRotate(p, i);
				break;
			case CSSTEP_COLORS: // Select color
				M_HandleColorRotate(p, i);
				break;
			case CSSTEP_FOLLOWERCATEGORY:
				M_HandleFollowerCategoryRotate(p, i);
				break;
			case CSSTEP_FOLLOWER:
				M_HandleFollowerRotate(p, i);
				break;
			case CSSTEP_FOLLOWERCOLORS:
				M_HandleFollowerColorRotate(p, i);
				break;
			case CSSTEP_READY:
			default: // Unready
				if (M_MenuBackPressed(i))
				{
					if (!M_HandleBeginningColors(p))
						M_HandleBackToChars(p);

					S_StartSound(NULL, sfx_s3k5b);
					M_SetMenuDelay(i);
				}
				break;
			}
		}

		// Just makes it easier to access later
		if (forceskin)
		{
			if (p->gridx != skins[cv_forceskin.value].kartspeed - 1 || p->gridy != skins[cv_forceskin.value].kartweight - 1)
				p->skin = -1;
			else
				p->skin = cv_forceskin.value;
		}
		else
		{
			p->skin = M_GetSkinIndexGivenPos(p);
		}

		if (playersChanged == true)
		{
			setup_page = 0; // reset that.
			break;
		}
	}

	// Setup new numplayers
	setup_numplayers = 0;
	for (i = 0; i < MAXSPLITSCREENPLAYERS; i++)
	{
		if (setup_player[i].mdepth == CSSTEP_NONE)
			break;

		setup_numplayers = i + 1;
	}

	return true;
}

// Apply character skin and colour changes while ingame (we just call the skin / color commands.)
// ...Will this cause command buffer issues? -Lat'
static void M_MPConfirmCharacterSelection(void)
{
	UINT8 i;
	INT16 col;

	for (i = 0; i < splitscreen + 1; i++)
	{
		// colour
		// (convert the number that's saved to a string we can use)
		col = setup_player[i].color;
		CV_StealthSetValue(&cv_playercolor[i], col);

		// follower
		if (setup_player[i].followern < 0)
			CV_StealthSet(&cv_follower[i], "None");
		else
			CV_StealthSet(&cv_follower[i], followers[setup_player[i].followern].name);

		// finally, call the skin[x] console command.
		// This will call SendNameAndColor which will synch everything we sent here and apply the changes!

		CV_StealthSet(&cv_skin[i], skins[setup_player[i].skin].name);

		// ...actually, let's do this last - Skin_OnChange has some return-early occasions
		// follower color
		CV_SetValue(&cv_followercolor[i], setup_player[i].followercolor);
	}
	M_ClearMenus(true);
}

void M_Character1PSelectTick(void)
{
	UINT8 i;
	boolean setupnext = true;

	setup_animcounter++;

	for (i = 0; i < MAXSPLITSCREENPLAYERS; i++)
	{
		if (setup_player[i].delay)
			setup_player[i].delay--;

		if (setup_player[i].rotate > 0)
			setup_player[i].rotate--;
		else if (setup_player[i].rotate < 0)
			setup_player[i].rotate++;
		else
			setup_player[i].hitlag = false;

		if (i >= setup_numplayers)
			continue;

		if (setup_player[i].mdepth < CSSTEP_READY || setup_player[i].delay > 0)
		{
			// Someone's not ready yet.
			setupnext = false;
		}
	}

	for (i = 0; i < CSEXPLOSIONS; i++)
	{
		if (setup_explosions[i].tics > 0)
			setup_explosions[i].tics--;
	}

	if (setupnext && setup_numplayers > 0)
	{
		// Selecting from the menu
		if (gamestate == GS_MENU)
		{
			// in a profile; update the selected profile and then go back to the profile menu.
			if (optionsmenu.profile)
			{
				// save player
				strcpy(optionsmenu.profile->skinname, skins[setup_player[0].skin].name);
				optionsmenu.profile->color = setup_player[0].color;

				// save follower
				strcpy(optionsmenu.profile->follower, followers[setup_player[0].followern].name);
				optionsmenu.profile->followercolor = setup_player[0].followercolor;

				// reset setup_player
				memset(setup_player, 0, sizeof(setup_player));
				setup_numplayers = 0;

				M_GoBack(0);
				return;
			}
			else // in a normal menu, stealthset the cvars and then go to the play menu.
			{
				for (i = 0; i < setup_numplayers; i++)
				{
					CV_StealthSet(&cv_skin[i], skins[setup_player[i].skin].name);
					CV_StealthSetValue(&cv_playercolor[i], setup_player[i].color);

					if (setup_player[i].followern < 0)
						CV_StealthSet(&cv_follower[i], "None");
					else
						CV_StealthSet(&cv_follower[i], followers[setup_player[i].followern].name);
					CV_StealthSetValue(&cv_followercolor[i], setup_player[i].followercolor);

					G_SetPlayerGamepadIndicatorToPlayerColor(i);
				}

				CV_StealthSetValue(&cv_splitplayers, setup_numplayers);

#if defined(TESTERS)
				M_MPOptSelectInit(0);
#else
				M_SetupGametypeMenu(0);
#endif
			}
		}
		else // In a game
		{
			// 23/05/2022: Since there's already restrictskinchange, just allow this to happen regardless.
			M_MPConfirmCharacterSelection();
		}
	}

	if (optionsmenu.profile)
		M_OptionsTick();
}

boolean M_Character1PSelectQuit(void)
{
	return true;
}
#include <../../doomtype.h>
#include <../../k_menu.h> //For setup_numplayers, between others...
#include <../../k_hud.h> //For buttons like kp_button_r, 
#include <../../z_zone.h> //PU_CACHE
#include <../../r_draw.h> //TC_RAINBOW and others..
#include <../../r_skins.h> //For skins, obviously.
#include "../../r_state.h"
#include <../../st_stuff.h> //faceprefix
#include "../../i_time.h"
#include "../../m_easing.h"
#include "../../sanitize.h"
#include "../../v_video.h"

#include "../n_menu.h" //setup_flatchargrid

//
// PLAY MENUS
//

#define CHARSEL_GRID_XOFFSET 64;

static void M_DrawCharSelectCircle(setup_player_t *p, INT16 x, INT16 y)
{
	angle_t angamt = ANGLE_MAX;
	UINT16 i, numoptions = 0;
	INT16 l = 0, r = 0;
	INT16 subtractcheck;
	UINT8 skinIndexGivenPos = M_GetSkinIndexGivenPos(p);
	switch (p->mdepth)
	{
		case CSSTEP_ALTS:
			numoptions = setup_flatchargrid.skinList[skinIndexGivenPos].isParent ? setup_flatchargrid.skinList[skinIndexGivenPos].uniondata.clones->numClones : 0;
			break;
		case CSSTEP_COLORS:
		case CSSTEP_FOLLOWERCOLORS:
			numoptions = p->colors.listLen;
			break;
		case CSSTEP_FOLLOWERCATEGORY:
			numoptions = setup_numfollowercategories+1;
			break;
		case CSSTEP_FOLLOWER:
			numoptions = setup_followercategories[p->followercategory][0];
			break;
		default:
			return;
	}

	if (numoptions == 0)
	{
		return;
	}

	subtractcheck = 1 ^ (numoptions & 1);

	angamt /= numoptions;

	for (i = 0; i < numoptions; i++)
	{
		fixed_t cx = x << FRACBITS, cy = y << FRACBITS;
		boolean subtract = (i & 1) == subtractcheck;
		angle_t ang = ((i+1)/2) * angamt;
		patch_t *patch = NULL;
		UINT8 *colormap = NULL;
		fixed_t radius = 28<<FRACBITS;
		INT16 n = 0;

		switch (p->mdepth)
		{
			case CSSTEP_ALTS:
			{
				INT16 skin;

				n = (p->clonenum) + numoptions/2;
				if (subtract)
					n -= ((i+1)/2);
				else
					n += ((i+1)/2);
				n = (n + numoptions) % numoptions;

				skin = setup_flatchargrid.skinList[skinIndexGivenPos].uniondata.clones->cloneIds[n]; //If we're here, is because it is a parent. So no need to check if this is valid or not.
				patch = faceprefix[skin][FACE_RANK];
				colormap = R_GetTranslationColormap(skin, skins[skin].prefcolor, GTC_MENUCACHE);
				radius = 24<<FRACBITS;

				cx -= (SHORT(patch->width) << FRACBITS) >> 1;
				cy -= (SHORT(patch->height) << FRACBITS) >> 1;
				break;
			}

			case CSSTEP_COLORS:
			{
				INT16 diff;

				if (i == 0)
				{
					n = l = r = M_GetColorBefore(&p->colors, p->color, (numoptions/2) - (numoptions & 1));
				}
				else if (subtract)
				{
					n = l = M_GetColorBefore(&p->colors, l, 1);
				}
				else
				{
					n = r = M_GetColorAfter(&p->colors, r, 1);
				}

				colormap = R_GetTranslationColormap(TC_DEFAULT, (n == SKINCOLOR_NONE) ? skins[p->skin].prefcolor : n, GTC_MENUCACHE);

				diff = (numoptions - i) / 2;  // only 0 when i == numoptions-1

				if (diff == 0)
					patch = W_CachePatchName("COLORSP2", PU_CACHE);
				else if (abs(diff) < 25)
					patch = W_CachePatchName("COLORSP1", PU_CACHE);
				else
					patch = W_CachePatchName("COLORSP0", PU_CACHE);

				cx -= (SHORT(patch->width) << FRACBITS) >> 1;
				break;
			}

			case CSSTEP_FOLLOWERCATEGORY:
			{
				followercategory_t *fc = NULL;

				n = (p->followercategory + 1) + numoptions/2;
				if (subtract)
					n -= ((i+1)/2);
				else
					n += ((i+1)/2);
				n = (n + numoptions) % numoptions;

				if (n == 0)
				{
					patch = W_CachePatchName("K_NOBLNS", PU_CACHE);
				}
				else
				{
					fc = &followercategories[setup_followercategories[n - 1][1]];
					patch = W_CachePatchName(fc->icon, PU_CACHE);
				}

				radius = 24<<FRACBITS;

				cx -= (SHORT(patch->width) << FRACBITS) >> 1;
				cy -= (SHORT(patch->height) << FRACBITS) >> 1;
				break;
			}

			case CSSTEP_FOLLOWER:
			{
				follower_t *fl = NULL;
				INT16 startfollowern = p->followern;

				if (i == 0)
				{
					n = p->followern;
					r = (numoptions+1)/2;
					while (r)
					{
						n--;
						if (n < 0)
							n = numfollowers-1;
						if (n == startfollowern)
							break;
						if (followers[n].category == setup_followercategories[p->followercategory][1]
							&& K_FollowerUsable(n))
							r--;
					}
					l = r = n;
				}
				else if (subtract)
				{
					do
					{
						l--;
						if (l < 0)
							l = numfollowers-1;
						if (l == startfollowern)
							break;
					}
					while (followers[l].category != setup_followercategories[p->followercategory][1]
						|| !K_FollowerUsable(l));
					n = l;
				}
				else
				{
					do
					{
						r++;
						if (r >= numfollowers)
							r = 0;
						if (r == startfollowern)
							break;
					}
					while (followers[r].category != setup_followercategories[p->followercategory][1]
						|| !K_FollowerUsable(r));
					n = r;
				}

				{
					fl = &followers[n];
					patch = W_CachePatchName(fl->icon, PU_CACHE);

					colormap = R_GetTranslationColormap(TC_DEFAULT,
						K_GetEffectiveFollowerColor(fl->defaultcolor, fl, p->color, &skins[p->skin]),
						GTC_MENUCACHE
					);
				}

				radius = 24<<FRACBITS;

				cx -= (SHORT(patch->width) << FRACBITS) >> 1;
				cy -= (SHORT(patch->height) << FRACBITS) >> 1;
				break;
			}

			case CSSTEP_FOLLOWERCOLORS:
			{
				INT16 diff;
				UINT16 col;

				if (i == 0)
				{
					n = l = r = M_GetColorBefore(&p->colors, p->followercolor, (numoptions/2) - (numoptions & 1));
				}
				else if (subtract)
				{
					n = l = M_GetColorBefore(&p->colors, l, 1);
				}
				else
				{
					n = r = M_GetColorAfter(&p->colors, r, 1);
				}

				col = K_GetEffectiveFollowerColor(n, &followers[p->followern], p->color, &skins[p->skin]);

				colormap = R_GetTranslationColormap(TC_DEFAULT, col, GTC_MENUCACHE);

				diff = (numoptions - i)/2;  // only 0 when i == numoptions-1

				if (diff == 0)
					patch = W_CachePatchName("COLORSP2", PU_CACHE);
				else if (abs(diff) < 25)
					patch = W_CachePatchName("COLORSP1", PU_CACHE);
				else
					patch = W_CachePatchName("COLORSP0", PU_CACHE);

				cx -= (SHORT(patch->width) << FRACBITS) >> 1;
				break;
			}

			default:
				break;
		}

		if (subtract)
			ang = (signed)(ANGLE_90 - ang);
		else
			ang = ANGLE_90 + ang;

		if (numoptions & 1)
			ang = (signed)(ang - (angamt/2));

		if (p->rotate)
		{
			SINT8 rotate = p->rotate;
			if ((p->hitlag == true) && (setup_animcounter & 1))
				rotate = -rotate;
			ang = (signed)(ang + ((angamt / CSROTATETICS) * rotate));
		}

		cx += FixedMul(radius, FINECOSINE(ang >> ANGLETOFINESHIFT));
		cy -= FixedMul(radius, FINESINE(ang >> ANGLETOFINESHIFT)) / 3;

		V_DrawFixedPatch(cx, cy, FRACUNIT, 0, patch, colormap);
		if (p->mdepth == CSSTEP_ALTS && n != p->clonenum)
			V_DrawFixedPatch(cx, cy, FRACUNIT, V_TRANSLUCENT, W_CachePatchName("ICONDARK", PU_CACHE), NULL);
	}
}

static void M_DrawCharSelectSprite(UINT8 num, INT16 x, INT16 y, boolean charflip)
{
	setup_player_t *p = &setup_player[num];
	UINT8 color;
	UINT8 *colormap;

	if (p->skin < 0)
	{
		return;
	}

	if (p->mdepth < CSSTEP_COLORS && p->mdepth != CSSTEP_ASKCHANGES)
	{
		color = skins[p->skin].prefcolor;
	}
	else
	{
		color = p->color;
	}

	if (color == SKINCOLOR_NONE)
	{
		color = skins[p->skin].prefcolor;
	}

	colormap = R_GetTranslationColormap(p->skin, color, GTC_MENUCACHE);

	M_DrawCharacterSprite(x, y, p->skin, SPR2_STIN, (charflip ? 1 : 7), ((p->mdepth == CSSTEP_READY) ? setup_animcounter : 0), p->mdepth == CSSTEP_ASKCHANGES ? V_TRANSLUCENT : 0, colormap);
}

// Returns false is the follower shouldn't be rendered.
// 'num' can be used to directly specify the follower number, but doing this will not animate it.
// if a setup_player_t is specified instead, its data will be used to animate the follower sprite.
static boolean M_DrawFollowerSprite(INT16 x, INT16 y, INT32 num, boolean charflip, INT32 addflags, UINT8 *colormap, setup_player_t *p)
{
	spritedef_t *sprdef;
	spriteframe_t *sprframe;
	patch_t *patch;
	INT32 followernum;
	state_t *usestate;
	UINT32 useframe;
	follower_t *fl;
	UINT8 rotation = (charflip ? 1 : 7);

	if (p != NULL)
		followernum = p->followern;
	else
		followernum = num;

	// Don't draw if we're outta bounds.
	if (followernum < 0 || followernum >= numfollowers)
		return false;

	fl = &followers[followernum];

	if (p != NULL)
	{
		usestate = p->follower_state;
		useframe = p->follower_frame;
	}
	else
	{
		usestate = &states[followers[followernum].followstate];
		useframe = usestate->frame & FF_FRAMEMASK;
	}

	sprdef = &sprites[usestate->sprite];

	// draw the follower

	if (useframe >= sprdef->numframes)
		useframe = 0;	// frame doesn't exist, we went beyond it... what?

	sprframe = &sprdef->spriteframes[useframe];
	patch = W_CachePatchNum(sprframe->lumppat[rotation], PU_CACHE);

	if (sprframe->flip & (1<<rotation)) // Only for first sprite
	{
		addflags ^= V_FLIP; // This sprite is left/right flipped!
	}

	fixed_t sine = 0;

	if (p != NULL)
	{
		UINT16 color = K_GetEffectiveFollowerColor(
			(p->mdepth < CSSTEP_FOLLOWERCOLORS && p->mdepth != CSSTEP_ASKCHANGES) ? fl->defaultcolor : p->followercolor,
			fl,
			p->color,
			&skins[p->skin]
		);
		sine = FixedMul(fl->bobamp, FINESINE(((FixedMul(4 * M_TAU_FIXED, fl->bobspeed) * p->follower_timer)>>ANGLETOFINESHIFT) & FINEMASK));
		colormap = R_GetTranslationColormap(TC_DEFAULT, color, GTC_MENUCACHE);
	}

	V_DrawFixedPatch((x*FRACUNIT), ((y-12)*FRACUNIT) + sine, fl->scale, addflags, patch, colormap);

	return true;
}

//Character preview that is set up just for ONE PLAYER
static void M_DrawCharSelectPreview()
{
	INT16 x = 15, y = 50;
	setup_player_t *p = &setup_player[0];
	const boolean charflip = false;
	const UINT8 num = 1;

	V_DrawScaledPatch(x, y+6, V_TRANSLUCENT, W_CachePatchName("PREVBACK", PU_CACHE));

	V_DrawScaledPatch(x-5, y, 0, W_CachePatchName("DUELGRPH", PU_CACHE)); //We need to return the player frame as the player frames were part of the grid

	//Draw the profile thing
	INT32 backx = x + ((num & 1) ? -1 : 11);
	V_DrawScaledPatch(backx, y+2, 0, W_CachePatchName("FILEBACK", PU_CACHE));

	profile_t *pr = NULL;
	if (p->mdepth > CSSTEP_PROFILE)
	{
		pr = PR_GetProfile(p->profilen);
	}
	V_DrawCenteredFileString(backx+26, y+2, 0, pr ? pr->profilename : "SELECT"); //Originally was just "PLAYER", the fuck do you mean "PLAYER"? What am I supposed TO DO?
	

	//i wanted to always draw it (Draw it in the profile selection step...) but by then the player profile isn't assigned yet and eggman or the last skin selected appears instead of the profile you're hovering over...  i'd rather not touch it
	//Also unlike the original code this is AFTER the profile name banner because otherwise that would draw above the CHARACTER YOU WANT TO PLAY AS which is STINKY and I DO NOT LIKE
	if (p->mdepth >= CSSTEP_CHARS || p->mdepth == CSSTEP_ASKCHANGES) 
	{
		M_DrawCharSelectSprite(0, x+32, y+75, charflip);
		M_DrawCharSelectCircle(p, x+32, y+64);
	}

	if (p->mdepth >= CSSTEP_FOLLOWER || p->mdepth == CSSTEP_ASKCHANGES)
	{
		M_DrawFollowerSprite(x+32+((charflip ? 1 : -1)*16), y+75, -1, charflip, p->mdepth == CSSTEP_ASKCHANGES ? V_TRANSLUCENT : 0, NULL, p);
	}

	// Profile selection
	if (p->mdepth == CSSTEP_PROFILE)
	{
		INT16 px = x + 7; //Originally +12, selected this number so it appears centered
		INT16 py = y+48 - p->profilen*12 + Easing_OutSine(M_DueFrac(p->profilen_slide.start, 5), p->profilen_slide.dist*12, 0);
		UINT8 maxp = PR_GetNumProfiles();

		UINT8 i = 0;
		UINT8 j;

		V_SetClipRect(0, (y+25)*FRACUNIT, BASEVIDWIDTH*FRACUNIT, (5*12)*FRACUNIT, 0);


		for (i = 0; i < maxp; i++)
		{
			profile_t *pr = PR_GetProfile(i);
			INT16 dist = abs(p->profilen - i);
			INT32 notSelectable = 0;
			SINT8 belongsTo = -1;

			if (i != PROFILE_GUEST)
			{
				for (j = 0; j < setup_numplayers; j++)
				{
					if (setup_player[j].mdepth > CSSTEP_PROFILE
						&& setup_player[j].profilen == i)
					{
						belongsTo = j;
						break;
					}
				}
			}

			if (belongsTo != -1 && belongsTo != num)
			{
				notSelectable |= V_TRANSLUCENT;
			}

			if (dist > 3)
			{
				py += 12;
				continue;
			}

			if (dist > 1)
			{
				V_DrawCenteredFileString(px+26, py, notSelectable, pr->version ? pr->profilename : "NEW");
				V_DrawScaledPatch(px, py, V_TRANSLUCENT, W_CachePatchName("FILEBACK", PU_CACHE));
			}
			else
			{
				V_DrawScaledPatch(px, py, 0, W_CachePatchName("FILEBACK", PU_CACHE));

				if (i != p->profilen || ((setup_animcounter/10) & 1))
				{
					const char *txt = pr->version ? pr->profilename : "NEW";

					fixed_t w = V_StringScaledWidth(FRACUNIT,FRACUNIT,FRACUNIT,notSelectable,FILE_FONT,txt);

					V_DrawStringScaled(((px+26) * FRACUNIT) - (w/2), py * FRACUNIT, FRACUNIT, FRACUNIT, FRACUNIT, notSelectable, i == p->profilen ? R_GetTranslationColormap(TC_RAINBOW, SKINCOLOR_SAPPHIRE, GTC_CACHE) : NULL, FILE_FONT, txt);
				}
			}
			py += 12;
		}

		V_ClearClipRect();
	}
	// "Changes?"
	else if (p->mdepth == CSSTEP_ASKCHANGES)
	{
		UINT8 i;
		char choices[2][9] = {"ALL GOOD", "CHANGE"};
		INT32 xpos = x+8;
		INT32 ypos = y+38;

		V_DrawFileString(xpos, ypos, 0, "READY?");

		for (i = 0; i < 2; i++)
		{
			UINT8 cy = ypos+16 + (i*10);

			if (p->changeselect == i)
				M_DrawCursorHand(xpos + 20, cy);

			V_DrawThinString(xpos+16, cy, (p->changeselect == i ? highlightflags : 0), choices[i]);
		}
	}
	UINT8 skinIndexGivenPos = M_GetSkinIndexGivenPos(p);
}

static void M_DrawCharSelectInformation() {
	//We'll be accessing these a lot
	setup_player_t *p = &setup_player[0];
	const UINT16 *skin = &p->skin;
	const boolean ironman = skins[*skin].flags & SF_IRONMAN;

	INT16 x = 15, y = 135;
	const UINT8 ySpacing = 10;
	const UINT8 iconSpacing = 12;

	//Charname
	V_DrawCenteredFileString(x+34, y, 0, skins[*skin].realname);
	
	//Add some padding
	x += 4;
	y += 14;

	// Char icon
	UINT8*colormap = R_GetTranslationColormap(*skin, skins[*skin].prefcolor, GTC_MENUCACHE);
	V_DrawMappedPatch(x, y, 0, faceprefix[*skin][FACE_WANTED], colormap);
	
	// Stats
	const UINT8 speed = skins[*skin].kartspeed;
	const UINT8 weight = skins[*skin].kartweight;

	V_DrawThinString((x += 37), (y += 1), 0, va("Class %c", ('A' + R_GetEngineClass(speed, weight, ironman)))); //funny classes thing that v2 has

	V_DrawScaledPatch((x), ((y += ySpacing) - 1), 0, W_CachePatchName("CHSELSPD", PU_CACHE));
	V_DrawThinString((x + iconSpacing), (y), 0, va("%c S", ironman ? '?' : ('0' + speed)));

	V_DrawScaledPatch(x, ((y += ySpacing) - 1), 0, W_CachePatchName("CHSELWEI", PU_CACHE));
	V_DrawThinString((x + iconSpacing), (y), 0, va("%c W", ironman ? '?' : ('0' + weight)));
}


static void M_DrawColorDrawer() {
	setup_player_t *p = &setup_player[0];
	INT16 x = 15, y = 50;
	if(p->showextra) {

	}
}
#define IDLELEN 8
#define SELECTLEN (8 + IDLELEN + 7 + IDLELEN)

static void M_DrawCharSelectCursor(UINT8 num)
{
	static const char *idleframes[IDLELEN] = {
		"CHHOV1", "CHHOV1", "CHHOV1", "CHHOV2", "CHHOV1", "CHHOV3", "CHHOV1", "CHHOV2"
	};
	static const char *selectframesa[SELECTLEN] = {
		"CHHOV1", "CHPIKA1", "CHHOV2", "CHPIKA2", "CHHOV3", "CHPIKA3", "CHHOV2", "CHPIKA4",
		"CHHOV1", "CHHOV1", "CHHOV1", "CHHOV2", "CHHOV1", "CHHOV3", "CHHOV1", "CHHOV2",
		"CHPIKA5", "CHHOV2", "CHPIKA6", "CHHOV3", "CHPIKA7", "CHHOV2", "CHPIKA8",
		"CHHOV1", "CHHOV1", "CHHOV1", "CHHOV2", "CHHOV1", "CHHOV3", "CHHOV1", "CHHOV2"
	};
	static const char *selectframesb[SELECTLEN] = {
		NULL, "CHPIKB1", NULL, "CHPIKB2", NULL, "CHPIKB3", NULL, "CHPIKB4",
		NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
		"CHPIKB5", NULL, "CHPIKB6", NULL, "CHPIKB7", NULL, "CHPIKB8",
		NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL
	};

	setup_player_t *p = &setup_player[num];
	char letter = 'A' + num;
	UINT16 color = SKINCOLOR_NONE;
	UINT8 *colormap;
	INT16 x, y;
	INT16 quadx, quady;

	if (p->mdepth < CSSTEP_ASKCHANGES)
		return;

	quadx = 4 * (p->gridx / 3);
	quady = 4 * (p->gridy / 3);

	x = 82 + (p->gridx*16) + quadx - 13;
	y = 22 + (p->gridy*16) + quady - 12;

	// profiles skew the graphic to the right slightly
	if (optionsmenu.profile)
		x += 64;
	else {
		x += CHARSEL_GRID_XOFFSET;
	}

	color = p->color;
	if (color == SKINCOLOR_NONE)
	{
		if (p->skin >= 0)
		{
			color = skins[p->skin].prefcolor;
		}
		else
		{
			color = SKINCOLOR_GREY;
		}
	}

	colormap = R_GetTranslationColormap(TC_DEFAULT, color, GTC_MENUCACHE);

	if (p->mdepth >= CSSTEP_READY)
	{
		V_DrawMappedPatch(x, y, 0, W_CachePatchName("CHCNFRM0", PU_CACHE), colormap);
	}
	else if (p->mdepth > CSSTEP_CHARS)
	{
		V_DrawMappedPatch(x, y, 0, W_CachePatchName(selectframesa[setup_animcounter % SELECTLEN], PU_CACHE), colormap);
		if (selectframesb[(setup_animcounter-1) % SELECTLEN] != NULL)
			V_DrawMappedPatch(x, y, V_TRANSLUCENT, W_CachePatchName(selectframesb[(setup_animcounter-1) % SELECTLEN], PU_CACHE), colormap);
	}
	else
	{
		V_DrawMappedPatch(x, y, 0, W_CachePatchName(idleframes[setup_animcounter % IDLELEN], PU_CACHE), colormap);
	}

	if (p->mdepth < CSSTEP_READY)
		V_DrawMappedPatch(x, y, 0, W_CachePatchName(va("CSELH%c", letter), PU_CACHE), colormap);
}

#undef IDLELEN
#undef SELECTLEN

void M_DrawCharacter1PSelect(void)
{
	const UINT8 pid = 0;

	UINT8 i, j;
	INT16 quadx, quady;
	INT16 skin;
	INT32 basex = optionsmenu.profile ? (64 + M_EaseWithTransition(Easing_InSine, 5 * 48)) : 0;
	boolean forceskin = M_CharacterSelectForceInAction();
	const setup_player_t *p = &setup_player[0];

	{ // "Header", player instructions go here.
		const int kLeft = 76;
		const int kTop = 6;
		const int kButtonWidth = 16;
		INT32 x = basex + kLeft;

		//We are past the profile selection
		if(p->mdepth > CSSTEP_PROFILE) {
			K_drawButton((x += 22) * FRACUNIT, (kTop - 3) * FRACUNIT, 0, kp_button_r, M_MenuButtonPressed(pid, MBT_R));
			if(p->mdepth == CSSTEP_CHARS)
				V_DrawThinString((x += kButtonWidth), kTop, 0, "Followers");
			else if (p->mdepth == CSSTEP_FOLLOWER)
				V_DrawThinString((x += kButtonWidth), kTop, 0, "Characters");
			

			K_drawButton((x += 58) * FRACUNIT, (kTop - 1) * FRACUNIT, 0, kp_button_c[1], M_MenuButtonPressed(pid, MBT_C));
			V_DrawThinString((x += kButtonWidth), kTop, 0, "Colors & Search");
		}
		else {
			//Else hint the player to select a profile. X + 55 to center it. Couldn't bother wasting time to figure out the math being done here to properly center it!
			if (p->mdepth == CSSTEP_PROFILE)
			{
				V_DrawThinString(x + 55, kTop, 0, "Select a profile");
			}
			else if (((setup_animcounter/10) & 1)) {
				V_DrawThinString(x + 55, kTop, 0, "Press a button!");
			}
		}
	}

	#if 0
	// We have to loop twice -- first time to draw the drop shadows, a second time to draw the icons.
	if (forceskin == false)
	{
		for (i = 0; i < 9; i++)
		{
			for (j = 0; j < 9; j++)
			{
				skin = setup_flatchargrid[i][j].skinlist[setup_page];
				quadx = 4 * (i / 3);
				quady = 4 * (j / 3);

				// Here's a quick little cheat to save on drawing time!
				// Don't draw a shadow if it'll get covered by another icon
				if ((i % 3 < 2) && (j % 3 < 2))
				{
					if ((setup_flatchargrid[i+1][j].skinlist[setup_page] != -1)
					&& (setup_flatchargrid[i][j+1].skinlist[setup_page] != -1)
					&& (setup_flatchargrid[i+1][j+1].skinlist[setup_page] != -1))
						continue;
				}

				if (skin != -1)
					V_DrawScaledPatch(basex+ 82 + (i*16) + quadx + 1, 22 + (j*16) + quady + 1, 0, W_CachePatchName("ICONBACK", PU_CACHE));
			}
		}
	}
	#endif
	
	if(!optionsmenu.profile) {
		basex += CHARSEL_GRID_XOFFSET;
	}

    UINT8 finalWidth = CHARSEL_MAX_ROWS * CHARSEL_ICON_WIDTH + ((CHARSEL_MAX_ROWS / 3) * CHARSEL_ICON_PADDING_EVERY_THREE);
    UINT8 finalHeight = CHARSEL_MAX_ROWS * CHARSEL_ICON_HEIGHT + ((CHARSEL_MAX_ROWS / 3) * CHARSEL_ICON_PADDING_EVERY_THREE);

	//Set the clipping rect. the bottom (finalheight) has additional pixels so if theres more rows to select below, the player can see it peek out.
	V_SetClipRect((basex + 82)*FRACUNIT, (22)*FRACUNIT, finalWidth*FRACUNIT, (finalHeight + CHARSEL_ICON_HEIGHT / 2)*FRACUNIT, 0); 

	// Draw this inbetween. These drop shadows should be covered by the stat graph, but the icons shouldn't.
	V_DrawScaledPatch(basex+ 3, 2, 0, W_CachePatchName(("PR_STGRPH"), PU_CACHE));

	// Calculate the number of rows
	const UINT8 numrows = (setup_flatchargrid.drawingListCount + CHARSEL_MAX_ROWS - 1) / CHARSEL_MAX_ROWS;

	// Calculate the range of rows to draw
	const int start_row = setup_charsel1p_row_offset - 1;
	const int end_row = setup_charsel1p_row_offset + CHARSEL_MAX_ROWS;

	for (int row = start_row; row <= end_row; row++) {
		if (row < 0 || row >= numrows) continue;

		for (UINT8 col = 0; col < CHARSEL_MAX_COLUMNS; col++) {
			int index = row * CHARSEL_MAX_COLUMNS + col;
			if (index >= setup_flatchargrid.drawingListCount) break;

			const UINT8 drawingIndex = setup_flatchargrid.drawingList[index];
			UINT8 skin = drawingIndex; // setup_flatchargrid.skinList[drawingIndex];

			if (forceskin && skin != cv_forceskin.value) continue;

			//UINT8 quadx = CHARSEL_ICON_PADDING_EVERY_THREE * (col / 3);
			//UINT8 quady = CHARSEL_ICON_PADDING_EVERY_THREE * (row / 3);

			// Calculate the quadx and quady based on the visible position
			UINT8 visible_row = row - setup_charsel1p_row_offset;
			UINT8 quadx = CHARSEL_ICON_PADDING_EVERY_THREE * (col / 3);
			UINT8 quady = CHARSEL_ICON_PADDING_EVERY_THREE * (visible_row / 3);

			if (skin != -1)
			{
				UINT8* colormap;

				if (p->mdepth <= CSSTEP_PROFILE)
					colormap = R_GetTranslationColormap(TC_RAINBOW, SKINCOLOR_GREY, GTC_MENUCACHE); //Draw them grey if you aren't still in the grid part
				else
					colormap = R_GetTranslationColormap(skin, skins[skin].prefcolor, GTC_MENUCACHE);

				// Calculate the y-coordinate
				int y = 22 + ((row - setup_charsel1p_row_offset) * CHARSEL_ICON_HEIGHT) + quady;

				// Draw the icon
				V_DrawMappedPatch(basex + 82 + (col * CHARSEL_ICON_WIDTH) + quadx, y, 0, faceprefix[skin][FACE_RANK], colormap);
			}
		}
	}

	// Character grid
	#if 0
	for (UINT8 index = 0; index < setup_flatchargrid.drawingListCount; index++)
	{
		UINT8 i = index % CHARSEL_MAX_COLUMNS;
		UINT8 j = index / CHARSEL_MAX_COLUMNS;

		const UINT8 drawingIndex = setup_flatchargrid.drawingList[index];
		UINT8 skin = drawingIndex; //setup_flatchargrid.skinList[drawingIndex];

		if ((forceskin == true) && (skin != cv_forceskin.value))
			continue;

		UINT8 quadx = CHARSEL_ICON_PADDING_EVERY_THREE * (i / 3);
		UINT8 quady = CHARSEL_ICON_PADDING_EVERY_THREE * (j / 3);

		if (skin != -1)
		{
			UINT8* colormap;

			if (p->mdepth <= CSSTEP_PROFILE)
				colormap = R_GetTranslationColormap(TC_RAINBOW, SKINCOLOR_GREY, GTC_MENUCACHE); //Draw them grey if you aren't still in the grid part
			else
				colormap = R_GetTranslationColormap(skin, skins[skin].prefcolor, GTC_MENUCACHE);

			// Adjust the y-coordinate by the row offset
			int adjusted_y = 22 + (j * CHARSEL_ICON_HEIGHT) + quady - (setup_charsel1p_row_offset * CHARSEL_ICON_HEIGHT);

			// Draw the icon
    		V_DrawMappedPatch(basex + 82 + (i * CHARSEL_ICON_WIDTH) + quadx, adjusted_y, 0, faceprefix[skin][FACE_RANK], colormap);

			// draw dot if there are more alts behind there!
			//TODO: Do this when we start making the parenting shit
			/*
			if (!forceskin && setup_flatchargrid.skinList[index].isParent && setup_page + 1 < setup_flatchargrid.skinList[index].uniondata.clones->numClones)
				V_DrawScaledPatch(basex + 82 + (i * 16) + quadx, 22 + (j * 16) + quady + 11, 0, W_CachePatchName("ALTSDOT", PU_CACHE) );*/
			}
		} 
	#endif

	V_ClearClipRect();

	// Explosions when you've made your final selection
	M_DrawCharSelectExplosions(true, basex + 82, 22);

	// This is where we had the logic for drawing the rest of the players. Oh woops, its gone now. Sad!
	// Check if we are not in the options menu
	if (optionsmenu.profile == NULL)
	{
		M_DrawCharSelectPreview(i);
		if (setup_player[0].mdepth > CSSTEP_PROFILE)
		{
			M_DrawCharSelectInformation();
		}
	}
	else
	{
		M_DrawProfileCard(optionsmenu.optx, optionsmenu.opty, false, optionsmenu.profile);
	}

	// if (i >= setup_numplayers)
	//	continue;

	// Draw the cursors
	M_DrawCharSelectCursor(0);
}
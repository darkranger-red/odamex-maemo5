// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id: p_teleport.cpp 3174 2012-05-11 01:03:43Z mike $
//
// Copyright (C) 1993-1996 by id Software, Inc.
// Copyright (C) 2006-2012 by The Odamex Team.
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// DESCRIPTION:
//	Teleportation.
//
//-----------------------------------------------------------------------------


#include <stdio.h>
#include <stdlib.h>

#include "doomtype.h"
#include "doomdef.h"

#include "s_sound.h"

#include "p_local.h"


// State.
#include "r_state.h"


extern void P_CalcHeight (player_t *player);


//
// TELEPORTATION
//
// [RH] Changed to look for a destination by ThingID
BOOL EV_Teleport (int tid, int side, AActor *thing)
{
	AActor *m;
	unsigned	an;
	fixed_t 	oldx;
	fixed_t 	oldy;
	fixed_t 	oldz;
	player_t	*player;

    // don't teleport missiles
    if (thing->flags & MF_MISSILE)
		return false;		

    // Don't teleport if hit back of line,
    //  so you can get out of teleporter.
    if (side == 1)		
		return false;	

	// [RH] Find destination based on it's TID rather than
	//		the sector containing it. Also allow for destinations
	//		that remember their vertical position.
	if (NULL == (m = AActor::FindGoal (NULL, tid, MT_TELEPORTMAN)))
		if (NULL == (m = AActor::FindGoal (NULL, tid, MT_TELEPORTMAN2)))
			return false;

	// killough 5/12/98: exclude voodoo dolls:
	player = thing->player;
	if (player && player->mo != thing)
		player = NULL;

	oldx = thing->x;
	oldy = thing->y;
	oldz = thing->z;

	fixed_t destz = (m->type == MT_TELEPORTMAN) ? P_FloorHeight(m) : m->z;

	if (!P_TeleportMove (thing, m->x, m->y, destz, false))
		return false;

    // fraggle: this was changed in final doom, 
    // problem between normal doom2 1.9 and final doom
    // Note that although chex.exe is based on Final Doom,
    // it does not have this quirk.
    if (gamemission < pack_tnt || gamemission == chex)
		    thing->z = thing->floorz;

	if (player)
		player->viewz = thing->z + thing->player->viewheight;

	// spawn teleport fog at source and destination
	if(serverside && !(player && player->spectator))
	{
		S_Sound (new AActor (oldx, oldy, oldz, MT_TFOG), CHAN_VOICE, "misc/teleport", 1, ATTN_NORM);
		an = m->angle >> ANGLETOFINESHIFT;
		// emit sound at new spot
		S_Sound (new AActor (m->x+20*finecosine[an], m->y+20*finesine[an], thing->z, MT_TFOG), CHAN_VOICE, "misc/teleport", 1, ATTN_NORM);
	}

	// don't move for a bit
	if (player && !player->spectator)
		thing->reactiontime = 18;

	thing->momx = thing->momy = thing->momz = 0;
	thing->angle = m->angle;

	return true;
}

// [ML] Original vanilla-style EV_Teleport, based on code from chocolate doom
BOOL EV_LineTeleport (line_t *line, int side, AActor *thing)
{
	AActor *m;
	unsigned	an;
	int		i;
	int		tag;
	fixed_t 	oldx;
	fixed_t 	oldy;
	fixed_t 	oldz;
	player_t	*player;
	sector_t*	sector;
	TThinkerIterator<AActor> iterator;

    // don't teleport missiles
    if (thing->flags & MF_MISSILE)
		return false;

    // Don't teleport if hit back of line,
    //  so you can get out of teleporter.
    if (side == 1)		
		return false;	

	tag = line->id;
	// Yeah, cycle through all of them...
	for (i = 0; i < numsectors; i++)
	{
		if (sectors[ i ].tag == tag )
		{
			while ( (m = iterator.Next ()) )
			{
				// not a teleportman
				if (m->type != MT_TELEPORTMAN)
					continue;
				
				sector = m->subsector->sector;
				// wrong sector
				if (sector-sectors != i )
					continue;
									
				// killough 5/12/98: exclude voodoo dolls:
				player = thing->player;
				if (player && player->mo != thing)
					player = NULL;

				oldx = thing->x;
				oldy = thing->y;
				oldz = thing->z;

				fixed_t destz = (m->type == MT_TELEPORTMAN) ? P_FloorHeight(m) : m->z;

				if (!P_TeleportMove (thing, m->x, m->y, destz, false))
					return false;

				// fraggle: this was changed in final doom, 
				// problem between normal doom2 1.9 and final doom
				// Note that although chex.exe is based on Final Doom,
				// it does not have this quirk.
				if (gamemission < pack_tnt || gamemission == chex)
						thing->z = thing->floorz;

				if (player)
					player->viewz = thing->z + thing->player->viewheight;

				// spawn teleport fog at source and destination
				if(serverside && !(player && player->spectator))
				{
					S_Sound (new AActor (oldx, oldy, oldz, MT_TFOG), CHAN_VOICE, "misc/teleport", 1, ATTN_NORM);
					an = m->angle >> ANGLETOFINESHIFT;
					// emit sound at new spot
					S_Sound (new AActor (m->x+20*finecosine[an], m->y+20*finesine[an], thing->z, MT_TFOG), CHAN_VOICE, "misc/teleport", 1, ATTN_NORM);
				}

				// don't move for a bit
				if (player && !player->spectator)
					thing->reactiontime = 18;

				thing->momx = thing->momy = thing->momz = 0;
				thing->angle = m->angle;

				return true;
			}
		}
	}

	return false;
}


//
// Silent TELEPORTATION, by Lee Killough
// Primarily for rooms-over-rooms etc.
// [RH] Changed to find destination by tid rather than sector
//

BOOL EV_SilentTeleport (int tid, line_t *line, int side, AActor *thing)
{
	AActor    *m;

	// don't teleport missiles
	// Don't teleport if hit back of line,
	// so you can get out of teleporter.
	// [RH] This could potentially be called without a source linedef

	if (thing->flags & MF_MISSILE || !line)
		return false;

	if (NULL == (m = AActor::FindGoal (NULL, tid, MT_TELEPORTMAN)))
		if (NULL == (m = AActor::FindGoal (NULL, tid, MT_TELEPORTMAN2)))
			return false;

	// Height of thing above ground, in case of mid-air teleports:
	fixed_t z = thing->z - thing->floorz;

	// Get the angle between the exit thing and source linedef.
	// Rotate 90 degrees, so that walking perpendicularly across
	// teleporter linedef causes thing to exit in the direction
	// indicated by the exit thing.
	angle_t angle =
		P_PointToAngle(0, 0, line->dx, line->dy) - m->angle + ANG90;

	// Sine, cosine of angle adjustment
	fixed_t s = finesine[angle>>ANGLETOFINESHIFT];
	fixed_t c = finecosine[angle>>ANGLETOFINESHIFT];

	// Momentum of thing crossing teleporter linedef
	fixed_t momx = thing->momx;
	fixed_t momy = thing->momy;

	// Whether this is a player, and if so, a pointer to its player_t
	player_t *player = thing->player;

	// Attempt to teleport, aborting if blocked
	if (!P_TeleportMove (thing, m->x, m->y, z + m->floorz, false))
		return 0;

	// Rotate thing according to difference in angles
	thing->angle += angle;

	// Rotate thing's momentum to come out of exit just like it entered
	thing->momx = FixedMul(momx, c) - FixedMul(momy, s);
	thing->momy = FixedMul(momy, c) + FixedMul(momx, s);

	// Adjust player's view, in case there has been a height change
	// Voodoo dolls are excluded by making sure player->mo == thing.
	if (player && player->mo == thing)
	{
		// Save the current deltaviewheight, used in stepping
		fixed_t deltaviewheight = player->deltaviewheight;

		// Clear deltaviewheight, since we don't want any changes
		player->deltaviewheight = 0;

		// Set player's view according to the newly set parameters
		P_CalcHeight(player);

		// Reset the delta to have the same dynamics as before
		player->deltaviewheight = deltaviewheight;
	}
	return true;
}

//
// Silent linedef-based TELEPORTATION, by Lee Killough
// Primarily for rooms-over-rooms etc.
// This is the complete player-preserving kind of teleporter.
// It has advantages over the teleporter with thing exits.
//

// maximum fixed_t units to move object to avoid hiccups
#define FUDGEFACTOR 10

// [RH] Modified to support different source and destination ids.
BOOL EV_SilentLineTeleport (line_t *line, int side, AActor *thing, int id,
							BOOL reverse)
{
	int i;
	line_t *l;

	if (thing->flags & MF_MISSILE || !line)
		return false;

	for (i = -1; (i = P_FindLineFromID (id, i)) >= 0;)
		if ((l=lines+i) != line && l->backsector)
		{
			// Get the thing's position along the source linedef
			fixed_t pos = abs(line->dx) > abs(line->dy) ?
				FixedDiv(thing->x - line->v1->x, line->dx) :
				FixedDiv(thing->y - line->v1->y, line->dy) ;

			// Get the angle between the two linedefs, for rotating
			// orientation and momentum. Rotate 180 degrees, and flip
			// the position across the exit linedef, if reversed.
			angle_t angle = (reverse ? pos = FRACUNIT-pos, 0 : ANG180) +
				P_PointToAngle(0, 0, l->dx, l->dy) -
				P_PointToAngle(0, 0, line->dx, line->dy);

			// Interpolate position across the exit linedef
			fixed_t x = l->v2->x - FixedMul(pos, l->dx);
			fixed_t y = l->v2->y - FixedMul(pos, l->dy);

			// Sine, cosine of angle adjustment
			fixed_t s = finesine[angle>>ANGLETOFINESHIFT];
			fixed_t c = finecosine[angle>>ANGLETOFINESHIFT];

			// Maximum distance thing can be moved away from interpolated
			// exit, to ensure that it is on the correct side of exit linedef
			int fudge = FUDGEFACTOR;

			// Whether this is a player, and if so, a pointer to its player_t.
			// Voodoo dolls are excluded by making sure thing->player->mo==thing.
			player_t *player = thing->player && thing->player->mo == thing ?
				thing->player : NULL;

			// Height of thing above ground
			fixed_t z = thing->z -
						P_FloorHeight(thing->x, thing->y, sides[line->sidenum[1]].sector) +
						P_FloorHeight(x, y, sides[l->sidenum[0]].sector);		
	
			// Side to exit the linedef on positionally.
			//
			// Notes:
			//
			// This flag concerns exit position, not momentum. Due to
			// roundoff error, the thing can land on either the left or
			// the right side of the exit linedef, and steps must be
			// taken to make sure it does not end up on the wrong side.
			//
			// Exit momentum is always towards side 1 in a reversed
			// teleporter, and always towards side 0 otherwise.
			//
			// Exiting positionally on side 1 is always safe, as far
			// as avoiding oscillations and stuck-in-wall problems,
			// but may not be optimum for non-reversed teleporters.
			//
			// Exiting on side 0 can cause oscillations if momentum
			// is towards side 1, as it is with reversed teleporters.
			//
			// Exiting on side 1 slightly improves player viewing
			// when going down a step on a non-reversed teleporter.

			int side = reverse;

			// Make sure we are on correct side of exit linedef.
			while (P_PointOnLineSide(x, y, l) != side && --fudge>=0)
				if (abs(l->dx) > abs(l->dy))
					y -= ((l->dx < 0) != side ? -1 : 1);
				else
					x += ((l->dy < 0) != side ? -1 : 1);

			// Attempt to teleport, aborting if blocked
			// Adjust z position to be same height above ground as before.
			// Ground level at the exit is measured as the higher of the
			// two floor heights at the exit linedef.
			if (!P_TeleportMove (thing, x, y, z, false))
				return false;

			// Rotate thing's orientation according to difference in linedef angles
			thing->angle += angle;

			// Momentum of thing crossing teleporter linedef
			x = thing->momx;
			y = thing->momy;

			// Rotate thing's momentum to come out of exit just like it entered
			thing->momx = FixedMul(x, c) - FixedMul(y, s);
			thing->momy = FixedMul(y, c) + FixedMul(x, s);

			// Adjust a player's view, in case there has been a height change
			if (player)
			{
				// Save the current deltaviewheight, used in stepping
				fixed_t deltaviewheight = player->deltaviewheight;

				// Clear deltaviewheight, since we don't want any changes now
				player->deltaviewheight = 0;

				// Set player's view according to the newly set parameters
				P_CalcHeight(player);

				// Reset the delta to have the same dynamics as before
				player->deltaviewheight = deltaviewheight;
			}

			return true;
		}
	return false;
}

VERSION_CONTROL (p_teleport_cpp, "$Id: p_teleport.cpp 3174 2012-05-11 01:03:43Z mike $")


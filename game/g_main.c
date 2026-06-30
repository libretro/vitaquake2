/*
 * Copyright (C) 1997-2001 Id Software, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or (at
 * your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 * See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
 * 02111-1307, USA.
 *
 * =======================================================================
 *
 * Jump in into the game.so and support functions.
 *
 * =======================================================================
 */

#include "header/local.h"

game_locals_t game;
level_locals_t level;
game_import_t gi;
game_export_t globals;
spawn_temp_t st;

int sm_meat_index;
int snd_fry;
int meansOfDeath;

edict_t *g_edicts;

cvar_t *deathmatch;
cvar_t *coop;
cvar_t *dmflags;
cvar_t *skill;
cvar_t *fraglimit;
cvar_t *timelimit;
cvar_t *password;
cvar_t *spectator_password;
cvar_t *needpass;
cvar_t *maxclients;
cvar_t *maxspectators;
cvar_t *maxentities;
cvar_t *g_select_empty;
cvar_t *dedicated;

cvar_t *filterban;

cvar_t *sv_maxvelocity;
cvar_t *sv_gravity;

cvar_t *sv_rollspeed;
cvar_t *sv_rollangle;
cvar_t *gun_x;
cvar_t *gun_y;
cvar_t *gun_z;

cvar_t *run_pitch;
cvar_t *run_roll;
cvar_t *bob_up;
cvar_t *bob_pitch;
cvar_t *bob_roll;

cvar_t *sv_cheats;

cvar_t *flood_msgs;
cvar_t *flood_persecond;
cvar_t *flood_waitdelay;

cvar_t *sv_maplist;

cvar_t *gib_on;

cvar_t *aimfix;

void SpawnEntities(char *mapname, char *entities, char *spawnpoint);
void ClientThink(edict_t *ent, usercmd_t *cmd);
qboolean ClientConnect(edict_t *ent, char *userinfo);
void ClientUserinfoChanged(edict_t *ent, char *userinfo);
void ClientDisconnect(edict_t *ent);
void ClientBegin(edict_t *ent);
void ClientCommand(edict_t *ent);
void RunEntity(edict_t *ent);
void WriteGame(char *filename, qboolean autosave);
void ReadGame(char *filename);
void WriteLevel(char *filename);
void ReadLevel(char *filename);
void InitGame(void);
void G_RunFrame(void);

/* =================================================================== */

void
ShutdownGame(void)
{
	gi.dprintf("==== ShutdownGame ====\n");

	gi.FreeTags(TAG_LEVEL);
	gi.FreeTags(TAG_GAME);
}

/*
 * Returns a pointer to the structure
 * with all entry points and global
 * variables
 */
game_export_t *
GetGameAPI(game_import_t *import)
{
	gi = *import;

	globals.apiversion = GAME_API_VERSION;
	globals.Init = InitGame;
	globals.Shutdown = ShutdownGame;
	globals.SpawnEntities = SpawnEntities;

	globals.WriteGame = WriteGame;
	globals.ReadGame = ReadGame;
	globals.WriteLevel = WriteLevel;
	globals.ReadLevel = ReadLevel;

	globals.ClientThink = ClientThink;
	globals.ClientConnect = ClientConnect;
	globals.ClientUserinfoChanged = ClientUserinfoChanged;
	globals.ClientDisconnect = ClientDisconnect;
	globals.ClientBegin = ClientBegin;
	globals.ClientCommand = ClientCommand;

	globals.RunFrame = G_RunFrame;

	globals.ServerCommand = ServerCommand;

	globals.edict_size = sizeof(edict_t);

	/* Initalize the PRNG */
	randk_seed();

	return &globals;
}

/* ====================================================================== */

void
ClientEndServerFrames(void)
{
	int i;
	edict_t *ent;

	/* calc the player views now that all
	   pushing  and damage has been added */
	for (i = 0; i < maxclients->value; i++)
	{
		ent = g_edicts + 1 + i;

		if (!ent->inuse || !ent->client)
		{
			continue;
		}

		ClientEndServerFrame(ent);
	}
}

/*
 * Returns the created target changelevel
 */
edict_t *
CreateTargetChangeLevel(char *map)
{
	edict_t *ent;

	if (!map)
	{
		return NULL;
	}

	ent = G_Spawn();
	ent->classname = "target_changelevel";
	Com_sprintf(level.nextmap, sizeof(level.nextmap), "%s", map);
	ent->map = level.nextmap;
	return ent;
}

/*
 * The timelimit or fraglimit has been exceeded
 */
void
EndDMLevel(void)
{
	edict_t *ent;
	char *s, *t, *f;
	static const char *seps = " ,\n\r";

	/* stay on same level flag */
	if ((int)dmflags->value & DF_SAME_LEVEL)
	{
		BeginIntermission(CreateTargetChangeLevel(level.mapname));
		return;
	}

	/* see if it's in the map list */
	if (*sv_maplist->string)
	{
		s = strdup(sv_maplist->string);
		f = NULL;
		t = strtok(s, seps);

		while (t != NULL)
		{
			if (Q_stricmp(t, level.mapname) == 0)
			{
				/* it's in the list, go to the next one */
				t = strtok(NULL, seps);

				if (t == NULL) /* end of list, go to first one */
				{
					if (f == NULL) /* there isn't a first one, same level */
					{
						BeginIntermission(CreateTargetChangeLevel(level.mapname));
					}
					else
					{
						BeginIntermission(CreateTargetChangeLevel(f));
					}
				}
				else
				{
					BeginIntermission(CreateTargetChangeLevel(t));
				}

				free(s);
				return;
			}

			if (!f)
			{
				f = t;
			}

			t = strtok(NULL, seps);
		}

		free(s);
	}

	if (level.nextmap[0]) /* go to a specific map */
	{
		BeginIntermission(CreateTargetChangeLevel(level.nextmap));
	}
	else    /* search for a changelevel */
	{
		ent = G_Find(NULL, FOFS(classname), "target_changelevel");

		if (!ent)
		{   /* the map designer didn't include a changelevel,
			   so create a fake ent that goes back to the same level */
			BeginIntermission(CreateTargetChangeLevel(level.mapname));
			return;
		}

		BeginIntermission(ent);
	}
}

void
CheckNeedPass(void)
{
	int need;

	/* if password or spectator_password has
	   changed, update needpass as needed */
	if (password->modified || spectator_password->modified)
	{
		password->modified = spectator_password->modified = false;

		need = 0;

		if (*password->string && Q_stricmp(password->string, "none"))
		{
			need |= 1;
		}

		if (*spectator_password->string &&
			Q_stricmp(spectator_password->string, "none"))
		{
			need |= 2;
		}

		gi.cvar_set("needpass", va("%d", need));
	}
}

void
CheckDMRules(void)
{
	int i;
	gclient_t *cl;

	if (level.intermissiontime)
	{
		return;
	}

	if (!deathmatch->value)
	{
		return;
	}

	if (timelimit->value)
	{
		if (level.time >= timelimit->value * 60)
		{
			gi.bprintf(PRINT_HIGH, "Timelimit hit.\n");
			EndDMLevel();
			return;
		}
	}

	if (fraglimit->value)
	{
		for (i = 0; i < maxclients->value; i++)
		{
			cl = game.clients + i;

			if (!g_edicts[i + 1].inuse)
			{
				continue;
			}

			if (cl->resp.score >= fraglimit->value)
			{
				gi.bprintf(PRINT_HIGH, "Fraglimit hit.\n");
				EndDMLevel();
				return;
			}
		}
	}
}

void
ExitLevel(void)
{
	int i;
	edict_t *ent;
	char command[256];

	Com_sprintf(command, sizeof(command), "gamemap \"%s\"\n", level.changemap);
	gi.AddCommandString(command);
	level.changemap = NULL;
	level.exitintermission = 0;
	level.intermissiontime = 0;
	ClientEndServerFrames();

	/* clear some things before going to next level */
	for (i = 0; i < maxclients->value; i++)
	{
		ent = g_edicts + 1 + i;

		if (!ent->inuse)
		{
			continue;
		}

		if (ent->health > ent->max_health)
		{
			ent->health = ent->max_health;
		}
	}

	debristhisframe = 0;
	gibsthisframe = 0;
}

/*
 * True only when two boxes interpenetrate by more than a small margin on every
 * axis - i.e. one is genuinely embedded in the other, not merely resting on a
 * face. A client riding on a platform touches its top but does not penetrate,
 * so it is not treated as embedded.
 */
static qboolean
Load_BoxesEmbedded(vec3_t amin, vec3_t amax, vec3_t bmin, vec3_t bmax)
{
	float lo, hi;
	int j;

	for (j = 0; j < 3; j++)
	{
		lo = (amin[j] > bmin[j]) ? amin[j] : bmin[j];
		hi = (amax[j] < bmax[j]) ? amax[j] : bmax[j];

		if ((hi - lo) <= 4.0f)
		{
			return false;
		}
	}

	return true;
}

/*
 * Returns true when the two boxes overlap at all (touching counts).
 */
static qboolean
Load_BoxesOverlap(vec3_t amin, vec3_t amax, vec3_t bmin, vec3_t bmax)
{
	if ((amin[0] > bmax[0]) || (amax[0] < bmin[0]))
	{
		return false;
	}

	if ((amin[1] > bmax[1]) || (amax[1] < bmin[1]))
	{
		return false;
	}

	if ((amin[2] > bmax[2]) || (amax[2] < bmin[2]))
	{
		return false;
	}

	return true;
}

/*
 * A savegame can capture a pushing mover (for example a vertically closing
 * blast-door) mid-move with its brush embedded in a client. On the next frame
 * the pusher deadlocks: it cannot advance without the client solid, and cannot
 * reverse because the client is buried inside it - so it crushes the player to
 * death (this is the Lost Station "stuck in the door and die on load" bug). In
 * that situation the player is usually penned in with nowhere to step, but the
 * mover itself has clear rest positions (open / closed). Snap any such mover to
 * whichever endpoint frees the embedded client so the level resumes cleanly.
 *
 * The detection requires genuine embedding (not a rider resting on a surface),
 * and a snap is only taken to an endpoint that demonstrably clears every
 * client, so a normal load - and a legitimately ridden platform - is untouched.
 */
void
Load_FreeStuckMovers(void)
{
	int i, c, k, chosen;
	edict_t *m, *cl;
	vec3_t relmin, relmax, cand[2], tmin, tmax;
	qboolean hit, clears;

	for (i = (int)maxclients->value + 1; i < globals.num_edicts; i++)
	{
		m = &g_edicts[i];

		if (!m->inuse || (m->movetype != MOVETYPE_PUSH) || !m->blocked)
		{
			continue; /* only solid pushers that can crush */
		}

		hit = false;

		for (c = 1; c <= (int)maxclients->value; c++)
		{
			cl = &g_edicts[c];

			if (cl->inuse && cl->client &&
				Load_BoxesEmbedded(m->absmin, m->absmax, cl->absmin, cl->absmax))
			{
				hit = true;
				break;
			}
		}

		if (!hit)
		{
			continue;
		}

		/* brush bbox relative to the mover's current s.origin */
		VectorSubtract(m->absmin, m->s.origin, relmin);
		VectorSubtract(m->absmax, m->s.origin, relmax);
		VectorCopy(m->moveinfo.end_origin, cand[0]);   /* open   */
		VectorCopy(m->moveinfo.start_origin, cand[1]); /* closed */

		chosen = -1;

		for (k = 0; (k < 2) && (chosen < 0); k++)
		{
			VectorAdd(relmin, cand[k], tmin);
			VectorAdd(relmax, cand[k], tmax);
			clears = true;

			for (c = 1; c <= (int)maxclients->value; c++)
			{
				cl = &g_edicts[c];

				if (cl->inuse && cl->client &&
					Load_BoxesOverlap(tmin, tmax, cl->absmin, cl->absmax))
				{
					clears = false;
					break;
				}
			}

			if (clears)
			{
				chosen = k;
			}
		}

		if (chosen < 0)
		{
			continue; /* neither endpoint frees the client; leave it be */
		}

		VectorCopy(cand[chosen], m->s.origin);
		VectorClear(m->velocity);
		m->moveinfo.state = (chosen == 0) ? 0 /* STATE_TOP */ : 1 /* STATE_BOTTOM */;
		m->moveinfo.current_speed = 0;
		m->think = NULL;
		m->nextthink = 0;
		gi.linkentity(m);

		/* detach any client that was riding/embedded in this mover */
		for (c = 1; c <= (int)maxclients->value; c++)
		{
			cl = &g_edicts[c];

			if (cl->inuse && (cl->groundentity == m))
			{
				cl->groundentity = NULL;
			}
		}
	}
}

/*
 * Advances the world by 0.1 seconds
 */
void
G_RunFrame(void)
{
	int i;
	edict_t *ent;

	level.framenum++;
	level.time = level.framenum * FRAMETIME;

	gibsthisframe = 0;
	debristhisframe = 0;

	/* choose a client for monsters to target this frame */
	AI_SetSightClient();

	/* exit intermissions */
	if (level.exitintermission)
	{
		ExitLevel();
		return;
	}

	/* treat each object in turn
	   even the world gets a chance
	   to think */
	ent = &g_edicts[0];

	for (i = 0; i < globals.num_edicts; i++, ent++)
	{
		if (!ent->inuse)
		{
			continue;
		}

		level.current_entity = ent;

		VectorCopy(ent->s.origin, ent->s.old_origin);

		/* if the ground entity moved, make sure we are still on it */
		if ((ent->groundentity) &&
			(ent->groundentity->linkcount != ent->groundentity_linkcount))
		{
			ent->groundentity = NULL;

			if (!(ent->flags & (FL_SWIM | FL_FLY)) &&
				(ent->svflags & SVF_MONSTER))
			{
				M_CheckGround(ent);
			}
		}

		if ((i > 0) && (i <= maxclients->value))
		{
			ClientBeginServerFrame(ent);
			continue;
		}

		G_RunEntity(ent);
	}

	/* see if it is time to end a deathmatch */
	CheckDMRules();

	/* see if needpass needs updated */
	CheckNeedPass();

	/* build the playerstate_t structures for all players */
	ClientEndServerFrames();
}

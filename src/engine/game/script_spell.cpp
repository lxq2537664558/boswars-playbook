//     ____                _       __               
//    / __ )____  _____   | |     / /___ ___________
//   / __  / __ \/ ___/   | | /| / / __ `/ ___/ ___/
//  / /_/ / /_/ (__  )    | |/ |/ / /_/ / /  (__  ) 
// /_____/\____/____/     |__/|__/\__,_/_/  /____/  
//                                              
//       A futuristic real-time strategy game.
//          This file is part of Bos Wars.
//
/**@name script_spell.cpp - The spell script functions.. */
//
//      (c) Copyright 1998-2007 by Joris Dauphin and Crestez Leonard
//
//      This program is free software; you can redistribute it and/or modify
//      it under the terms of the GNU General Public License as published by
//      the Free Software Foundation; only version 2 of the License.
//
//      This program is distributed in the hope that it will be useful,
//      but WITHOUT ANY WARRANTY; without even the implied warranty of
//      MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//      GNU General Public License for more details.
//
//      You should have received a copy of the GNU General Public License
//      along with this program; if not, write to the Free Software
//      Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
//      02111-1307, USA.
//
//@{

/*----------------------------------------------------------------------------
-- Includes
----------------------------------------------------------------------------*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "stratagus.h"
#include "spells.h"
#include "sound.h"
#include "script.h"
#include "missile.h"
#include "unittype.h"
#include "upgrade.h"


// **************************************************************************
// Action parsers for spellAction
// **************************************************************************

/**
**  Parse the missile location description for a spell action.
**
**  @param l         Lua state.
**  @param location  Pointer to missile location description.
**
**  @note This is only here to avoid code duplication. You don't have
**        any reason to USE this:)
*/
static void CclSpellMissileLocation(lua_State *l, SpellActionMissileLocation *location)
{
	const char *value;
	int args;
	int j;

	Assert(location != NULL);

	LuaCheckTable(l, -1);
	args = lua_objlen(l, -1);
	j = 0;

	for (j = 0; j < args; ++j) {
		value = LuaToString(l, -1, j + 1);
		++j;
		if (!strcmp(value, "base")) {
			value = LuaToString(l, -1, j + 1);
			if (!strcmp(value, "caster")) {
				location->Base = LocBaseCaster;
			} else if (!strcmp(value, "target")) {
				location->Base = LocBaseTarget;
			} else {
				LuaError(l, "Unsupported missile location base flag: %s" _C_ value);
			}
		} else if (!strcmp(value, "add-x")) {
			location->AddX = LuaToNumber(l, -1, j + 1);
		} else if (!strcmp(value, "add-y")) {
			location->AddY = LuaToNumber(l, -1, j + 1);
		} else if (!strcmp(value, "add-rand-x")) {
			location->AddRandX = LuaToNumber(l, -1, j + 1);
		} else if (!strcmp(value, "add-rand-y")) {
			location->AddRandY = LuaToNumber(l, -1, j + 1);
		} else {
			LuaError(l, "Unsupported missile location description flag: %s" _C_ value);
		}
	}
}

/**
**  Parse the action for spell.
**
**  @param l  Lua state.
*/
static SpellActionType *CclSpellAction(lua_State *l)
{
	const char *value;
	int args;
	int j;

	LuaCheckTable(l, -1);
	args = lua_objlen(l, -1);
	j = 0;

	value = LuaToString(l, -1, j + 1);
	++j;

	if (!strcmp(value, "spawn-missile")) {
		SpawnMissile *spellaction = new SpawnMissile;
		for (; j < args; ++j) {
			value = LuaToString(l, -1, j + 1);
			++j;
			if (!strcmp(value, "damage")) {
				spellaction->Damage = LuaToNumber(l, -1, j + 1);
			} else if (!strcmp(value, "delay")) {
				spellaction->Delay = LuaToNumber(l, -1, j + 1);
			} else if (!strcmp(value, "ttl")) {
				spellaction->TTL = LuaToNumber(l, -1, j + 1);
			} else if (!strcmp(value, "start-point")) {
				lua_rawgeti(l, -1, j + 1);
				CclSpellMissileLocation(l, &spellaction->StartPoint);
				lua_pop(l, 1);
			} else if (!strcmp(value, "end-point")) {
				lua_rawgeti(l, -1, j + 1);
				CclSpellMissileLocation(l, &spellaction->EndPoint);
				lua_pop(l, 1);
			} else if (!strcmp(value, "missile")) {
				value = LuaToString(l, -1, j + 1);
				spellaction->Missile = MissileTypeByIdent(value);
				if (spellaction->Missile == NULL) {
					DebugPrint("in spawn-missile : missile %s does not exist\n" _C_ value);
				}
			} else {
				LuaError(l, "Unsupported spawn-missile tag: %s" _C_ value);
			}
		}
		// Now, checking value.
		if (spellaction->Missile == NULL) {
			LuaError(l, "Use a missile for spawn-missile (with missile)");
		}
		return spellaction;
	} else if (!strcmp(value, "area-adjust-vitals")) {
		AreaAdjustVitals *spellaction = new AreaAdjustVitals;
		for (; j < args; ++j) {
			value = LuaToString(l, -1, j + 1);
			++j;
			if (!strcmp(value, "hit-points")) {
				spellaction->HP = LuaToNumber(l, -1, j + 1);
			} else if (!strcmp(value, "mana-points")) {
				spellaction->Mana = LuaToNumber(l, -1, j + 1);
			} else {
				LuaError(l, "Unsupported area-adjust-vitals tag: %s" _C_ value);
			}
		}
		return spellaction;
	} else if (!strcmp(value, "area-bombardment")) {
		AreaBombardment *spellaction = new AreaBombardment;
		for (; j < args; ++j) {
			value = LuaToString(l, -1, j + 1);
			++j;
			if (!strcmp(value, "fields")) {
				spellaction->Fields = LuaToNumber(l, -1, j + 1);
			} else if (!strcmp(value, "shards")) {
				spellaction->Shards = LuaToNumber(l, -1, j + 1);
			} else if (!strcmp(value, "damage")) {
				spellaction->Damage = LuaToNumber(l, -1, j + 1);
			} else if (!strcmp(value, "start-offset-x")) {
				spellaction->StartOffsetX = LuaToNumber(l, -1, j + 1);
			} else if (!strcmp(value, "start-offset-y")) {
				spellaction->StartOffsetY = LuaToNumber(l, -1, j + 1);
			} else if (!strcmp(value, "missile")) {
				value = LuaToString(l, -1, j + 1);
				spellaction->Missile = MissileTypeByIdent(value);
				if (spellaction->Missile == NULL) {
					DebugPrint("in area-bombardement : missile %s does not exist\n" _C_ value);
				}
			} else {
				LuaError(l, "Unsupported area-bombardment tag: %s" _C_ value);
			}
		}
		// Now, checking value.
		if (spellaction->Missile == NULL) {
			LuaError(l, "Use a missile for area-bombardment (with missile)");
		}
		return spellaction;
	} else if (!strcmp(value, "demolish")) {
		Demolish *spellaction = new Demolish;
		for (; j < args; ++j) {
			value = LuaToString(l, -1, j + 1);
			++j;
			if (!strcmp(value, "range")) {
				spellaction->Range = LuaToNumber(l, -1, j + 1);
			} else if (!strcmp(value, "damage")) {
				spellaction->Damage = LuaToNumber(l, -1, j + 1);
			} else {
				LuaError(l, "Unsupported demolish tag: %s" _C_ value);
			}
		}
		return spellaction;
	} else if (!strcmp(value, "adjust-variable")) {
		AdjustVariable *spellaction = new AdjustVariable;
		lua_rawgeti(l, -1, j + 1);
		LuaCheckTable(l, -1);
		spellaction->Var = new SpellActionTypeAdjustVariable[UnitTypeVar.NumberVariable];
		for (lua_pushnil(l); lua_next(l, -2); lua_pop(l, 1)) {
			int i;

			i = GetVariableIndex(LuaToString(l, -2));
			if (i == -1) {
				LuaError(l, "in adjust-variable : Bad variable index : '%s'" _C_ LuaToString(l, -2));
			}
			if (lua_isnumber(l, -1)) {
				spellaction->Var[i].Enable = (LuaToNumber(l, -1) != 0);
				spellaction->Var[i].ModifEnable = 1;
				spellaction->Var[i].Value = LuaToNumber(l, -1);
				spellaction->Var[i].ModifValue = 1;
				spellaction->Var[i].Max = LuaToNumber(l, -1);
				spellaction->Var[i].ModifMax = 1;
			} else if (lua_istable(l, -1)) {
				for (lua_pushnil(l); lua_next(l, -2); lua_pop(l, 1)) {
					const char *key;

					key = LuaToString(l, -2);
					if (!strcmp(key, "Enable")) {
						spellaction->Var[i].Enable = LuaToBoolean(l, -1);
						spellaction->Var[i].ModifEnable = 1;
					} else if (!strcmp(key, "Value")) {
						spellaction->Var[i].Value = LuaToNumber(l, -1);
						spellaction->Var[i].ModifValue = 1;
					} else if (!strcmp(key, "Max")) {
						spellaction->Var[i].Max = LuaToNumber(l, -1);
						spellaction->Var[i].ModifMax = 1;
					} else if (!strcmp(key, "Increase")) {
						spellaction->Var[i].Increase = LuaToNumber(l, -1);
						spellaction->Var[i].ModifIncrease = 1;
					} else if (!strcmp(key, "InvertEnable")) {
						spellaction->Var[i].InvertEnable = LuaToBoolean(l, -1);
					} else if (!strcmp(key, "AddValue")) {
						spellaction->Var[i].AddValue = LuaToNumber(l, -1);
					} else if (!strcmp(key, "AddMax")) {
						spellaction->Var[i].AddMax = LuaToNumber(l, -1);
					} else if (!strcmp(key, "AddIncrease")) {
						spellaction->Var[i].AddIncrease = LuaToNumber(l, -1);
					} else if (!strcmp(key, "IncreaseTime")) {
						spellaction->Var[i].IncreaseTime = LuaToNumber(l, -1);
					} else if (!strcmp(key, "TargetIsCaster")) {
						value = LuaToString(l, -1);
						if (!strcmp(value, "caster")) {
							spellaction->Var[i].TargetIsCaster = 1;
						} else if (!strcmp(value, "target")) {
							spellaction->Var[i].TargetIsCaster = 0;
						} else { // Error
							LuaError(l, "key '%s' not valid for TargetIsCaster in adjustvariable" _C_ value);
						}
					} else { // Error
						LuaError(l, "key '%s' not valid for adjustvariable" _C_ key);
					}
				}
			} else {
				LuaError(l, "in adjust-variable : Bad variable value");
			}
		}
		lua_pop(l, 1); // pop table
		return spellaction;
	} else if (!strcmp(value, "summon")) {
		Summon *spellaction = new Summon;
		for (; j < args; ++j) {
			value = LuaToString(l, -1, j + 1);
			++j;
			if (!strcmp(value, "unit-type")) {
				value = LuaToString(l, -1, j + 1);
				spellaction->UnitType = UnitTypeByIdent(value);
				if (!spellaction->UnitType) {
					spellaction->UnitType = 0;
					DebugPrint("unit type \"%s\" not found for summon spell.\n" _C_ value);
				}
			} else if (!strcmp(value, "time-to-live")) {
				spellaction->TTL = LuaToNumber(l, -1, j + 1);
			} else if (!strcmp(value, "require-corpse")) {
				spellaction->RequireCorpse = 1;
				--j;
			} else {
				LuaError(l, "Unsupported summon tag: %s" _C_ value);
			}
		}
		// Now, checking value.
		if (spellaction->UnitType == NULL) {
			LuaError(l, "Use a unittype for summon (with unit-type)");
		}
		return spellaction;
	} else if (!strcmp(value, "spawn-portal")) {
		SpawnPortal *spellaction = new SpawnPortal;
		for (; j < args; ++j) {
			value = LuaToString(l, -1, j + 1);
			++j;
			if (!strcmp(value, "portal-type")) {
				value = LuaToString(l, -1, j + 1);
				spellaction->PortalType = UnitTypeByIdent(value);
				if (!spellaction->PortalType) {
					spellaction->PortalType = 0;
					DebugPrint("unit type \"%s\" not found for spawn-portal.\n" _C_ value);
				}
			} else {
				LuaError(l, "Unsupported spawn-portal tag: %s" _C_ value);
			}
		}
		// Now, checking value.
		if (spellaction->PortalType == NULL) {
			LuaError(l, "Use a unittype for spawn-portal (with portal-type)");
		}
		return spellaction;
	} else if (!strcmp(value, "capture")) {
		Capture *spellaction = new Capture;
		for (; j < args; ++j) {
			value = LuaToString(l, -1, j + 1);
			++j;
			if (!strcmp(value, "sacrifice")) {
				spellaction->SacrificeEnable = 1;
			} else if (!strcmp(value, "damage")) {
				spellaction->Damage = LuaToNumber(l, -1, j + 1);
			} else if (!strcmp(value, "percent")) {
				spellaction->DamagePercent = LuaToNumber(l, -1, j + 1);
			} else {
				LuaError(l, "Unsupported Capture tag: %s" _C_ value);
			}
		}
		return spellaction;
	} else if (!strcmp(value, "polymorph")) {
		Polymorph *spellaction = new Polymorph;
		for (; j < args; ++j) {
			value = LuaToString(l, -1, j + 1);
			++j;
			if (!strcmp(value, "new-form")) {
				value = LuaToString(l, -1, j + 1);
				spellaction->NewForm = UnitTypeByIdent(value);
				if (!spellaction->NewForm) {
					spellaction->NewForm= 0;
					DebugPrint("unit type \"%s\" not found for polymorph spell.\n" _C_ value);
				}
				// FIXME: temp polymorphs? hard to do.
			} else if (!strcmp(value, "player-neutral")) {
				spellaction->PlayerNeutral = 1;
				--j;
			} else {
				LuaError(l, "Unsupported polymorph tag: %s" _C_ value);
			}
		}
		// Now, checking value.
		if (spellaction->NewForm == NULL) {
			LuaError(l, "Use a unittype for polymorph (with new-form)");
		}
		return spellaction;
	} else if (!strcmp(value, "adjust-vitals")) {
		AdjustVitals *spellaction = new AdjustVitals;
		for (; j < args; ++j) {
			value = LuaToString(l, -1, j + 1);
			++j;
			if (!strcmp(value, "hit-points")) {
				spellaction->HP = LuaToNumber(l, -1, j + 1);
			} else if (!strcmp(value, "mana-points")) {
				spellaction->Mana = LuaToNumber(l, -1, j + 1);
			} else if (!strcmp(value, "max-multi-cast")) {
				spellaction->MaxMultiCast = LuaToNumber(l, -1, j + 1);
			} else {
				LuaError(l, "Unsupported adjust-vitals tag: %s" _C_ value);
			}
		}
		return spellaction;
	} else {
		LuaError(l, "Unsupported action type: %s" _C_ value);
	}
	return NULL;
}

/**
**  Get a condition value from a scm object.
**
**  @param l      Lua state.
**  @param value  scm value to convert.
**
**  @return CONDITION_TRUE, CONDITION_FALSE, CONDITION_ONLY or -1 on error.
**  @note This is a helper function to make CclSpellCondition shorter
**        and easier to understand.
*/
char Ccl2Condition(lua_State *l, const char *value)
{
	if (!strcmp(value, "true")) {
		return CONDITION_TRUE;
	} else if (!strcmp(value, "false")) {
		return CONDITION_FALSE;
	} else if (!strcmp(value, "only")) {
		return CONDITION_ONLY;
	} else {
		LuaError(l, "Bad condition result: %s" _C_ value);
		return -1;
	}
}

/**
**  Parse the Condition for spell.
**
**  @param l          Lua state.
**  @param condition  pointer to condition to fill with data.
**
**  @note Conditions must be allocated. All data already in is LOST.
*/
static void CclSpellCondition(lua_State *l, ConditionInfo *condition)
{
	const char *value;
	int i;
	int args;
	int j;

	//
	// Initializations:
	//

	// Flags are defaulted to 0(CONDITION_TRUE)
	condition->Variable = new ConditionInfoVariable[UnitTypeVar.NumberVariable];
	// Initialize min/max stuff to values with no effect.
	for (i = 0; i < UnitTypeVar.NumberVariable; i++) {
		condition->Variable[i].MinValue = -1;
		condition->Variable[i].MaxValue = -1;
		condition->Variable[i].MinMax = -1;
		condition->Variable[i].MinValuePercent = -8;
		condition->Variable[i].MaxValuePercent = 1024;
	}
	//  Now parse the list and set values.
	LuaCheckTable(l, -1);
	args = lua_objlen(l, -1);
	for (j = 0; j < args; ++j) {
		value = LuaToString(l, -1, j + 1);
		++j;
		if (!strcmp(value, "alliance")) {
			condition->Alliance = Ccl2Condition(l, LuaToString(l, -1, j + 1));
		} else if (!strcmp(value, "opponent")) {
			condition->Opponent = Ccl2Condition(l, LuaToString(l, -1, j + 1));
		} else if (!strcmp(value, "self")) {
			condition->TargetSelf = Ccl2Condition(l, LuaToString(l, -1, j + 1));
		} else if (!strcmp(value, "Building")) {
			condition->Building = Ccl2Condition(l, LuaToString(l, -1, j + 1));
		} else if (!strcmp(value, "Organic")) {
			condition->Organic = Ccl2Condition(l, LuaToString(l, -1, j + 1));
		} else {
			i = GetVariableIndex(value);
			if (i != -1) { // Valid index.
				lua_rawgeti(l, -1, j + 1);
				LuaCheckTable(l, -1);
				for (lua_pushnil(l); lua_next(l, -2); lua_pop(l, 1)) {
					const char *key;

					key = LuaToString(l, -2);
					if (!strcmp(key, "Enable")) {
						condition->Variable[i].Enable = Ccl2Condition(l, LuaToString(l, -1));
					} else if (!strcmp(key, "MinValue")) {
						condition->Variable[i].MinValue = LuaToNumber(l, -1);
					} else if (!strcmp(key, "MaxValue")) {
						condition->Variable[i].MaxValue = LuaToNumber(l, -1);
					} else if (!strcmp(key, "MinMax")) {
						condition->Variable[i].MinMax = LuaToNumber(l, -1);
					} else if (!strcmp(key, "MinValuePercent")) {
						condition->Variable[i].MinValuePercent = LuaToNumber(l, -1);
					} else if (!strcmp(key, "MaxValuePercent")) {
						condition->Variable[i].MaxValuePercent = LuaToNumber(l, -1);
					} else if (!strcmp(key, "ConditionApplyOnCaster")) {
						condition->Variable[i].ConditionApplyOnCaster = LuaToBoolean(l, -1);
					} else { // Error
						LuaError(l, "%s invalid for Variable in condition" _C_ key);
					}
				}
				lua_pop(l, 1); // lua_rawgeti()
				continue;
			}
			LuaError(l, "Unsuported condition tag: %s" _C_ value);
		}
	}
}

/**
**  Parse the Condition for spell.
**
**  @param l         Lua state.
**  @param autocast  pointer to autocast to fill with data.
**
**  @note: autocast must be allocated. All data already in is LOST.
*/
static void CclSpellAutocast(lua_State *l, AutoCastInfo *autocast)
{
	const char *value;
	int args;
	int j;

	LuaCheckTable(l, -1);
	args = lua_objlen(l, -1);
	for (j = 0; j < args; ++j) {
		value = LuaToString(l, -1, j + 1);
		++j;
		if (!strcmp(value, "range")) {
			autocast->Range = LuaToNumber(l, -1, j + 1);
		} else if (!strcmp(value, "combat")) {
			autocast->Combat = Ccl2Condition(l, LuaToString(l, -1, j + 1));
		} else if (!strcmp(value, "condition")) {
			if (!autocast->Condition) {
				autocast->Condition = new ConditionInfo;
			}
			lua_rawgeti(l, -1, j + 1);
			CclSpellCondition(l, autocast->Condition);
			lua_pop(l, 1);
		} else {
			LuaError(l, "Unsupported autocast tag: %s" _C_ value);
		}
	}
}

/**
**  Parse Spell.
**
**  @param l  Lua state.
*/
static int CclDefineSpell(lua_State *l)
{
	std::string identname;
	SpellType *spell;
	const char *value;
	int args;

	args = lua_gettop(l);
	identname = LuaToString(l, 1);
	spell = SpellTypeByIdent(identname);
	if (spell != NULL) {
		DebugPrint("Redefining spell-type `%s'\n" _C_ identname.c_str());
	} else {
		spell = new SpellType(SpellTypeTable.size(), identname);
		for (size_t i = 0; i < UnitTypes.size(); ++i) { // adjust array for caster already defined
			if (UnitTypes[i]->CanCastSpell) {
				char *newc = new char[(SpellTypeTable.size() + 1) * sizeof(char)];
				memcpy(newc, UnitTypes[i]->CanCastSpell, SpellTypeTable.size() * sizeof(char));
				delete[] UnitTypes[i]->CanCastSpell;
				UnitTypes[i]->CanCastSpell = newc;
				UnitTypes[i]->CanCastSpell[SpellTypeTable.size()] = 0;
			}
			if (UnitTypes[i]->AutoCastActive) {
				char *newc = new char[(SpellTypeTable.size() + 1) * sizeof(char)];
				memcpy(newc, UnitTypes[i]->AutoCastActive, SpellTypeTable.size() * sizeof(char));
				delete[] UnitTypes[i]->AutoCastActive;
				UnitTypes[i]->AutoCastActive = newc;
				UnitTypes[i]->AutoCastActive[SpellTypeTable.size()] = 0;
			}
		}
		SpellTypeTable.push_back(spell);
	}
	for (int i = 1; i < args; ++i) {
		value = LuaToString(l, i + 1);
		++i;
		if (!strcmp(value, "showname")) {
			spell->Name = LuaToString(l, i + 1);
		} else if (!strcmp(value, "manacost")) {
			spell->ManaCost = LuaToNumber(l, i + 1);
		} else if (!strcmp(value, "range")) {
			if (!lua_isstring(l, i + 1) && !lua_isnumber(l, i + 1)) {
				LuaError(l, "incorrect argument");
			}
			if (lua_isstring(l, i + 1) && !strcmp(lua_tostring(l, i + 1), "infinite")) {
				spell->Range = INFINITE_RANGE;
			} else if (lua_isnumber(l, i + 1)) {
				spell->Range = static_cast<int>(lua_tonumber(l, i + 1));
			} else {
				LuaError(l, "Invalid range");
			}
		} else if (!strcmp(value, "repeat-cast")) {
			spell->RepeatCast = 1;
			--i;
		} else if (!strcmp(value, "target")) {
			value = LuaToString(l, i + 1);
			if (!strcmp(value, "self")) {
				spell->Target = TargetSelf;
			} else if (!strcmp(value, "unit")) {
				spell->Target = TargetUnit;
			} else if (!strcmp(value, "position")) {
				spell->Target = TargetPosition;
			} else {
				LuaError(l, "Unsupported spell target type tag: %s" _C_ value);
			}
		} else if (!strcmp(value, "action")) {
			int subargs;
			int k;

			LuaCheckTable(l, i + 1);
			subargs = lua_objlen(l, i + 1);
			for (k = 0; k < subargs; ++k) {
				lua_rawgeti(l, i + 1, k + 1);
				spell->Action.push_back(CclSpellAction(l));
				lua_pop(l, 1);
			}
		} else if (!strcmp(value, "condition")) {
			if (!spell->Condition) {
				spell->Condition = new ConditionInfo;
			}
			lua_pushvalue(l, i + 1);
			CclSpellCondition(l, spell->Condition);
			lua_pop(l, 1);
		} else if (!strcmp(value, "autocast")) {
			if (!spell->AutoCast) {
				spell->AutoCast = new AutoCastInfo();
			}
			lua_pushvalue(l, i + 1);
			CclSpellAutocast(l, spell->AutoCast);
			lua_pop(l, 1);
		} else if (!strcmp(value, "ai-cast")) {
			if (!spell->AICast) {
				spell->AICast = new AutoCastInfo();
			}
			lua_pushvalue(l, i + 1);
			CclSpellAutocast(l, spell->AICast);
			lua_pop(l, 1);
		} else if (!strcmp(value, "sound-when-cast")) {
			//  Free the old name, get the new one
			spell->SoundWhenCast.Name = LuaToString(l, i + 1);
			spell->SoundWhenCast.Sound = SoundForName(spell->SoundWhenCast.Name);
			//  Check for sound.
			if (!spell->SoundWhenCast.Sound) {
				spell->SoundWhenCast.Name.clear();
			}
		} else {
			LuaError(l, "Unsupported tag: %s" _C_ value);
		}
	}
	return 0;
}

/**
** Register CCL features for Spell.
*/
void SpellCclRegister(void)
{
	lua_register(Lua, "DefineSpell", CclDefineSpell);
}

//@}

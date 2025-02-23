/*
 * This file is part of the AzerothCore Project. See AUTHORS file for Copyright information
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Affero General Public License as published by the
 * Free Software Foundation; either version 3 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU Affero General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef _SCRIPT_MGR_MACRO_H_
#define _SCRIPT_MGR_MACRO_H_

#include "ScriptMgr.h"
#include <optional>

template<typename T>
using Optional = std::optional<T>;

template<typename ScriptName>
inline Optional<bool> IsValidBoolScript(std::function<bool(ScriptName*)> executeHook)
{
    if (ScriptRegistry<ScriptName>::ScriptPointerList.empty())
        return {};

    for (auto const& [scriptID, script] : ScriptRegistry<ScriptName>::ScriptPointerList)
    {
        if (executeHook(script))
            return true;
    }

    return false;
}

template<typename ScriptName, class T>
inline T* GetReturnAIScript(std::function<T* (ScriptName*)> executeHook)
{
    if (ScriptRegistry<ScriptName>::ScriptPointerList.empty())
        return nullptr;

    for (auto const& [scriptID, script] : ScriptRegistry<ScriptName>::ScriptPointerList)
    {
        if (T* scriptAI = executeHook(script))
        {
            return scriptAI;
        }
    }

    return nullptr;
}

template<typename ScriptName>
inline void ExecuteScript(std::function<void(ScriptName*)> executeHook)
{
    if (ScriptRegistry<ScriptName>::ScriptPointerList.empty())
        return;

    for (auto const& [scriptID, script] : ScriptRegistry<ScriptName>::ScriptPointerList)
    {
        executeHook(script);
    }
}

inline bool ReturnValidBool(Optional<bool> ret, bool need = false)
{
    return ret && *ret ? need : !need;
}

#define CALL_ENABLED_HOOKS(scriptType, hookType, action) \
    if (!ScriptRegistry<scriptType>::EnabledHooks[hookType].empty()) \
        for (auto const& script : ScriptRegistry<scriptType>::EnabledHooks[hookType]) { action; }

#define CALL_ENABLED_BOOLEAN_HOOKS(scriptType, hookType, action) \
    if (ScriptRegistry<scriptType>::EnabledHooks[hookType].empty()) \
        return true; \
    for (auto const& script : ScriptRegistry<scriptType>::EnabledHooks[hookType]) { if (action) return false; } \
    return true;

#define CALL_ENABLED_BOOLEAN_HOOKS_WITH_DEFAULT_FALSE(scriptType, hookType, action) \
    if (ScriptRegistry<scriptType>::EnabledHooks[hookType].empty()) \
        return false; \
    for (auto const& script : ScriptRegistry<scriptType>::EnabledHooks[hookType]) { if (action) return true; } \
    return false;

namespace
{
    typedef std::set<ScriptObject*> ExampleScriptContainer;
    ExampleScriptContainer ExampleScripts;
}

// This is the global static registry of scripts.
template<class TScript>
class ScriptRegistry
{
public:

    typedef std::map<uint32, TScript*> ScriptMap;
    typedef typename ScriptMap::iterator ScriptMapIterator;

    // The actual list of scripts. This will be accessed concurrently, so it must not be modified
    // after server startup.
    static ScriptMap ScriptPointerList;

    static ScriptRegistry* Instance()
    {
        static ScriptRegistry instance;
        return &instance;
    }

    static void AddScript(TScript* const script)
    {
        ASSERT(script);

        // See if the script is using the same memory as another script. If this happens, it means that
        // someone forgot to allocate new memory for a script.
        for (ScriptMapIterator it = ScriptPointerList.begin(); it != ScriptPointerList.end(); ++it)
        {
            if (it->second == script)
            {
                TC_LOG_ERROR("scripts", "Script '%s' has same memory pointer as '%s'.",
                    script->GetName().c_str(), it->second->GetName().c_str());

                return;
            }
        }

        // Get an ID for the script. An ID only exists if it's a script that is assigned in the database
        // through a script name (or similar).
        uint32 id = sObjectMgr->GetScriptId(script->GetName().c_str());
        if (id)
        {
            // Try to find an existing script.
            bool existing = false;
            for (ScriptMapIterator it = ScriptPointerList.begin(); it != ScriptPointerList.end(); ++it)
            {
                // If the script names match...
                if (it->second->GetName() == script->GetName())
                {
                    // ... It exists.
                    existing = true;
                    break;
                }
            }

            // If the script isn't assigned -> assign it!
            if (!existing)
            {
                ScriptPointerList[id] = script;
                sScriptMgr->IncreaseScriptCount();

#ifdef SCRIPTS
                UnusedScriptNamesContainer::iterator itr = std::lower_bound(UnusedScriptNames.begin(), UnusedScriptNames.end(), script->GetName());

                if (itr != UnusedScriptNames.end() && *itr == script->GetName())
                    UnusedScriptNames.erase(itr);
#endif
            }
            else
            {
                // If the script is already assigned -> delete it!
                TC_LOG_ERROR("scripts", "Script '%s' already assigned with the same script name, so the script can't work.",
                    script->GetName().c_str());

                ASSERT(false); // Error that should be fixed ASAP.
            }
        }
        else
        {

            // We're dealing with a code-only script; just add it.
            ScriptPointerList[_scriptIdCounter++] = script;
            sScriptMgr->IncreaseScriptCount();

        }

    }

    // Gets a script by its ID (assigned by ObjectMgr).
    static TScript* GetScriptById(uint32 id)
    {
        ScriptMapIterator it = ScriptPointerList.find(id);
        if (it != ScriptPointerList.end())
            return it->second;

        return NULL;
    }

private:

    // Counter used for code-only scripts.
    static uint32 _scriptIdCounter;
};

#endif // _SCRIPT_MGR_MACRO_H_

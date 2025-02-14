 #include "BotFactory.h"

 #include <random>
 #include <utility>
 
 #include "AccountMgr.h"
 #include "AiFactory.h"
 #include "ArenaTeam.h"
 #include "DBCStores.h"
 #include "DBCStructure.h"
 #include "GuildMgr.h"
 #include "Helper.h"
 #include "Item.h"
 #include "ItemVisitors.h"
 #include "Log.h"
 #include "LogCommon.h"
 #include "LootMgr.h"
 #include "MapManager.h"
 #include "ObjectMgr.h"
 #include "PetDefines.h"
 #include "Player.h"
 #include "PlayerbotAI.h"
 #include "PlayerbotAIConfig.h"
 #include "PlayerbotSpec.h"
 #include "Playerbots.h"
 #include "RandomPlayerbotFactory.h"
 #include "SharedDefines.h"
 #include "SpellAuraDefines.h"
 #include "World.h"
  
 BotFactory::BotFactory(Player* bot, uint32 level, uint32 itemQuality, uint32 gearScoreLimit)
     : level(level), bot(bot)
 {
     botAI = GET_PLAYERBOT_AI(bot);
 }
 
 void BotFactory::Init()
 {
     /*for (std::vector<uint32>::iterator i = sPlayerbotAIConfig->randomBotQuestIds.begin();
          i != sPlayerbotAIConfig->randomBotQuestIds.end(); ++i)
     {
         uint32 questId = *i;
         AddPrevQuests(questId, specialQuestIds);
         specialQuestIds.remove(questId);
         specialQuestIds.push_back(questId);
     }
     uint32 maxStoreSize = sSpellMgr->GetSpellInfoStoreSize();
     for (uint32 id = 1; id < maxStoreSize; ++id)
     {
         SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(id);
         if (!spellInfo)
             continue;
 
         if (id == 47181 || id == 50358 || id == 47242 || id == 52639 || id == 47147 || id == 7218)  // Test Enchant
             continue;
 
         uint32 requiredLevel = spellInfo->BaseLevel;
 
         for (uint8 j = 0; j < MAX_SPELL_EFFECTS; ++j)
         {
             if (spellInfo->Effects[j].Effect != SPELL_EFFECT_ENCHANT_ITEM)
                 continue;
 
             uint32 enchant_id = spellInfo->Effects[j].MiscValue;
             if (!enchant_id)
                 continue;
 
             SpellItemEnchantmentEntry const* enchant = sSpellItemEnchantmentStore.LookupEntry(enchant_id);
             if (!enchant || (enchant->Flags != PERM_ENCHANTMENT_SLOT && enchant->slot != TEMP_ENCHANTMENT_SLOT))
                 continue;
 
             // SpellInfo const* enchantSpell = sSpellMgr->GetSpellInfo(enchant->spellid[0]);
             // if (!enchantSpell)
             //     continue;
             if (strstr(spellInfo->SpellName[0], "Test"))
                 break;
 
             enchantSpellIdCache.push_back(id);
             break;
             // TC_LOG_INFO("playerbots", "Add {} to enchantment spells", id);
         }
     }
     TC_LOG_INFO("playerbots", "Loading {} enchantment spells", enchantSpellIdCache.size());
     for (auto iter = sSpellItemEnchantmentStore.begin(); iter != sSpellItemEnchantmentStore.end(); iter++)
     {
         uint32 gemId = iter->GemID;
         if (gemId == 0)
         {
             continue;
         }
         ItemTemplate const* proto = sObjectMgr->GetItemTemplate(gemId);
 
         if (proto->ItemLevel < 60)
             continue;
 
         if (proto->Flags & ITEM_FLAG_UNIQUE_EQUIPPABLE)
         {
             continue;
         }
         if (sRandomItemMgr->IsTestItem(gemId))
             continue;
 
         if (!proto || !sGemPropertiesStore.LookupEntry(proto->GemProperties))
         {
             continue;
         }
         // TC_LOG_INFO("playerbots", "Add {} to enchantment gems", gemId);
         enchantGemIdCache.push_back(gemId);
     }
     TC_LOG_INFO("playerbots", "Loading {} enchantment gems", enchantGemIdCache.size());*/
 }
 
 void BotFactory::Prepare()
 {
     if (bot->isDead())
         bot->ResurrectPlayer(1.0f, false);
 
     bot->CombatStop(true);
     uint32 currentLevel = bot->GetLevel();
     bot->GiveLevel(level);
     if (level != currentLevel)
     {
         bot->SetUInt32Value(PLAYER_FIELD_XP, 0);
     }
 }
 
 void BotFactory::Randomize(bool incremental)
 {
     TC_LOG_INFO("playerbots", "%s randomizing %s (level %u class = %s)...", (incremental ? "Incremental" : "Full"),
              bot->GetName().c_str(), level, ClassToString((Classes)bot->GetClass()).c_str());

     Prepare();
     bot->RemoveAllSpellCooldown();
 
     bot->GiveLevel(level);
     bot->InitStatsForLevel();
     CancelAuras();
 
     if (!incremental)
     {
         bot->RemovePet(PetRemoveMode::PET_REMOVE_ABANDON, PetRemoveFlag::PET_REMOVE_FLAG_NONE);
     }

     InitPet();

     // -- Select spec
     if (bot->GetLevel() >= 10)
    {
    }
 
     bot->SetMoney(urand(level * 100000, level * 5 * 100000));
     bot->SetHealth(bot->GetMaxHealth());
     bot->SetPower(POWER_MANA, bot->GetMaxPower(POWER_MANA));
     bot->SaveToDB(false);
 }
 
 void BotFactory::Refresh()
 {
     InitPet();
     bot->DurabilityRepairAll(false, 1.0f, false);
     if (bot->isDead())
         bot->ResurrectPlayer(1.0f, false);
     uint32 money = urand(level * 1000, level * 5 * 1000);
     if (bot->GetMoney() < money)
         bot->SetMoney(money);
 }
 
 void BotFactory::InitPet()
 {
     Pet* pet = bot->GetPet();
     if (!pet)
     {
         if (bot->GetClass() != CLASS_HUNTER)
             return;
 
         Map* map = bot->GetMap();
         if (!map)
             return;
 
         std::vector<uint32> ids;
 
         CreatureTemplateContainer const* creatures = sObjectMgr->GetCreatureTemplates();
         for (CreatureTemplateContainer::const_iterator itr = creatures->begin(); itr != creatures->end(); ++itr)
         {
             if (!itr->second.IsTameable(bot->CanTameExoticPets()))
                 continue;
 
             if (itr->second.minlevel > bot->GetLevel())
                 continue;
 
             ids.push_back(itr->first);
         }
 
         if (ids.empty())
         {
             TC_LOG_ERROR("playerbots", "No pets available for bot %s (%u level)", bot->GetName().c_str(), bot->GetLevel());
             return;
         }
 
         for (uint32 i = 0; i < 10; i++)
         {
             uint32 index = urand(0, ids.size() - 1);
             CreatureTemplate const* co = sObjectMgr->GetCreatureTemplate(ids[index]);
             if (!co)
                 continue;
             if (co->Name.size() > 21)
                 continue;
             uint32 guid = map->GenerateLowGuid<HighGuid::Pet>();
             uint32 pet_number = sObjectMgr->GeneratePetNumber();
             if (bot->GetPetStable() && bot->GetPetStable()->CurrentPet)
             {
                 bot->GetPetStable()->CurrentPet.value();
                 bot->RemovePet(nullptr, PET_SAVE_AS_CURRENT);
                 bot->RemovePet(nullptr, PET_SAVE_NOT_IN_SLOT);
             }
             if (bot->GetPetStable() && bot->GetPetStable()->GetUnslottedHunterPet())
             {
                 bot->GetPetStable()->UnslottedPets.clear();
                 bot->RemovePet(nullptr, PET_SAVE_AS_CURRENT);
                 bot->RemovePet(nullptr, PET_SAVE_NOT_IN_SLOT);
             }

             pet = bot->CreateTamedPetFrom(co->Entry, 0);
             if (!pet)
             {
                 continue;
             }
 
             // prepare visual effect for levelup
             pet->SetUInt32Value(UNIT_FIELD_LEVEL, bot->GetLevel() - 1);
 
             // add to world
             pet->GetMap()->AddToMap(pet->ToCreature());
 
             // visual effect for levelup
             pet->SetUInt32Value(UNIT_FIELD_LEVEL, bot->GetLevel());
 
             // caster have pet now
             bot->SetMinion(pet, true);
 
             pet->InitTalentForLevel();
 
             pet->SavePetToDB();
             bot->PetSpellInitialize();
             break;
         }
     }
 
     if (pet)
     {
         pet->InitStatsForLevel(bot->GetLevel());
         pet->SetLevel(bot->GetLevel());
         pet->SetHealth(pet->GetMaxHealth());
     }
     else
     {
         TC_LOG_ERROR("playerbots", "Cannot create pet for bot {}", bot->GetName().c_str());
         return;
     }
 
     // TC_LOG_INFO("playerbots", "Start make spell auto cast for {} spells. {} already auto casted.", pet->m_spells.size(),
     // pet->GetPetAutoSpellSize());
     for (PetSpellMap::const_iterator itr = pet->m_spells.begin(); itr != pet->m_spells.end(); ++itr)
     {
         if (itr->second.state == PETSPELL_REMOVED)
             continue;
 
         SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(itr->first);
         if (!spellInfo)
             continue;
 
         if (spellInfo->IsPassive())
         {
             continue;
         }
         pet->ToggleAutocast(spellInfo, true);
     }
 }
  
 void BotFactory::ClearEverything()
 {
     bot->GiveLevel(bot->GetClass() == CLASS_DEATH_KNIGHT ? sWorld->getIntConfig(CONFIG_START_HEROIC_PLAYER_LEVEL)
                                                          : sWorld->getIntConfig(CONFIG_START_PLAYER_LEVEL));
     bot->SetUInt32Value(PLAYER_FIELD_XP, 0);
     TC_LOG_INFO("playerbots", "Resetting player...");
     bot->ResetTalents(true);
 }
  
 ObjectGuid BotFactory::GetRandomBot()
 {
     GuidVector guids;
     for (std::vector<uint32>::iterator i = sPlayerbotAIConfig->randomBotAccounts.begin();
          i != sPlayerbotAIConfig->randomBotAccounts.end(); i++)
     {
         uint32 accountId = *i;
         if (!AccountMgr::GetCharactersCount(accountId))
             continue;
 
         CharacterDatabasePreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_CHARS_BY_ACCOUNT_ID);
         stmt->setUInt32(0, accountId);
         PreparedQueryResult result = CharacterDatabase.Query(stmt);
         if (!result)
             continue;
 
         do
         {
             Field* fields = result->Fetch();
             ObjectGuid guid = ObjectGuid::Create<HighGuid::Player>(fields[0].GetUInt32());
             if (!ObjectAccessor::FindPlayer(guid))
                 guids.push_back(guid);
         } while (result->NextRow());
     }
 
     if (guids.empty())
         return ObjectGuid::Empty;
 
     uint32 index = urand(0, guids.size() - 1);
     return guids[index];
 }
 
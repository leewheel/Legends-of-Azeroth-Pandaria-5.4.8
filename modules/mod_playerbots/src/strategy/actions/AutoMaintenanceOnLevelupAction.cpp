#include "AutoMaintenanceOnLevelupAction.h"

#include "GuildMgr.h"
#include "PlayerbotAIConfig.h"
#include "Playerbots.h"
#include "RandomPlayerbotMgr.h"
#include "SharedDefines.h"

bool AutoMaintenanceOnLevelupAction::Execute(Event event)
{
    AutoPickTalents();
    AutoUpgradeEquip();
    AutoTeleportForLevel();
    return true;
}

void AutoMaintenanceOnLevelupAction::AutoTeleportForLevel()
{
    /*if (!sPlayerbotAIConfig->autoTeleportForLevel || !sRandomPlayerbotMgr->IsRandomBot(bot))
    {
        return;
    }
    if (botAI->HasRealPlayerMaster())
    {
        return;
    }
    sRandomPlayerbotMgr->RandomTeleportForLevel(bot);*/
}

void AutoMaintenanceOnLevelupAction::AutoPickTalents()
{
    //if (!sPlayerbotAIConfig->autoPickTalents || !sRandomPlayerbotMgr->IsRandomBot(bot))
        //return;

    if ((bot->CalculateTalentsPoints() - bot->GetUsedTalentCount()) <= 0)
        return;

    //PlayerbotFactory factory(bot, bot->GetLevel());
    //factory.InitTalentsTree(true, true, true);
    //factory.InitPetTalents();
}

void AutoMaintenanceOnLevelupAction::AutoUpgradeEquip()
{
    /*if (!sPlayerbotAIConfig->autoUpgradeEquip || !sRandomPlayerbotMgr->IsRandomBot(bot))
    {
        return;
    }
    PlayerbotFactory factory(bot, bot->GetLevel());
    if (!sPlayerbotAIConfig->equipmentPersistence || bot->GetLevel() < sPlayerbotAIConfig->equipmentPersistenceLevel)
    {
        factory.InitEquipment(true);
    }
    factory.InitAmmo();*/
    return;
}


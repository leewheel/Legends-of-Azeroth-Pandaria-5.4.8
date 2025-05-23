/*
* This file is part of the Pandaria 5.4.8 Project. See THANKS file for Copyright information
*
* This program is free software; you can redistribute it and/or modify it
* under the terms of the GNU General Public License as published by the
* Free Software Foundation; either version 2 of the License, or (at your
* option) any later version.
*
* This program is distributed in the hope that it will be useful, but WITHOUT
* ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
* FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
* more details.
*
* You should have received a copy of the GNU General Public License along
* with this program. If not, see <http://www.gnu.org/licenses/>.
*/

#include "Common.h"
#include "GameTime.h"
#include "ObjectMgr.h"
#include "Player.h"
#include "World.h"
#include "WorldPacket.h"
#include "DatabaseEnv.h"
#include "DBCStores.h"
#include "ScriptMgr.h"
#include "AccountMgr.h"
#include "AuctionHouseMgr.h"
#include "AuctionHouseBot.h"
#include "Item.h"
#include "Language.h"
#include "Log.h"
#include "Config.h"
#include <vector>

enum eAuctionHouse
{
    AH_MINIMUM_DEPOSIT = 100
};

AuctionHouseMgr::AuctionHouseMgr() { }

AuctionHouseMgr::~AuctionHouseMgr()
{
    for (ItemMap::iterator itr = mAitems.begin(); itr != mAitems.end(); ++itr)
        delete itr->second;
}

AuctionHouseMgr* AuctionHouseMgr::instance()
{
    static AuctionHouseMgr instance;
    return &instance;
}

AuctionHouseObject* AuctionHouseMgr::GetAuctionsMap(uint32 factionTemplateId)
{
    if (sWorld->getBoolConfig(CONFIG_ALLOW_TWO_SIDE_INTERACTION_AUCTION))
        return &mNeutralAuctions;

    // teams have linked auction houses
    FactionTemplateEntry const* factionTemplate = sFactionTemplateStore.LookupEntry(factionTemplateId);
    if (!factionTemplate)
        return &mNeutralAuctions;
    else if (factionTemplate->ourMask & FACTION_MASK_ALLIANCE)
        return &mAllianceAuctions;
    else if (factionTemplate->ourMask & FACTION_MASK_HORDE)
        return &mHordeAuctions;
    else
        return &mNeutralAuctions;
}

AuctionHouseObject* AuctionHouseMgr::GetAuctionsMapByHouseId(uint8 auctionHouseId)
{
    if (sWorld->getBoolConfig(CONFIG_ALLOW_TWO_SIDE_INTERACTION_AUCTION))
        return &mNeutralAuctions;

    switch(auctionHouseId)
    {
        case AUCTIONHOUSE_ALLIANCE : return &mAllianceAuctions;
        case AUCTIONHOUSE_HORDE : return &mHordeAuctions;
        default : return &mNeutralAuctions;
    }
}

uint32 AuctionHouseMgr::GetAuctionDeposit(AuctionHouseEntry const* entry, uint32 time, Item* pItem, uint32 count)
{
    uint32 MSV = pItem->GetTemplate()->GetSellPrice();

    if (MSV <= 0)
        return AsUnderlyingType(AH_MINIMUM_DEPOSIT) * sWorld->getRate(RATE_AUCTION_DEPOSIT);

    float multiplier = CalculatePct(float(entry->depositPercent), 3);
    uint32 timeHr = (((time / 60) / 60) / 12);
    uint64 deposit = uint64(MSV * multiplier * sWorld->getRate(RATE_AUCTION_DEPOSIT));
    float remainderbase = float(MSV * multiplier * sWorld->getRate(RATE_AUCTION_DEPOSIT)) - deposit;

    deposit *= timeHr * count;

    int i = count;
    while (i > 0 && (remainderbase * i) != uint32(remainderbase * i))
        i--;

    if (i)
        deposit += remainderbase * i * timeHr;

    TC_LOG_DEBUG("auctionHouse", "MSV:        %u", MSV);
    TC_LOG_DEBUG("auctionHouse", "Items:      %u", count);
    TC_LOG_DEBUG("auctionHouse", "Multiplier: %f", multiplier);
    TC_LOG_DEBUG("auctionHouse", "Deposit:    " UI64FMTD, deposit);
    TC_LOG_DEBUG("auctionHouse", "Deposit rm: %f", remainderbase * count);

    if (deposit < AsUnderlyingType(AH_MINIMUM_DEPOSIT) * sWorld->getRate(RATE_AUCTION_DEPOSIT))
        return AsUnderlyingType(AH_MINIMUM_DEPOSIT) * sWorld->getRate(RATE_AUCTION_DEPOSIT);
    else
        return deposit;
}

//does not clear ram
void AuctionHouseMgr::SendAuctionWonMail(AuctionEntry* auction, CharacterDatabaseTransaction& trans)
{
    Item* pItem = GetAItem(auction->itemGUIDLow);
    if (!pItem)
        return;

    uint32 bidderAccId = 0;
    Player* bidder = ObjectAccessor::FindConnectedPlayer(auction->bidder);
    // data for gm.log
    std::string bidderName;
    bool logGmTrade = false;

    if (bidder)
    {
        bidderAccId = bidder->GetSession()->GetAccountId();
        bidderName = bidder->GetName();
        logGmTrade = bidder->GetSession()->GetSecurity() >= SEC_MODERATOR;
    }
    else
    {
        bidderAccId = sObjectMgr->GetPlayerAccountIdByGUID(auction->bidder);
        logGmTrade = AccountMgr::GetSecurity(bidderAccId, realm.Id.Realm) >= SEC_MODERATOR;

        if (logGmTrade && !sObjectMgr->GetPlayerNameByGUID(auction->bidder, bidderName))
            bidderName = sObjectMgr->GetTrinityStringForDBCLocale(LANG_UNKNOWN);
    }

    if (logGmTrade)
    {
        std::string ownerName;
        if (!sObjectMgr->GetPlayerNameByGUID(auction->owner, ownerName))
            ownerName = sObjectMgr->GetTrinityStringForDBCLocale(LANG_UNKNOWN);

        uint32 ownerAccId = sObjectMgr->GetPlayerAccountIdByGUID(auction->owner);

        sLog->outCommand(bidderAccId, "GM %s (Account: %u) won item in auction: %s (Entry: %u Count: %u) and pay money: %u. Original owner %s (Account: %u)",
            bidderName.c_str(), bidderAccId, pItem->GetTemplate()->Name1.c_str(), pItem->GetEntry(), pItem->GetCount(), auction->bid, ownerName.c_str(), ownerAccId);
    }

    // receiver exist
    if (bidder || bidderAccId && !sAuctionBotConfig->IsBotChar(auction->bidder))
    {
        // set owner to bidder (to prevent delete item with sender char deleting)
        // owner in `data` will set at mail receive and item extracting
        CharacterDatabasePreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_UPD_ITEM_OWNER);
        stmt->setUInt32(0, auction->bidder);
        stmt->setUInt32(1, pItem->GetGUID().GetCounter());
        trans->Append(stmt);

        if (bidder)
        {
            bidder->GetSession()->SendAuctionBidderNotification(auction->GetHouseId(), auction->Id, auction->bidder, 0, 0, auction->itemEntry);
            // FIXME: for offline player need also
            bidder->UpdateAchievementCriteria(ACHIEVEMENT_CRITERIA_TYPE_WON_AUCTIONS, 1);
        }

        MailDraft(auction->BuildAuctionMailSubject(AUCTION_WON), AuctionEntry::BuildAuctionMailBody(auction->owner, auction->bid, auction->buyout, 0, 0))
            .AddItem(pItem)
            .SendMailTo(trans, MailReceiver(bidder, auction->bidder), auction, MAIL_CHECK_MASK_COPIED);
    }
}

void AuctionHouseMgr::SendAuctionSalePendingMail(AuctionEntry* auction, CharacterDatabaseTransaction& trans)
{
    Player* owner = ObjectAccessor::FindConnectedPlayer(auction->owner);
    uint32 owner_accId = sObjectMgr->GetPlayerAccountIdByGUID(auction->owner);
    // owner exist (online or offline)
    if (owner || owner_accId && !sAuctionBotConfig->IsBotChar(auction->owner))
        MailDraft(auction->BuildAuctionMailSubject(AUCTION_SALE_PENDING), AuctionEntry::BuildAuctionMailBody(auction->bidder, auction->bid, auction->buyout, auction->deposit, auction->GetAuctionCut()))
            .SendMailTo(trans, MailReceiver(owner, auction->owner), auction, MAIL_CHECK_MASK_COPIED);
}

//call this method to send mail to auction owner, when auction is successful, it does not clear ram
void AuctionHouseMgr::SendAuctionSuccessfulMail(AuctionEntry* auction, CharacterDatabaseTransaction& trans)
{
    Player* owner = ObjectAccessor::FindConnectedPlayer(auction->owner);
    uint32 owner_accId = sObjectMgr->GetPlayerAccountIdByGUID(auction->owner);
    // owner exist
    if (owner || owner_accId && !sAuctionBotConfig->IsBotChar(auction->owner))
    {
        uint32 profit = auction->bid + auction->deposit - auction->GetAuctionCut();

        //FIXME: what do if owner offline
        if (owner)
        {
            owner->UpdateAchievementCriteria(ACHIEVEMENT_CRITERIA_TYPE_GOLD_EARNED_BY_AUCTIONS, profit);
            owner->UpdateAchievementCriteria(ACHIEVEMENT_CRITERIA_TYPE_HIGHEST_AUCTION_SOLD, auction->bid);
            //send auction owner notification, bidder must be current!
            owner->GetSession()->SendAuctionOwnerNotification(auction);
        }

        MailDraft(auction->BuildAuctionMailSubject(AUCTION_SUCCESSFUL), AuctionEntry::BuildAuctionMailBody(auction->bidder, auction->bid, auction->buyout, auction->deposit, auction->GetAuctionCut()))
            .AddMoney(profit)
            .SendMailTo(trans, MailReceiver(owner, auction->owner), auction, MAIL_CHECK_MASK_COPIED, sWorld->getIntConfig(CONFIG_MAIL_DELIVERY_DELAY));
    }
}

//does not clear ram
void AuctionHouseMgr::SendAuctionExpiredMail(AuctionEntry* auction, CharacterDatabaseTransaction& trans)
{
    //return an item in auction to its owner by mail
    Item* pItem = GetAItem(auction->itemGUIDLow);
    if (!pItem)
        return;

    Player* owner = ObjectAccessor::FindConnectedPlayer(auction->owner);
    uint32 owner_accId = sObjectMgr->GetPlayerAccountIdByGUID(auction->owner);
    // owner exist
    if (owner || owner_accId && !sAuctionBotConfig->IsBotChar(auction->owner))
    {
        if (owner)
            owner->GetSession()->SendAuctionOwnerNotification(auction);

        MailDraft(auction->BuildAuctionMailSubject(AUCTION_EXPIRED), AuctionEntry::BuildAuctionMailBody(ObjectGuid::Empty, 0, auction->buyout, auction->deposit, 0))
            .AddItem(pItem)
            .SendMailTo(trans, MailReceiver(owner, auction->owner), auction, MAIL_CHECK_MASK_COPIED, 0);
    }
    else
    {
        // owner doesn't exist, delete the item
        sAuctionMgr->RemoveAItem(auction->itemGUIDLow, true, &trans);
    }
}

//this function sends mail to old bidder
void AuctionHouseMgr::SendAuctionOutbiddedMail(AuctionEntry* auction, uint64 newPrice, Player* newBidder, CharacterDatabaseTransaction& trans)
{
    Player* oldBidder = ObjectAccessor::FindPlayer(auction->bidder);

    uint32 oldBidder_accId = 0;
    if (!oldBidder)
        oldBidder_accId = sObjectMgr->GetPlayerAccountIdByGUID(auction->bidder);

    // old bidder exist
    if (oldBidder || oldBidder_accId && !sAuctionBotConfig->IsBotChar(auction->bidder))
    {
        if (oldBidder && newBidder)
            oldBidder->GetSession()->SendAuctionBidderNotification(auction->GetHouseId(), auction->Id, newBidder->GetGUID(), newPrice, auction->GetAuctionOutBid(), auction->itemEntry);

        MailDraft(auction->BuildAuctionMailSubject(AUCTION_OUTBIDDED), AuctionEntry::BuildAuctionMailBody(auction->owner, auction->bid, auction->buyout, auction->deposit, auction->GetAuctionCut()))
            .AddMoney(auction->bid)
            .SendMailTo(trans, MailReceiver(oldBidder, auction->bidder), auction, MAIL_CHECK_MASK_COPIED);
    }
}

//this function sends mail, when auction is cancelled to old bidder
void AuctionHouseMgr::SendAuctionCancelledToBidderMail(AuctionEntry* auction, CharacterDatabaseTransaction& trans, Item* item)
{
    Player* bidder = ObjectAccessor::FindConnectedPlayer(auction->bidder);

    uint32 bidder_accId = 0;
    if (!bidder)
        bidder_accId = sObjectMgr->GetPlayerAccountIdByGUID(auction->bidder);

//     if (bidder)
//         bidder->GetSession()->SendAuctionRemovedNotification(auction->Id, auction->itemEntry, item->GetItemRandomPropertyId());

    // bidder exist
    if (bidder || bidder_accId && !sAuctionBotConfig->IsBotChar(auction->bidder))
        MailDraft(auction->BuildAuctionMailSubject(AUCTION_CANCELLED_TO_BIDDER), AuctionEntry::BuildAuctionMailBody(auction->owner, auction->bid, auction->buyout, auction->deposit, 0))
            .AddMoney(auction->bid)
            .SendMailTo(trans, MailReceiver(bidder, auction->bidder), auction, MAIL_CHECK_MASK_COPIED);
}

void AuctionHouseMgr::LoadAuctionItems()
{
    uint32 oldMSTime = getMSTime();

    // need to clear in case we are reloading
    if (!mAitems.empty())
    {
        for (ItemMap::iterator itr = mAitems.begin(); itr != mAitems.end(); ++itr)
            delete itr->second;

        mAitems.clear();
    }

    // data needs to be at first place for Item::LoadFromDB
    CharacterDatabasePreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_AUCTION_ITEMS);
    PreparedQueryResult result = CharacterDatabase.Query(stmt);

    if (!result)
    {
        TC_LOG_INFO("server.loading", ">> Loaded 0 auction items. DB table `auctionhouse` or `item_instance` is empty!");

        return;
    }

    uint32 count = 0;

    do
    {
        Field* fields = result->Fetch();

        ObjectGuid::LowType item_guid = fields[18].GetUInt32();
        uint32 itemEntry    = fields[19].GetUInt32();

        ItemTemplate const* proto = sObjectMgr->GetItemTemplate(itemEntry);
        if (!proto)
        {
            TC_LOG_ERROR("misc", "AuctionHouseMgr::LoadAuctionItems: Unknown item (GUID: %u id: #%u) in auction, skipped.", item_guid, itemEntry);
            continue;
        }

        Item* item = NewItemOrBag(proto);
        if (!item->LoadFromDB(item_guid, ObjectGuid::Empty, fields, itemEntry))
        {
            delete item;
            continue;
        }
        AddAItem(item);

        ++count;
    }
    while (result->NextRow());

    TC_LOG_INFO("server.loading", ">> Loaded %u auction items in %u ms", count, GetMSTimeDiffToNow(oldMSTime));

}

void AuctionHouseMgr::LoadAuctions()
{
    uint32 oldMSTime = getMSTime();

    CharacterDatabasePreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_AUCTIONS);
    PreparedQueryResult result = CharacterDatabase.Query(stmt);

    if (!result)
    {
        TC_LOG_INFO("server.loading", ">> Loaded 0 auctions. DB table `auctionhouse` is empty.");

        return;
    }

    // parse bidder list
    std::unordered_map<uint32, std::unordered_set<ObjectGuid>> biddersByAuction;
    CharacterDatabasePreparedStatement* stmt2 = CharacterDatabase.GetPreparedStatement(CHAR_SEL_AUCTION_BIDDERS);

    uint32 countBidders = 0;
    if (PreparedQueryResult resultBidders = CharacterDatabase.Query(stmt2))
    {
        do
        {
            Field* fields = resultBidders->Fetch();
            biddersByAuction[fields[0].GetUInt32()].insert(ObjectGuid::Create<HighGuid::Player>(fields[1].GetUInt32()));
            ++countBidders;
        }
        while (resultBidders->NextRow());
    }

    // parse auctions from db
    uint32 countAuctions = 0;
    CharacterDatabaseTransaction trans = CharacterDatabase.BeginTransaction();
    do
    {
        Field* fields = result->Fetch();

        AuctionEntry* aItem = new AuctionEntry();
        if (!aItem->LoadFromDB(fields))
        {
            aItem->DeleteFromDB(trans);
            delete aItem;
            continue;
        }

        auto it = biddersByAuction.find(aItem->Id);
        if (it != biddersByAuction.end())
            aItem->bidders = std::move(it->second);

        GetAuctionsMapByHouseId(aItem->houseId)->AddAuction(aItem);
        ++countAuctions;
    } while (result->NextRow());

    CharacterDatabase.CommitTransaction(trans);

    TC_LOG_INFO("server.loading", ">> Loaded %u auctions with %u bidders in %u ms", countAuctions, countBidders, GetMSTimeDiffToNow(oldMSTime));
}

void AuctionHouseMgr::AddAItem(Item* it)
{
    ASSERT(it);
    ASSERT(mAitems.find(it->GetGUID().GetCounter()) == mAitems.end());
    mAitems[it->GetGUID().GetCounter()] = it;
}

bool AuctionHouseMgr::RemoveAItem(ObjectGuid::LowType id, bool deleteItem /*= false*/, CharacterDatabaseTransaction* trans /*= nullptr*/)
{
    ItemMap::iterator i = mAitems.find(id);
    if (i == mAitems.end())
        return false;

    if (deleteItem)
    {
        ASSERT(trans);
        i->second->FSetState(ITEM_REMOVED);
        i->second->SaveToDB(*trans);
    }

    mAitems.erase(i);
    return true;
}

bool AuctionHouseMgr::PendingAuctionAdd(Player* player, AuctionEntry* aEntry, Item* item)
{
    PlayerAuctions* thisAH;
    auto itr = pendingAuctionMap.find(player->GetGUID());
    if (itr != pendingAuctionMap.end())
    {
        thisAH = itr->second.first;

        // Get deposit so far
        uint64 totalDeposit = 0;
        for (AuctionEntry const* thisAuction : *thisAH)
            totalDeposit += GetAuctionDeposit(thisAuction->auctionHouseEntry, thisAuction->etime, item, thisAuction->itemCount);

        // Add this deposit
        totalDeposit += GetAuctionDeposit(aEntry->auctionHouseEntry, aEntry->etime, item, aEntry->itemCount);

        if (!player->HasEnoughMoney(totalDeposit))
            return false;
    }
    else
    {
        thisAH = new PlayerAuctions;
        pendingAuctionMap[player->GetGUID()] = AuctionPair(thisAH, 0);
    }
    thisAH->push_back(aEntry);
    return true;
}

uint32 AuctionHouseMgr::PendingAuctionCount(Player const* player) const
{
    auto const itr = pendingAuctionMap.find(player->GetGUID());
    if (itr != pendingAuctionMap.end())
        return itr->second.first->size();

    return 0;
}

void AuctionHouseMgr::PendingAuctionProcess(Player* player)
{
    auto iterMap = pendingAuctionMap.find(player->GetGUID());
    if (iterMap == pendingAuctionMap.end())
        return;

    PlayerAuctions* thisAH = iterMap->second.first;

    CharacterDatabaseTransaction trans = CharacterDatabase.BeginTransaction();

    uint32 totalItems = 0;
    for (auto itrAH = thisAH->begin(); itrAH != thisAH->end(); ++itrAH)
    {
        AuctionEntry* AH = (*itrAH);
        totalItems += AH->itemCount;
    }

    uint64 totaldeposit = 0;
    auto itr = (*thisAH->begin());

    if (Item* item = GetAItem(itr->itemGUIDLow))
        totaldeposit = GetAuctionDeposit(itr->auctionHouseEntry, itr->etime, item, totalItems);

    uint64 depositremain = totaldeposit;
    for (auto itr = thisAH->begin(); itr != thisAH->end(); ++itr)
    {
        AuctionEntry* AH = (*itr);

        if (next(itr) == thisAH->end())
            AH->deposit = depositremain;
        else
        {
            AH->deposit = totaldeposit / thisAH->size();
            depositremain -= AH->deposit;
        }

        AH->DeleteFromDB(trans);

        AH->SaveToDB(trans);
    }

    CharacterDatabase.CommitTransaction(trans);
    pendingAuctionMap.erase(player->GetGUID());
    delete thisAH;
    player->ModifyMoney(-int64(totaldeposit));
}

void AuctionHouseMgr::UpdatePendingAuctions()
{
    for (auto itr = pendingAuctionMap.begin(); itr != pendingAuctionMap.end();)
    {
        ObjectGuid playerGUID = itr->first;
        if (Player* player = ObjectAccessor::FindConnectedPlayer(playerGUID))
        {
            // Check if there were auctions since last update process if not
            if (PendingAuctionCount(player) == itr->second.second)
            {
                ++itr;
                PendingAuctionProcess(player);
            }
            else
            {
                ++itr;
                pendingAuctionMap[playerGUID].second = PendingAuctionCount(player);
            }
        }
        else
        {
            // Expire any auctions that we couldn't get a deposit for
            TC_LOG_WARN("auctionHouse", "Player %s was offline, unable to retrieve deposit!", playerGUID.ToString().c_str());
            PlayerAuctions* thisAH = itr->second.first;
            ++itr;
            CharacterDatabaseTransaction trans = CharacterDatabase.BeginTransaction();
            for (auto AHitr = thisAH->begin(); AHitr != thisAH->end();)
            {
                AuctionEntry* AH = (*AHitr);
                ++AHitr;
                AH->expire_time = GameTime::GetGameTime();
                AH->DeleteFromDB(trans);
                AH->SaveToDB(trans);
            }
            CharacterDatabase.CommitTransaction(trans);
            pendingAuctionMap.erase(playerGUID);
            delete thisAH;
        }
    }
}

void AuctionHouseMgr::Update()
{
    mHordeAuctions.Update();
    mAllianceAuctions.Update();
    mNeutralAuctions.Update();
}

AuctionHouseEntry const* AuctionHouseMgr::GetAuctionHouseEntry(uint32 factionTemplateId)
{
    uint32 houseid = AUCTIONHOUSE_NEUTRAL; // goblin auction house

    if (!sWorld->getBoolConfig(CONFIG_ALLOW_TWO_SIDE_INTERACTION_AUCTION))
    {
        // FIXME: found way for proper auctionhouse selection by another way
        // AuctionHouse.dbc have faction field with _player_ factions associated with auction house races.
        // but no easy way convert creature faction to player race faction for specific city
        FactionTemplateEntry const* u_entry = sFactionTemplateStore.LookupEntry(factionTemplateId);
        if (!u_entry)
            houseid = AUCTIONHOUSE_NEUTRAL; // goblin auction house
        else if (u_entry->ourMask & FACTION_MASK_ALLIANCE)
            houseid = AUCTIONHOUSE_ALLIANCE; // human auction house
        else if (u_entry->ourMask & FACTION_MASK_HORDE)
            houseid = AUCTIONHOUSE_HORDE; // orc auction house
        else
            houseid = AUCTIONHOUSE_NEUTRAL; // goblin auction house
    }

    return sAuctionHouseStore.LookupEntry(houseid);
}

AuctionHouseEntry const* AuctionHouseMgr::GetAuctionHouseEntryFromHouse(uint8 houseId)
{
    return (sWorld->getBoolConfig(CONFIG_ALLOW_TWO_SIDE_INTERACTION_AUCTION)) ? sAuctionHouseStore.LookupEntry(AUCTIONHOUSE_NEUTRAL) : sAuctionHouseStore.LookupEntry(houseId);
}

void AuctionHouseObject::AddAuction(AuctionEntry* auction)
{
    ASSERT(auction);

    AuctionsMap[auction->Id] = auction;
    sScriptMgr->OnAuctionAdd(this, auction);
}

bool AuctionHouseObject::RemoveAuction(AuctionEntry* auction)
{
    bool wasInMap = AuctionsMap.erase(auction->Id) ? true : false;

    sScriptMgr->OnAuctionRemove(this, auction);

    // we need to delete the entry, it is not referenced any more
    delete auction;
    return wasInMap;
}

void AuctionHouseObject::Update()
{
    time_t curTime = GameTime::GetGameTime();
    ///- Handle expired auctions

    // If storage is empty, no need to update. next == NULL in this case.
    if (AuctionsMap.empty())
        return;

    // Clear expired throttled players
    for (PlayerGetAllThrottleMap::const_iterator itr = GetAllThrottleMap.begin(); itr != GetAllThrottleMap.end();)
    {
        if (itr->second.NextAllowedReplication <= curTime)
            itr = GetAllThrottleMap.erase(itr);
        else
            ++itr;
    }

    CharacterDatabaseTransaction trans = CharacterDatabase.BeginTransaction();

    for (AuctionEntryMap::iterator it = AuctionsMap.begin(); it != AuctionsMap.end();)
    {
        // from auctionhousehandler.cpp, creates auction pointer & player pointer
        AuctionEntry* auction = it->second;
        // Increment iterator due to AuctionEntry deletion
        ++it;

        ///- filter auctions expired on next update
        if (auction->expire_time > curTime + 60)
            continue;

        ///- Either cancel the auction if there was no bidder
        if (auction->bidder == 0 && auction->bid == 0)
        {
            sAuctionMgr->SendAuctionExpiredMail(auction, trans);
            sScriptMgr->OnAuctionExpire(this, auction);
        }
        ///- Or perform the transaction
        else
        {
            //we should send an "item sold" message if the seller is online
            //we send the item to the winner
            //we send the money to the seller
            sAuctionMgr->SendAuctionSuccessfulMail(auction, trans);
            sAuctionMgr->SendAuctionWonMail(auction, trans);
            sScriptMgr->OnAuctionSuccessful(this, auction);
        }

        ///- In any case clear the auction
        auction->DeleteFromDB(trans);

        sAuctionMgr->RemoveAItem(auction->itemGUIDLow);
        RemoveAuction(auction);
    }

    // Run DB changes
    CharacterDatabase.CommitTransaction(trans);
}

void AuctionHouseObject::BuildListBidderItems(WorldPacket& data, Player* player, uint32& count, uint32& totalcount)
{
    for (AuctionEntryMap::const_iterator itr = AuctionsMap.begin(); itr != AuctionsMap.end(); ++itr)
    {
        AuctionEntry* Aentry = itr->second;
        if (Aentry && Aentry->bidders.find(player->GetGUID()) != Aentry->bidders.end())
        {
            if (itr->second->BuildAuctionInfo(data))
                ++count;

            ++totalcount;
        }
    }
}

void AuctionHouseObject::BuildListOwnerItems(WorldPacket& data, Player* player, uint32& count, uint32& totalcount)
{
    for (AuctionEntryMap::const_iterator itr = AuctionsMap.begin(); itr != AuctionsMap.end(); ++itr)
    {
        AuctionEntry* Aentry = itr->second;
        if (Aentry && Aentry->owner == player->GetGUID())
        {
            if (Aentry->BuildAuctionInfo(data))
                ++count;

            ++totalcount;
        }
    }
}

void AuctionHouseObject::BuildListAuctionItems(WorldPacket& data, Player* player,
    std::wstring const& wsearchedname, uint32 listfrom, uint8 levelmin, uint8 levelmax, uint8 usable,
    uint32 inventoryType, uint32 itemClass, uint32 itemSubClass, uint32 quality,
    uint32& count, uint32& totalcount, bool getall)
{
    time_t curTime = GameTime::GetGameTime();

    PlayerGetAllThrottleMap::const_iterator itr = GetAllThrottleMap.find(player->GetGUID());
    time_t throttleTime = itr != GetAllThrottleMap.end() ? itr->second.NextAllowedReplication : curTime;

    if (getall && throttleTime <= curTime)
    {
        for (AuctionEntryMap::const_iterator itr = AuctionsMap.begin(); itr != AuctionsMap.end(); ++itr)
        {
            AuctionEntry* Aentry = itr->second;
            // Skip expired auctions
            if (Aentry->expire_time < curTime)
                continue;

            Item* item = sAuctionMgr->GetAItem(Aentry->itemGUIDLow);
            if (!item)
                continue;

            ++count;
            ++totalcount;
            Aentry->BuildAuctionInfo(data, item);

            if (count >= MAX_GETALL_RETURN)
                break;
        }
        GetAllThrottleMap[player->GetGUID()].NextAllowedReplication = curTime + sWorld->getIntConfig(CONFIG_AUCTION_GETALL_DELAY);
        return;
    }

    for (AuctionEntryMap::const_iterator itr = AuctionsMap.begin(); itr != AuctionsMap.end(); ++itr)
    {
        AuctionEntry* Aentry = itr->second;
        // Skip expired auctions
        if (Aentry->expire_time < curTime)
            continue;

        Item *item = sAuctionMgr->GetAItem(Aentry->itemGUIDLow);
        if (!item)
            continue;

        ItemTemplate const *proto = item->GetTemplate();

        if (itemClass != 0xffffffff && proto->GetClass() != itemClass)
            continue;

        if (itemSubClass != 0xffffffff && proto->GetSubClass() != itemSubClass)
            continue;

        if (inventoryType != 0xffffffff && proto->GetInventoryType() != inventoryType)
            continue;

        if (quality != 0xffffffff && proto->GetQuality() != quality)
            continue;

        if (levelmin != 0x00 && (proto->GetRequiredLevel() < levelmin || (levelmax != 0 && proto->GetRequiredLevel() > levelmax)))
            continue;

        if (usable != 0x00 && player->CanUseItem(item) != EQUIP_ERR_OK)
            continue;

        // Allow search by suffix (ie: of the Monkey) or partial name (ie: Monkey)
        if (!wsearchedname.empty())
        {
            std::string name = proto->Name1;
            if (name.empty())
                continue;

            // DO NOT use GetItemEnchantMod(proto->RandomProperty) as it may return a result
            //  that matches the search but it may not equal item->GetItemRandomPropertyId()
            //  used in BuildAuctionInfo() which then causes wrong items to be listed
            int32 propRefID = item->GetItemRandomPropertyId();

            if (propRefID)
            {
                // Append the suffix to the name (ie: of the Monkey) if one exists
                // These are found in ItemRandomSuffix.dbc and ItemRandomProperties.dbc
                //  even though the DBC names seem misleading

                char* suffix = nullptr;

                if (propRefID < 0)
                {
                    ItemRandomSuffixEntry const* itemRandSuffix = sItemRandomSuffixStore.LookupEntry(-propRefID);
                    if (itemRandSuffix)
                        suffix = const_cast<char *>(itemRandSuffix->nameSuffix[player->GetSession()->GetSessionDbcLocale()]);

                }
                else
                {
                    ItemRandomPropertiesEntry const* itemRandProp = sItemRandomPropertiesStore.LookupEntry(propRefID);
                    if (itemRandProp)
                        suffix = const_cast<char *>(itemRandProp->nameSuffix[player->GetSession()->GetSessionDbcLocale()]);
                }

                // dbc local name
                if (suffix)
                {
                    // Append the suffix (ie: of the Monkey) to the name using localization
                    // or default enUS if localization is invalid
                    name += ' ';
                    name += suffix;
                }
            }

            // Perform the search (with or without suffix)
            if (!Utf8FitTo(name, wsearchedname))
                continue;
        }

        // Add the item if no search term or if entered search term was found
        if (count < 50 && totalcount >= listfrom)
        {
            ++count;
            Aentry->BuildAuctionInfo(data, item);
        }
        ++totalcount;
    }
}

//this function inserts to WorldPacket auction's data
bool AuctionEntry::BuildAuctionInfo(WorldPacket& data, Item* sourceItem) const
{
    Item* item = (sourceItem) ? sourceItem : sAuctionMgr->GetAItem(itemGUIDLow);
    if (!item)
    {
        TC_LOG_ERROR("misc", "AuctionEntry::BuildAuctionInfo: Auction %u has a non-existent item: %u", Id, itemGUIDLow);
        return false;
    }
    data << uint32(Id);
    data << uint32(item->GetEntry());

    for (uint8 i = 0; i < PROP_ENCHANTMENT_SLOT_0; ++i) // PROP_ENCHANTMENT_SLOT_0 = 10
    {
        data << uint32(item->GetEnchantmentId(EnchantmentSlot(i)));
        data << uint32(item->GetEnchantmentDuration(EnchantmentSlot(i)));
        data << uint32(item->GetEnchantmentCharges(EnchantmentSlot(i)));
    }

    uint32 mask = item->GetUInt32Value(ITEM_FIELD_MODIFIERS_MASK);
    data << int32(mask);
    for (uint32 i = 0; i < ITEM_MODIFIER_INDEX_MAX; ++i)
        if (mask & (1 << i))
            data << item->GetDynamicUInt32Value(ITEM_DYNAMIC_MODIFIERS, i);

    data << int32(item->GetItemRandomPropertyId());                 // Random item property id
    data << uint32(item->GetItemSuffixFactor());                    // SuffixFactor
    data << uint32(item->GetCount());                               // item->count
    data << uint32(item->GetSpellCharges());                        // item->charge FFFFFFF
    data << uint32(0);                                              // Unknown
    data << owner.GetRawValue();                                    // Auction->owner
    data << uint64(startbid);                                       // Auction->startbid (not sure if useful)
    data << uint64(bid ? GetAuctionOutBid() : 0);
    // Minimal outbid
    data << uint64(buyout);                                         // Auction->buyout
    data << uint32((expire_time - GameTime::GetGameTime()) * IN_MILLISECONDS);   // time left
    data << bidder.GetRawValue();                                   // auction->bidder current
    data << uint64(bid);                                            // current bid
    return true;
}

void AuctionHouseObject::BuildReplicate(WorldPackets::AuctionHouse::AuctionReplicateResponse& auctionReplicateResult, Player* player,
    uint32 global, uint32 cursor, uint32 tombstone, uint32 count)
{
    time_t curTime = sWorld->GetGameTime();

    auto throttleItr = GetAllThrottleMap.find(player->GetGUID());
    if (throttleItr != GetAllThrottleMap.end())
    {
        if (throttleItr->second.Global != global || throttleItr->second.Cursor != cursor || throttleItr->second.Tombstone != tombstone)
            return;

        if (!throttleItr->second.IsReplicationInProgress() && throttleItr->second.NextAllowedReplication > curTime)
            return;
    }
    else
    {
        throttleItr = GetAllThrottleMap.insert({ player->GetGUID(), PlayerGetAllThrottleData{} }).first;
        throttleItr->second.NextAllowedReplication = curTime + sWorld->getIntConfig(CONFIG_AUCTION_GETALL_DELAY);
        throttleItr->second.Global = uint32(curTime);
    }

    if (AuctionsMap.empty() || !count)
        return;

    auto itr = AuctionsMap.upper_bound(cursor);
    for (; itr != AuctionsMap.end(); ++itr)
    {
        AuctionEntry* auction = itr->second;
        if (auction->expire_time < curTime)
            continue;

        Item* item = sAuctionMgr->GetAItem(auction->itemGUIDLow);
        if (!item)
            continue;

        auction->BuildAuctionInfo(auctionReplicateResult.Items, true, item);
        if (!--count)
            break;
    }

    auctionReplicateResult.ChangeNumberGlobal = throttleItr->second.Global;
    auctionReplicateResult.ChangeNumberCursor = throttleItr->second.Cursor = !auctionReplicateResult.Items.empty() ? auctionReplicateResult.Items.back().AuctionItemID : 0;
    auctionReplicateResult.ChangeNumberTombstone = throttleItr->second.Tombstone = !count ? AuctionsMap.rbegin()->first : 0;
}

void AuctionEntry::BuildAuctionInfo(std::vector<WorldPackets::AuctionHouse::AuctionItem>& items, bool listAuctionItems, Item* sourceItem /*= nullptr*/) const
{
    Item* item = (sourceItem) ? sourceItem : sAuctionMgr->GetAItem(itemGUIDLow);
    if (!item)
    {
        TC_LOG_ERROR("misc", "AuctionEntry::BuildAuctionInfo: Auction %u has a non-existent item: %u", Id, itemGUIDLow);
        return;
    }

    WorldPackets::AuctionHouse::AuctionItem auctionItem;

    auctionItem.AuctionItemID = Id;
    auctionItem.Item.Initialize(item);
    auctionItem.BuyoutPrice = buyout;
    auctionItem.CensorBidInfo = false;
    auctionItem.CensorServerSideInfo = listAuctionItems;
    auctionItem.Charges = item->GetSpellCharges();
    auctionItem.Count = item->GetCount();
    auctionItem.DeleteReason = 0; // Always 0 ?
    auctionItem.DurationLeft = (expire_time - time(NULL)) * IN_MILLISECONDS;
    auctionItem.EndTime = expire_time;
    auctionItem.Flags = 0; // todo
    auctionItem.ItemGuid = item->GetGUID();
    auctionItem.MinBid = startbid;
    auctionItem.Owner = ObjectGuid::Create<HighGuid::Player>(owner);
    auctionItem.OwnerAccountID = ObjectGuid(HighGuid::WowAccount, sObjectMgr->GetPlayerAccountIdByGUID(auctionItem.Owner));
    auctionItem.MinIncrement = bidder ? GetAuctionOutBid() : 0;
    auctionItem.Bidder = bidder ? ObjectGuid::Create<HighGuid::Player>(bidder) : ObjectGuid::Empty;
    auctionItem.BidAmount = bidder ? bid : 0;

    for (uint8 i = 0; i < MAX_INSPECTED_ENCHANTMENT_SLOT; i++)
    {
        if (!item->GetEnchantmentId((EnchantmentSlot)i))
            continue;

        auctionItem.Enchantments.emplace_back(item->GetEnchantmentId((EnchantmentSlot)i), item->GetEnchantmentDuration((EnchantmentSlot)i), item->GetEnchantmentCharges((EnchantmentSlot)i), i);
    }

    items.emplace_back(auctionItem);
}

uint32 AuctionEntry::GetAuctionCut() const
{
    int64 cut = int64(CalculatePct(bid, auctionHouseEntry->cutPercent) * sWorld->getRate(RATE_AUCTION_CUT));
    return std::max(cut, int64(0));
}

/// the sum of outbid is (1% from current bid)*5, if bid is very small, it is 1c
uint32 AuctionEntry::GetAuctionOutBid() const
{
    uint32 outbid = CalculatePct(bid, 5);
    return outbid ? outbid : 1;
}

void AuctionEntry::DeleteFromDB(CharacterDatabaseTransaction& trans) const
{
    CharacterDatabasePreparedStatement* stmt;

    stmt = CharacterDatabase.GetPreparedStatement(CHAR_DEL_AUCTION);
    stmt->setUInt32(0, Id);
    trans->Append(stmt);

    stmt = CharacterDatabase.GetPreparedStatement(CHAR_DEL_AUCTION_BIDDERS);
    stmt->setUInt32(0, Id);
    trans->Append(stmt);
}

void AuctionEntry::SaveToDB(CharacterDatabaseTransaction& trans) const
{
    CharacterDatabasePreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_INS_AUCTION);
    stmt->setUInt32(0, Id);
    stmt->setUInt32(1, houseId);
    stmt->setUInt32(2, itemGUIDLow);
    stmt->setUInt32(3, owner.GetCounter());
    stmt->setUInt32(4, buyout);
    stmt->setUInt32(5, uint32(expire_time));
    stmt->setUInt32(6, bidder.GetCounter());
    stmt->setUInt32(7, bid);
    stmt->setUInt32(8, startbid);
    stmt->setUInt32(9, deposit);
    trans->Append(stmt);
}

bool AuctionEntry::LoadFromDB(Field* fields)
{
    Id = fields[0].GetUInt32();
    houseId = fields[1].GetUInt8();
    itemGUIDLow = fields[2].GetUInt32();
    itemEntry = fields[3].GetUInt32();
    itemCount = fields[4].GetUInt32();
    owner = ObjectGuid::Create<HighGuid::Player>(fields[5].GetUInt32());
    buyout = fields[6].GetUInt32();
    expire_time = fields[7].GetUInt32();
    bidder = ObjectGuid::Create<HighGuid::Player>(fields[8].GetUInt32());
    bid = fields[9].GetUInt32();
    startbid = fields[10].GetUInt32();
    deposit = fields[11].GetUInt32();

    auctionHouseEntry = AuctionHouseMgr::GetAuctionHouseEntryFromHouse(houseId);
    if (!auctionHouseEntry)
    {
        TC_LOG_ERROR("misc", "Auction %u has invalid house id %u", Id, houseId);
        return false;
    }

    // check if sold item exists for guid
    // and itemEntry in fact (GetAItem will fail if problematic in result check in AuctionHouseMgr::LoadAuctionItems)
    if (!sAuctionMgr->GetAItem(itemGUIDLow))
    {
        TC_LOG_ERROR("misc", "Auction %u has not a existing item : %u", Id, itemGUIDLow);
        return false;
    }

    return true;
}

std::string AuctionEntry::BuildAuctionMailSubject(MailAuctionAnswers response) const
{
    std::ostringstream strm;
    strm << itemEntry << ":0:" << response << ':' << Id << ':' << itemCount;
    return strm.str();
}

std::string AuctionEntry::BuildAuctionMailBody(ObjectGuid guid, uint64 bid, uint64 buyout, uint64 deposit, uint64 cut)
{
    std::ostringstream strm;
    strm.width(16);
    strm << std::right << std::hex << guid.GetRawValue();   // HighGuid::Player always present, even for empty guids
    strm << std::dec << ':' << bid << ':' << buyout;
    strm << ':' << deposit << ':' << cut;
    return strm.str();
}

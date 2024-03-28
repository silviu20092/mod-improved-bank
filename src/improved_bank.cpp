/*
 * Credits: silviu20092
 */

#include "ObjectMgr.h"
#include "Chat.h"
#include "ScriptMgr.h"
#include "Config.h"
#include "Player.h"
#include "ScriptedGossip.h"
#include "DatabaseEnv.h"
#include "Tokenize.h"
#include "GameTime.h"
#include "improved_bank.h"

ImprovedBank::ImprovedBank()
{
    accountWide = true;
    searchBank = false;
    showDepositReagents = true;
    depositReagentsSearchBank = false;
}

ImprovedBank::~ImprovedBank()
{
}

ImprovedBank* ImprovedBank::instance()
{
    static ImprovedBank instance;
    return &instance;
}

std::string ImprovedBank::ItemIcon(uint32 entry, uint32 width, uint32 height, int x, int y) const
{
    std::ostringstream ss;
    ss << "|TInterface";
    const ItemTemplate* temp = sObjectMgr->GetItemTemplate(entry);
    const ItemDisplayInfoEntry* dispInfo = NULL;
    if (temp)
    {
        dispInfo = sItemDisplayInfoStore.LookupEntry(temp->DisplayInfoID);
        if (dispInfo)
            ss << "/ICONS/" << dispInfo->inventoryIcon;
    }
    if (!dispInfo)
        ss << "/InventoryItems/WoWUnknownItem01";
    ss << ":" << width << ":" << height << ":" << x << ":" << y << "|t";
    return ss.str();
}

std::string ImprovedBank::ItemIcon(uint32 entry) const
{
    return ItemIcon(entry, 30, 30, 0, 0);
}

std::string ImprovedBank::ItemNameWithLocale(const Player* player, const ItemTemplate* itemTemplate, int32 randomPropertyId) const
{
    LocaleConstant loc_idx = player->GetSession()->GetSessionDbLocaleIndex();
    std::string name = itemTemplate->Name1;
    if (ItemLocale const* il = sObjectMgr->GetItemLocale(itemTemplate->ItemId))
        ObjectMgr::GetLocaleString(il->Name, loc_idx, name);

    std::array<char const*, 16> const* suffix = nullptr;
    if (randomPropertyId < 0)
    {
        if (const ItemRandomSuffixEntry* itemRandEntry = sItemRandomSuffixStore.LookupEntry(-randomPropertyId))
            suffix = &itemRandEntry->Name;
    }
    else
    {
        if (const ItemRandomPropertiesEntry* itemRandEntry = sItemRandomPropertiesStore.LookupEntry(randomPropertyId))
            suffix = &itemRandEntry->Name;
    }
    if (suffix)
    {
        std::string_view test((*suffix)[(name != itemTemplate->Name1) ? loc_idx : DEFAULT_LOCALE]);
        if (!test.empty())
        {
            name += ' ';
            name += test;
        }
    }

    return name;
}

std::string ImprovedBank::ItemLink(const Player* player, const ItemTemplate* itemTemplate, int32 randomPropertyId) const
{
    std::stringstream oss;
    oss << "|c";
    oss << std::hex << ItemQualityColors[itemTemplate->Quality] << std::dec;
    oss << "|Hitem:";
    oss << itemTemplate->ItemId;
    oss << ":0:0:0:0:0:0:0:0:0|h[";
    oss << ItemNameWithLocale(player, itemTemplate, randomPropertyId);
    oss << "]|h|r";

    return oss.str();
}

void ImprovedBank::AddDepositItem(const Player* player, const Item* item, PagedData& pagedData, const std::string& from) const
{
    ItemTemplate const* itemTemplate = sObjectMgr->GetItemTemplate(item->GetEntry());
    if (!itemTemplate)
        return;

    if (item->IsNotEmptyBag())
        return;

    ItemIdentifier itemIdentifier;
    itemIdentifier.id = pagedData.data.size();
    itemIdentifier.guid = item->GetGUID();
    itemIdentifier.name = ItemNameWithLocale(player, itemTemplate, item->GetItemRandomPropertyId());

    std::ostringstream oss;
    oss << ItemIcon(item->GetEntry());
    oss << ItemLink(player, itemTemplate, item->GetItemRandomPropertyId());
    if (item->GetCount() > 1)
        oss << " - " << item->GetCount() << "x";
    oss << " - IN " << from;

    itemIdentifier.duration = item->GetUInt32Value(ITEM_FIELD_DURATION);
    itemIdentifier.tradeable = item->HasFlag(ITEM_FIELD_FLAGS, ITEM_FIELD_FLAG_BOP_TRADEABLE);

    if (itemIdentifier.duration > 0)
        oss << " - |cffb50505DURATION|r";
    if (itemIdentifier.tradeable)
        oss << " - |cffb50505TRADEABLE|r";

    itemIdentifier.uiName = oss.str();

    pagedData.data.push_back(itemIdentifier);
}

void ImprovedBank::BuildItemCatalogueFromInventory(const Player* player, PagedData& pagedData)
{
    pagedData.Reset();

    for (uint8 i = INVENTORY_SLOT_ITEM_START; i < INVENTORY_SLOT_ITEM_END; i++)
        if (Item* item = player->GetItemByPos(INVENTORY_SLOT_BAG_0, i))
            AddDepositItem(player, item, pagedData, "BACKPACK");

    for (uint8 i = INVENTORY_SLOT_BAG_START; i < INVENTORY_SLOT_BAG_END; i++)
        if (Bag* bag = player->GetBagByPos(i))
            for (uint32 j = 0; j < bag->GetBagSize(); j++)
                if (Item* item = player->GetItemByPos(i, j))
                    AddDepositItem(player, item, pagedData, "BAGS");

    for (uint8 i = KEYRING_SLOT_START; i < KEYRING_SLOT_END; ++i)
        if (Item* item = player->GetItemByPos(INVENTORY_SLOT_BAG_0, i))
            AddDepositItem(player, item, pagedData, "KEYRING");

    for (uint8 i = EQUIPMENT_SLOT_START; i < EQUIPMENT_SLOT_END; i++)
        if (Item* item = player->GetItemByPos(INVENTORY_SLOT_BAG_0, i))
            AddDepositItem(player, item, pagedData, "EQUIPPED");

    if (GetSearchBank())
    {
        for (uint8 i = BANK_SLOT_ITEM_START; i < BANK_SLOT_ITEM_END; i++)
            if (Item* item = player->GetItemByPos(INVENTORY_SLOT_BAG_0, i))
                AddDepositItem(player, item, pagedData, "BANK");

        for (uint8 i = BANK_SLOT_BAG_START; i < BANK_SLOT_BAG_END; i++)
            if (Bag* bag = player->GetBagByPos(i))
                for (uint32 j = 0; j < bag->GetBagSize(); j++)
                    if (Item* item = player->GetItemByPos(i, j))
                        AddDepositItem(player, item, pagedData, "BANK BAGS");
    }

    pagedData.SortAndCalculateTotals();
}

void ImprovedBank::PagedData::CalculateTotals()
{
    totalPages = data.size() / PAGE_SIZE;
    if (data.size() % PAGE_SIZE != 0)
        totalPages++;
}

void ImprovedBank::PagedData::SortAndCalculateTotals()
{
    if (data.size() > 0)
    {
        std::sort(data.begin(), data.end());
        CalculateTotals();
    }
}

bool ImprovedBank::AddPagedData(Player* player, const PagedData& pagedData, uint32 page, uint32 sender, uint32 pageSender, uint32 refreshSender)
{
    const ItemIdentifierContainer& itemCatalogue = pagedData.data;
    if (itemCatalogue.size() == 0 || (page + 1) > pagedData.totalPages)
        return false;

    uint32 lowIndex = page * PagedData::PAGE_SIZE;
    if (itemCatalogue.size() <= lowIndex)
        return false;

    uint32 highIndex = lowIndex + PagedData::PAGE_SIZE - 1;
    if (highIndex >= itemCatalogue.size())
        highIndex = itemCatalogue.size() - 1;

    for (uint32 i = lowIndex; i <= highIndex; i++)
    {
        const ItemIdentifier& itemInfo = itemCatalogue[i];
        if (sender == GOSSIP_SENDER_MAIN + 1)
        {
            if (itemInfo.tradeable)
                AddGossipItemFor(player, GOSSIP_ICON_MONEY_BAG, itemInfo.uiName, sender, GOSSIP_ACTION_INFO_DEF + itemInfo.id, "Item is eligible for BOP trade. Depositing it will invalidate this!", 0, false);
            else if (itemInfo.duration > 0)
                AddGossipItemFor(player, GOSSIP_ICON_MONEY_BAG, itemInfo.uiName, sender, GOSSIP_ACTION_INFO_DEF + itemInfo.id, "Item has an expiration time. Timer will still advance while the item is deposited!", 0, false);
            else
                AddGossipItemFor(player, GOSSIP_ICON_MONEY_BAG, itemInfo.uiName, sender, GOSSIP_ACTION_INFO_DEF + itemInfo.id);
        }
        else
            AddGossipItemFor(player, GOSSIP_ICON_MONEY_BAG, itemInfo.uiName, sender, GOSSIP_ACTION_INFO_DEF + itemInfo.id);
    }

    AddGossipItemFor(player, GOSSIP_ICON_INTERACT_1, "[Refresh]", refreshSender, GOSSIP_ACTION_INFO_DEF + page);

    if (page + 1 < pagedData.totalPages)
        AddGossipItemFor(player, GOSSIP_ICON_CHAT, "[Next] ->", pageSender, GOSSIP_ACTION_INFO_DEF + page + 1);
    AddGossipItemFor(player, GOSSIP_ICON_CHAT, "<- [Back]", page == 0 ? GOSSIP_SENDER_MAIN : pageSender, page == 0 ? GOSSIP_ACTION_INFO_DEF : GOSSIP_ACTION_INFO_DEF + page - 1);

    return true;
}

void ImprovedBank::NoPagedData(Player* player)
{
    AddGossipItemFor(player, GOSSIP_ICON_CHAT, "|cffb50505NOTHING ON THIS PAGE, GO BACK|r", GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF);
    AddGossipItemFor(player, GOSSIP_ICON_CHAT, "<- [Back]", GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF);
}

const ImprovedBank::ItemIdentifier* ImprovedBank::FindItemIdentifierById(uint32 id, const PagedData& pagedData) const
{
    ItemIdentifierContainer::const_iterator citer = std::find_if(pagedData.data.begin(), pagedData.data.end(), [&id](const ItemIdentifier& itemIdentifier) {
        return itemIdentifier.id == id;
    });
    if (citer != pagedData.data.end())
        return &*citer;
    return nullptr;
}

ImprovedBank::ItemIdentifier* ImprovedBank::FindItemIdentifierById(uint32 id, PagedData& pagedData)
{
    ItemIdentifierContainer::iterator iter = std::find_if(pagedData.data.begin(), pagedData.data.end(), [&id](const ItemIdentifier& itemIdentifier) {
        return itemIdentifier.id == id;
        });
    if (iter != pagedData.data.end())
        return &*iter;
    return nullptr;
}

std::string ImprovedBank::GetItemCharges(const Item* item) const
{
    std::ostringstream oss;
    for (uint8 i = 0; i < MAX_ITEM_PROTO_SPELLS; ++i)
        oss << item->GetSpellCharges(i) << ' ';
    return oss.str();
}

std::string ImprovedBank::GetItemEnchantments(const Item* item) const
{
    std::ostringstream oss;
    for (uint8 i = 0; i < MAX_ENCHANTMENT_SLOT; ++i)
    {
        oss << item->GetEnchantmentId(EnchantmentSlot(i)) << ' ';
        oss << item->GetEnchantmentDuration(EnchantmentSlot(i)) << ' ';
        oss << item->GetEnchantmentCharges(EnchantmentSlot(i)) << ' ';
    }
    return oss.str();
}

void ImprovedBank::AddDepositItemToDatabase(const Player* player, const Item* item) const
{
    CharacterDatabase.Execute("INSERT INTO mod_improved_bank(owner_guid, owner_account, item_entry, item_count, duration, charges, flags, enchantments, randomPropertyId, durability, deposit_time) VALUES ({}, {}, {}, {}, {}, \"{}\", {}, \"{}\", {}, {}, {})",
        player->GetGUID().GetCounter(), player->GetSession()->GetAccountId(), item->GetEntry(), item->GetCount(),
        item->GetUInt32Value(ITEM_FIELD_DURATION), GetItemCharges(item), item->GetUInt32Value(ITEM_FIELD_FLAGS), GetItemEnchantments(item),
        item->GetItemRandomPropertyId(), item->GetUInt32Value(ITEM_FIELD_DURABILITY), (uint32)GameTime::GetGameTime().count());

}

bool ImprovedBank::DepositItem(ObjectGuid itemGuid, Player* player, uint32* count)
{
    Item* item = player->GetItemByGuid(itemGuid);
    if (item == nullptr)
        return false;

    if (item->IsNotEmptyBag())
        return false;

    if (item->HasFlag(ITEM_FIELD_FLAGS, ITEM_FIELD_FLAG_BOP_TRADEABLE))
        item->RemoveFlag(ITEM_FIELD_FLAGS, ITEM_FIELD_FLAG_BOP_TRADEABLE);

    AddDepositItemToDatabase(player, item);

    if (count != nullptr)
        *count += item->GetCount();

    player->DestroyItem(item->GetBagSlot(), item->GetSlot(), true);

    return true;
}

bool ImprovedBank::DepositItem(uint32 id, Player* player, const PagedData& pagedData)
{
    const ItemIdentifier* itemIdentifier = FindItemIdentifierById(id, pagedData);
    if (itemIdentifier == nullptr)
        return false;

    return DepositItem(itemIdentifier->guid, player);
}

void ImprovedBank::BuildWithdrawItemCatalogue(const Player* player, PagedData& pagedData)
{
    pagedData.Reset();

    std::string baseQuery = "select mib.id, mib.owner_guid, mib.item_entry, mib.item_count, ifnull(ch.name, \"CHAR DELETED\") ch_name, duration, charges, flags, enchantments, randomPropertyId, durability, deposit_time from mod_improved_bank mib "
        "left outer join characters ch on mib.owner_guid = ch.guid where owner_account = {}";
    QueryResult result;

    if (GetAccountWide())
        result = CharacterDatabase.Query(baseQuery, player->GetSession()->GetAccountId());
    else
        result = CharacterDatabase.Query(baseQuery + " and owner_guid = {}", player->GetSession()->GetAccountId(), player->GetGUID().GetCounter());

    if (!result)
        return;

    do
    {
        Field* fields = result->Fetch();

        ItemIdentifier itemIdentifier;
        itemIdentifier.id = fields[0].Get<uint32>();
        itemIdentifier.entry = fields[2].Get<uint32>();
        const ItemTemplate* itemTemplate = sObjectMgr->GetItemTemplate(itemIdentifier.entry);
        if (!itemTemplate)
            continue;
        int32 randomPropertyId = fields[9].Get<int32>();
        itemIdentifier.randomPropertyId = randomPropertyId;
        itemIdentifier.name = ItemNameWithLocale(player, itemTemplate, randomPropertyId);
        itemIdentifier.count = fields[3].Get<uint32>();

        std::ostringstream oss;
        oss << ItemIcon(itemIdentifier.entry);
        oss << ItemLink(player, itemTemplate, randomPropertyId);
        if (itemIdentifier.count > 1)
            oss << " - " << itemIdentifier.count << "x";
        if (GetAccountWide() && player->GetGUID().GetCounter() != fields[1].Get<uint32>())
            oss << " - FROM " << fields[4].Get<std::string>();

        itemIdentifier.depositTime = fields[11].Get<uint32>();

        int32 duration = (int32)fields[5].Get<uint32>();
        if (duration == 0)
            itemIdentifier.duration = 0;
        else
        {
            uint32 diff = GameTime::GetGameTime().count() - itemIdentifier.depositTime;
            itemIdentifier.duration = duration - diff;
            if (itemIdentifier.duration <= 0)
            {
                itemIdentifier.duration = -1;
                oss << " - |cffb50505EXPIRED|r";
            }
        }

        itemIdentifier.uiName = oss.str();

        itemIdentifier.charges = fields[6].Get<std::string>();
        itemIdentifier.flags = fields[7].Get<uint32>();
        itemIdentifier.enchants = fields[8].Get<std::string>();
        itemIdentifier.durability = fields[10].Get<uint32>();

        pagedData.data.push_back(itemIdentifier);
    } while (result->NextRow());

    pagedData.SortAndCalculateTotals();
}

bool ImprovedBank::LoadDataIntoItemFields(Item* item, std::string const& data, uint32 startOffset, uint32 count)
{
    if (data.empty())
        return false;

    std::vector<std::string_view> tokens = Acore::Tokenize(data, ' ', false);

    if (tokens.size() != count)
        return false;

    for (uint32 index = 0; index < count; ++index)
    {
        Optional<uint32> val = Acore::StringTo<uint32>(tokens[index]);
        if (!val)
        {
            return false;
        }

        item->UpdateUInt32Value(startOffset + index, *val);
    }

    return true;
}

Item* ImprovedBank::CreateItem(Player* player, ItemPosCountVec const& dest, uint32 itemEntry, bool update, int32 randomPropertyId,
    uint32 duration, const std::string& charges, uint32 flags, const std::string& enchants, uint32 durability)
{
    uint32 count = 0;
    for (ItemPosCountVec::const_iterator itr = dest.begin(); itr != dest.end(); ++itr)
        count += itr->count;

    Item* newItem = Item::CreateItem(itemEntry, count, player, false, randomPropertyId);
    if (newItem != nullptr)
    {
        ItemTemplate const* itemTemplate = newItem->GetTemplate();

        newItem->SetUInt32Value(ITEM_FIELD_DURATION, duration);

        std::vector<std::string_view> tokens = Acore::Tokenize(charges, ' ', false);
        if (tokens.size() == MAX_ITEM_PROTO_SPELLS)
        {
            for (uint8 i = 0; i < MAX_ITEM_PROTO_SPELLS; ++i)
            {
                if (Optional<int32> charges = Acore::StringTo<int32>(tokens[i]))
                    newItem->SetSpellCharges(i, *charges);
            }
        }

        newItem->SetUInt32Value(ITEM_FIELD_FLAGS, flags);
        if (newItem->IsSoulBound() && itemTemplate->Bonding == NO_BIND && sScriptMgr->CanApplySoulboundFlag(newItem, itemTemplate))
            newItem->ApplyModFlag(ITEM_FIELD_FLAGS, ITEM_FIELD_FLAG_SOULBOUND, false);

        LoadDataIntoItemFields(newItem, enchants, ITEM_FIELD_ENCHANTMENT_1_1, MAX_ENCHANTMENT_SLOT * MAX_ENCHANTMENT_OFFSET);

        newItem->SetUInt32Value(ITEM_FIELD_DURABILITY, durability);
        if (durability > itemTemplate->MaxDurability && !newItem->HasFlag(ITEM_FIELD_FLAGS, ITEM_FIELD_FLAG_WRAPPED))
            newItem->SetUInt32Value(ITEM_FIELD_DURABILITY, itemTemplate->MaxDurability);

        if (itemTemplate->Quality >= ITEM_QUALITY_RARE)
            player->AdditionalSavingAddMask(ADDITIONAL_SAVING_INVENTORY_AND_GOLD);

        player->ItemAddedQuestCheck(itemEntry, count);
        player->UpdateAchievementCriteria(ACHIEVEMENT_CRITERIA_TYPE_RECEIVE_EPIC_ITEM, itemEntry, count);
        player->UpdateAchievementCriteria(ACHIEVEMENT_CRITERIA_TYPE_OWN_ITEM, itemEntry, count);
        newItem = player->StoreItem(dest, newItem, update);

        sScriptMgr->OnStoreNewItem(player, newItem, count);
    }
    return newItem;
}

bool ImprovedBank::WithdrawItem(uint32 id, Player* player, PagedData& pagedData)
{
    ItemIdentifier* itemIdentifier = FindItemIdentifierById(id, pagedData);
    if (itemIdentifier == nullptr)
        return false;

    // item expired, just remove it
    if (itemIdentifier->duration == -1)
    {
        RemoveItemFromDatabase(itemIdentifier->id);
        return false;
    }

    // re-check with deposit time if item expired, maybe player did not refresh in the meantime
    if (itemIdentifier->duration > 0)
    {
        uint32 diff = GameTime::GetGameTime().count() - itemIdentifier->depositTime;
        itemIdentifier->duration = itemIdentifier->duration - diff;
        if (itemIdentifier->duration <= 0)
        {
            RemoveItemFromDatabase(itemIdentifier->id);
            return false;
        }
    }

    ItemPosCountVec dest;
    InventoryResult msg = player->CanStoreNewItem(NULL_BAG, NULL_SLOT, dest, itemIdentifier->entry, itemIdentifier->count);
    if (msg == EQUIP_ERR_OK)
    {
        Item* item = CreateItem(player, dest, itemIdentifier->entry, true, itemIdentifier->randomPropertyId, (uint32)itemIdentifier->duration, itemIdentifier->charges, itemIdentifier->flags, itemIdentifier->enchants, itemIdentifier->durability);
        player->SendNewItem(item, itemIdentifier->count, true, false);

        RemoveItemFromDatabase(itemIdentifier->id);
        RemoveFromPagedData(itemIdentifier->id, pagedData);
        
        return true;
    }

    return false;
}

void ImprovedBank::RemoveFromPagedData(uint32 id, PagedData& pagedData)
{
    ImprovedBank::ItemIdentifierContainer::const_iterator citr = std::remove_if(pagedData.data.begin(), pagedData.data.end(), [&](const ItemIdentifier& identifier) { return identifier.id == id; });
    pagedData.data.erase(citr, pagedData.data.end());

    pagedData.CalculateTotals();
}

void ImprovedBank::RemoveItemFromDatabase(uint32 id)
{
    CharacterDatabase.Execute("DELETE FROM mod_improved_bank WHERE id = {}", id);
}

bool ImprovedBank::IsReagent(const Item* item) const
{
    const ItemTemplate* itemTemplate = item->GetTemplate();
    return (itemTemplate->Class == ITEM_CLASS_TRADE_GOODS || itemTemplate->Class == ITEM_CLASS_GEM) && itemTemplate->GetMaxStackSize() > 1;
}

void ImprovedBank::DepositAllReagents(Player* player, uint32* totalCount)
{
    *totalCount = 0;

    for (uint8 i = INVENTORY_SLOT_ITEM_START; i < INVENTORY_SLOT_ITEM_END; i++)
        if (Item* item = player->GetItemByPos(INVENTORY_SLOT_BAG_0, i))
            if (IsReagent(item))
                DepositItem(item->GetGUID(), player, totalCount);

    for (uint8 i = INVENTORY_SLOT_BAG_START; i < INVENTORY_SLOT_BAG_END; i++)
        if (Bag* bag = player->GetBagByPos(i))
            for (uint32 j = 0; j < bag->GetBagSize(); j++)
                if (Item* item = player->GetItemByPos(i, j))
                    if (IsReagent(item))
                        DepositItem(item->GetGUID(), player, totalCount);

    if (GetDepositReagentsSearchBank())
    {
        for (uint8 i = BANK_SLOT_ITEM_START; i < BANK_SLOT_ITEM_END; i++)
            if (Item* item = player->GetItemByPos(INVENTORY_SLOT_BAG_0, i))
                if (IsReagent(item))
                    DepositItem(item->GetGUID(), player, totalCount);

        for (uint8 i = BANK_SLOT_BAG_START; i < BANK_SLOT_BAG_END; i++)
            if (Bag* bag = player->GetBagByPos(i))
                for (uint32 j = 0; j < bag->GetBagSize(); j++)
                    if (Item* item = player->GetItemByPos(i, j))
                        if (IsReagent(item))
                            DepositItem(item->GetGUID(), player, totalCount);
    }
}

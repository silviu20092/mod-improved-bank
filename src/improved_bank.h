/*
 * Credits: silviu20092
 */

#ifndef _IMPROVED_BANK_H_
#define _IMPROVED_BANK_H_

#include <string>
#include <vector>
#include "SharedDefines.h"
#include "Player.h"

class ImprovedBank
{
public:
    struct ItemIdentifier
    {
        uint32 id;
        uint32 entry;
        uint32 count;
        ObjectGuid guid;
        std::string name;
        std::string uiName;

        // item specific properties, used for withdraw
        int32 duration;
        std::string charges;
        uint32 flags;
        std::string enchants;
        int32 randomPropertyId;
        uint32 durability;
        uint32 depositTime;

        // deposit info, for warnings
        bool tradeable;

        bool operator<(const ItemIdentifier& a)
        {
            return name < a.name;
        }
    };
    typedef std::vector<ItemIdentifier> ItemIdentifierContainer;

    struct PagedData
    {
        static constexpr int PAGE_SIZE = 12;
        uint32 totalPages = 0;
        uint32 currentPage = 0;
        ItemIdentifierContainer data;

        void Reset()
        {
            totalPages = 0;
            data.clear();
        }

        void CalculateTotals();
        void SortAndCalculateTotals();
    };
private:
    ImprovedBank();
    ~ImprovedBank();

    bool accountWide;
    bool searchBank;
    bool showDepositReagents;
    bool depositReagentsSearchBank;

    std::string ItemIcon(uint32 entry, uint32 width, uint32 height, int x, int y) const;
    std::string ItemNameWithLocale(const Player* player, const ItemTemplate* itemTemplate, int32 randomPropertyId) const;
    std::string ItemLink(const Player* player, const ItemTemplate* itemTemplate, int32 randomPropertyId) const;

    void AddDepositItem(const Player* player, const Item* item, PagedData& pagedData, const std::string& from) const;
    void AddDepositItemToDatabase(const Player* player, const Item* item) const;
    const ItemIdentifier* FindItemIdentifierById(uint32 id, const PagedData& pagedData) const;
    ItemIdentifier* FindItemIdentifierById(uint32 id, PagedData& pagedData);
    void RemoveFromPagedData(uint32 id, PagedData& pagedData);
    void RemoveItemFromDatabase(uint32 id);

    bool DepositItem(ObjectGuid itemGuid, Player* player, uint32* count = nullptr);
    bool IsReagent(const Item* item) const;

    std::string GetItemCharges(const Item* item) const;
    std::string GetItemEnchantments(const Item* item) const;

    Item* CreateItem(Player* player, ItemPosCountVec const& dest, uint32 itemEntry, bool update, int32 randomPropertyId,
        uint32 duration, const std::string& charges, uint32 flags, const std::string& enchants, uint32 durability);
    bool LoadDataIntoItemFields(Item* item, std::string const& data, uint32 startOffset, uint32 count);
public:
    static ImprovedBank* instance();

    void SetAccountWide(bool value) { accountWide = value; }
    void SetSearchBank(bool value) { searchBank = value; }
    void SetShowDepositReagents(bool value) { showDepositReagents = value; }
    void SetDepositReagentsSearchBank(bool value) { depositReagentsSearchBank = value; }
    bool GetAccountWide() { return accountWide; }
    bool GetSearchBank() { return searchBank; }
    bool GetShowDepositReagents() { return showDepositReagents; }
    bool GetDepositReagentsSearchBank() { return depositReagentsSearchBank; }

    std::string ItemIcon(uint32 entry) const;

    void BuildItemCatalogueFromInventory(const Player* player, PagedData& pagedData);
    bool AddPagedData(Player* player, const PagedData& pagedData, uint32 page, uint32 sender, uint32 pageSender, uint32 refreshSender);
    void NoPagedData(Player* player);

    bool DepositItem(uint32 id, Player* player, const PagedData& pagedData);

    void BuildWithdrawItemCatalogue(const Player* player, PagedData& pagedData);
    bool WithdrawItem(uint32 id, Player* player, PagedData& pagedData);

    void DepositAllReagents(Player* player, uint32* totalCount);
};

#define sImprovedBank ImprovedBank::instance()

#endif

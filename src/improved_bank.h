/*
 * Credits: silviu20092
 */

#ifndef _IMPROVED_BANK_H_
#define _IMPROVED_BANK_H_

#include <string>
#include <vector>
#include "SharedDefines.h"

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
    std::string ItemNameWithLocale(const Player* player, const ItemTemplate* itemTemplate) const;
    std::string ItemLink(const Player* player, const ItemTemplate* itemTemplate) const;
    std::string ItemLink(const Player* player, uint32 entry) const;

    void AddDepositItem(const Player* player, const Item* item, PagedData& pagedData, const std::string& from) const;
    void AddDepositItemToDatabase(const Player* player, const Item* item) const;
    const ItemIdentifier* FindItemIdentifierById(uint32 id, const PagedData& pagedData) const;
    void RemoveFromPagedData(uint32 id, PagedData& pagedData);
    void RemoveItemFromDatabase(uint32 id);

    bool DepositItem(ObjectGuid itemGuid, Player* player, uint32* count = nullptr);
    bool IsReagent(const Item* item) const;
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
    bool AddPagedData(Player* player, Creature* creature, const PagedData& pagedData, uint32 page, uint32 sender, uint32 pageSender, uint32 refreshSender);
    void NoPagedData(Player* player, Creature* creature);

    bool DepositItem(uint32 id, Player* player, const PagedData& pagedData);

    void BuildWithdrawItemCatalogue(const Player* player, PagedData& pagedData);
    bool WithdrawItem(uint32 id, Player* player, PagedData& pagedData);

    void DepositAllReagents(Player* player, uint32* totalCount);
};

#define sImprovedBank ImprovedBank::instance()

#endif

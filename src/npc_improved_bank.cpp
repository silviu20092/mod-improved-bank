/*
 * Credits: silviu20092
 */

#include "ScriptMgr.h"
#include "ScriptedGossip.h"
#include "Player.h"
#include "Chat.h"
#include "improved_bank.h"

class npc_improved_bank : public CreatureScript
{
private:
    std::unordered_map<uint32, ImprovedBank::PagedData> depositPagedDataMap;
    std::unordered_map<uint32, ImprovedBank::PagedData> withdrawPagedDataMap;
private:
    bool AddPagedDataNpc(Player* player, Creature* creature, ImprovedBank::PagedData& pagedData, uint32 page, uint32 sender, uint32 pageSender, uint32 refreshSender)
    {
        ClearGossipMenuFor(player);
        while (!sImprovedBank->AddPagedData(player, pagedData, page, sender, pageSender, refreshSender))
        {
            if (page == 0)
            {
                sImprovedBank->NoPagedData(player);
                break;
            }
            else
                page--;
        }
        pagedData.currentPage = page;
        SendGossipMenuFor(player, DEFAULT_GOSSIP_MESSAGE, creature->GetGUID());
        return true;
    }
public:
    npc_improved_bank() : CreatureScript("npc_improved_bank")
    {
    }

    bool OnGossipHello(Player* player, Creature* creature) override
    {
        AddGossipItemFor(player, GOSSIP_ICON_VENDOR, "Deposit...", GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 1);
        if (sImprovedBank->GetShowDepositReagents())
            AddGossipItemFor(player, GOSSIP_ICON_VENDOR, "Deposit all reagents...", GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 4);
        AddGossipItemFor(player, GOSSIP_ICON_MONEY_BAG, "Withdraw...", GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 2);
        AddGossipItemFor(player, GOSSIP_ICON_CHAT, "Nevermind", GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 3);
        SendGossipMenuFor(player, DEFAULT_GOSSIP_MESSAGE, creature->GetGUID());
        return true;
    }

    bool OnGossipSelect(Player* player, Creature* creature, uint32 sender, uint32 action) override
    {
        ImprovedBank::PagedData& depositPageData = depositPagedDataMap[player->GetGUID().GetCounter()];
        ImprovedBank::PagedData& withdrawPageData = withdrawPagedDataMap[player->GetSession()->GetAccountId()];
        if (sender == GOSSIP_SENDER_MAIN)
        {
            if (action == GOSSIP_ACTION_INFO_DEF)
            {
                ClearGossipMenuFor(player);
                return OnGossipHello(player, creature);
            }
            else if (action == GOSSIP_ACTION_INFO_DEF + 1)
            {
                sImprovedBank->BuildItemCatalogueFromInventory(player, depositPageData);
                return AddPagedDataNpc(player, creature, depositPageData, 0, GOSSIP_SENDER_MAIN + 1, GOSSIP_SENDER_MAIN + 2, GOSSIP_SENDER_MAIN + 2);
            }
            else if (action == GOSSIP_ACTION_INFO_DEF + 2)
            {
                sImprovedBank->BuildWithdrawItemCatalogue(player, withdrawPageData);
                return AddPagedDataNpc(player, creature, withdrawPageData, 0, GOSSIP_SENDER_MAIN + 3, GOSSIP_SENDER_MAIN + 4, GOSSIP_SENDER_MAIN + 5);
            }
            else if (action == GOSSIP_ACTION_INFO_DEF + 3)
            {
                CloseGossipMenuFor(player);
                return true;
            }
            else if (action == GOSSIP_ACTION_INFO_DEF + 4)
            {
                uint32 count = 0;
                sImprovedBank->DepositAllReagents(player, &count);
                if (count == 0)
                    ChatHandler(player->GetSession()).SendSysMessage("No reagents found to deposit.");
                else
                    ChatHandler(player->GetSession()).PSendSysMessage("Deposited a total of %d reagents.", count);
                CloseGossipMenuFor(player);
                return true;
            }
        }
        else if (sender == GOSSIP_SENDER_MAIN + 1)
        {
            uint32 id = action - GOSSIP_ACTION_INFO_DEF;
            if (!sImprovedBank->DepositItem(id, player, depositPageData))
            {
                ChatHandler(player->GetSession()).SendSysMessage("Could not deposit item. Item might no longer be inventory.");
                CloseGossipMenuFor(player);
                return false;
            }

            sImprovedBank->BuildItemCatalogueFromInventory(player, depositPageData);
            return AddPagedDataNpc(player, creature, depositPageData, depositPageData.currentPage, GOSSIP_SENDER_MAIN + 1, GOSSIP_SENDER_MAIN + 2, GOSSIP_SENDER_MAIN + 2);
        }
        else if (sender == GOSSIP_SENDER_MAIN + 2)
        {
            depositPageData.currentPage = action - GOSSIP_ACTION_INFO_DEF;
            sImprovedBank->BuildItemCatalogueFromInventory(player, depositPageData);
            return AddPagedDataNpc(player, creature, depositPageData, depositPageData.currentPage, GOSSIP_SENDER_MAIN + 1, GOSSIP_SENDER_MAIN + 2, GOSSIP_SENDER_MAIN + 2);
        }
        else if (sender == GOSSIP_SENDER_MAIN + 3)
        {
            uint32 id = action - GOSSIP_ACTION_INFO_DEF;
            if (!sImprovedBank->WithdrawItem(id, player, withdrawPageData))
            {
                ChatHandler(player->GetSession()).SendSysMessage("Could not withdraw item. Possible reasons: item already withdrawn, no space in inventory, unique item already in inventory, item expired.");
                CloseGossipMenuFor(player);
                return false;
            }
            return AddPagedDataNpc(player, creature, withdrawPageData, withdrawPageData.currentPage, GOSSIP_SENDER_MAIN + 3, GOSSIP_SENDER_MAIN + 4, GOSSIP_SENDER_MAIN + 5);
        }
        else if (sender == GOSSIP_SENDER_MAIN + 4)
        {
            withdrawPageData.currentPage = action - GOSSIP_ACTION_INFO_DEF;
            return AddPagedDataNpc(player, creature, withdrawPageData, withdrawPageData.currentPage, GOSSIP_SENDER_MAIN + 3, GOSSIP_SENDER_MAIN + 4, GOSSIP_SENDER_MAIN + 5);
        }
        else if (sender == GOSSIP_SENDER_MAIN + 5)
        {
            sImprovedBank->BuildWithdrawItemCatalogue(player, withdrawPageData);
            return AddPagedDataNpc(player, creature, withdrawPageData, withdrawPageData.currentPage, GOSSIP_SENDER_MAIN + 3, GOSSIP_SENDER_MAIN + 4, GOSSIP_SENDER_MAIN + 5);
        }

        CloseGossipMenuFor(player);
        return false;
    }
};

void AddSC_npc_improved_bank()
{
    new npc_improved_bank();
}

#pragma once

std::atomic<bool> DupeItems{ 0 };

typedef uint32_t*(__fastcall* DropInventoryItem_t)(class IModelObject*, uint32_t, int64_t, uint8_t, float*, uint8_t, char, uint32_t, uint32_t);
inline DropInventoryItem_t DropInventoryItemOriginal;
uint32_t* DropInventoryItem(IModelObject* Player, uint32_t ItemIndex, int64_t InventoryItem, uint8_t DropMode, float* Position, uint8_t PhysicsMode, char RemoveFromInventory, uint32_t Quantity, uint32_t Context)
{
    if (DupeItems.load(std::memory_order_relaxed)) {

        const bool HasThrowVector = Position && (Position[0] != 0.0f || Position[1] != 0.0f || Position[2] != 0.0f);

        if (!HasThrowVector) {
            RemoveFromInventory = false;
        }
    }

    return DropInventoryItemOriginal(Player, ItemIndex, InventoryItem, DropMode, Position, PhysicsMode, RemoveFromInventory, Quantity, Context);
}
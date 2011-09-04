#ifndef PERSISTENTITEM_H
#define PERSISTENTITEM_H

class PersistentItem
{
public:
    PersistentItem();

    PersistentItem(int itemId, int itemAmount, int posX, int posY):
        mItemId(itemId), mItemAmount(itemAmount), mPosX(posX), mPosY(posY)
    {}

    /**
     * Returns the item id
     */
    int getItemId() const
    { return mItemId; }

    /**
     * Returns the item amount
     */
    int getItemAmount() const
    { return mItemAmount; }

    /**
     * Returns the position x of the item
     */
    int getPosX() const
    { return mPosX; }

    /**
     * Returns the position x of the item
     */
    int getPosY() const
    { return mPosY; }
private:
    int mItemId;
    int mItemAmount;
    int mPosX;
    int mPosY;
};

#endif // PERSISTENTITEM_H

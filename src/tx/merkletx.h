// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2017-2019 The WaykiChain Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php

#ifndef COIN_MERKLETX_H
#define COIN_MERKLETX_H

#include "persistence/block.h"
#include "tx/txbase.h"

/** A transaction with a merkle branch linking it to the block chain. */
class CMerkleTx {
private:
    int GetDepthInMainChainINTERNAL(CBlockIndex *&pindexRet) const;

public:
    uint256 blockHash;
    vector<uint256> vMerkleBranch;
    int nIndex;
    std::shared_ptr<CTxBase> pTx;
    int nHeight;
    // memory only
    mutable bool fMerkleVerified;

    CMerkleTx() {
        Init();
    }

    CMerkleTx(std::shared_ptr<CBaseTx> pBaseTx) : pTx(pBaseTx) {
        Init();
    }

    void Init() {
        blockHash       = uint256();
        nIndex          = -1;
        fMerkleVerified = false;
    }

    IMPLEMENT_SERIALIZE(
        READWRITE(blockHash);
        READWRITE(vMerkleBranch);
        READWRITE(nIndex);
        READWRITE(pTx);
        READWRITE(nHeight);)

    int SetMerkleBranch(const CBlock *pblock = NULL);

    // Return depth of transaction in blockchain:
    // -1  : not in blockchain, and not in memory pool (conflicted transaction)
    //  0  : in memory pool, waiting to be included in a block
    // >=1 : this many blocks deep in the main chain
    int GetDepthInMainChain(CBlockIndex *&pindexRet) const;
    int GetDepthInMainChain() const {
        CBlockIndex *pindexRet;
        return GetDepthInMainChain(pindexRet);
    }
    bool IsInMainChain() const {
        CBlockIndex *pindexRet;
        return GetDepthInMainChainINTERNAL(pindexRet) > 0;
    }
    int GetBlocksToMaturity() const;

    int SetMerkleBranch(const CBlock *pblock) {
        AssertLockHeld(cs_main);
        CBlock blockTmp;

        if (pblock) {
            // Update the tx's blockHash
            blockHash = pblock->GetHash();

            // Locate the transaction
            for (nIndex = 0; nIndex < (int)pblock->vptx.size(); nIndex++)
                if ((pblock->vptx[nIndex])->GetHash() == pTx->GetHash())
                    break;
            if (nIndex == (int)pblock->vptx.size()) {
                vMerkleBranch.clear();
                nIndex = -1;
                LogPrint("INFO", "ERROR: SetMerkleBranch() : couldn't find tx in block\n");
                return 0;
            }

            // Fill in merkle branch
            vMerkleBranch = pblock->GetMerkleBranch(nIndex);
        }

        // Is the tx in a block that's in the main chain
        map<uint256, CBlockIndex *>::iterator mi = mapBlockIndex.find(blockHash);
        if (mi == mapBlockIndex.end())
            return 0;
        CBlockIndex *pIndex = (*mi).second;
        if (!pIndex || !chainActive.Contains(pIndex))
            return 0;

        return chainActive.Height() - pIndex->nHeight + 1;
    }
};

#endif  //COIN_MERKLETX_H
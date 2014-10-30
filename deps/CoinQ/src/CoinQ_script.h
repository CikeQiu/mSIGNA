///////////////////////////////////////////////////////////////////////////////
//
// CoinQ_script.h 
//
// Copyright (c) 2013 Eric Lombrozo
//
// All Rights Reserved.

#ifndef _COINQ_SCRIPT_H_
#define _COINQ_SCRIPT_H_

#include "CoinQ_txs.h"

#include <CoinCore/CoinNodeData.h>
#include <CoinCore/typedefs.h>

#include <utility>

namespace CoinQ {
namespace Script {

/*
 * opPushData - constructs an operator (of variable length) indicating nBytes of data follow.
*/
uchar_vector opPushData(uint32_t nBytes);

/*
 * getDataLength
 *      precondition: pos indicates the position in script of a push operator which is followed by data.
 *      postcondition: pos is advanced to the start of the data.
 *      returns: number of bytes in the data.
*/
uint32_t getDataLength(const uchar_vector& script, uint& pos);

// TODO: Get rid of PUBKEY in names below
enum ScriptType {
    SCRIPT_PUBKEY_UNKNOWN_TYPE,
    SCRIPT_PUBKEY_EMPTY_SCRIPT,
    SCRIPT_PUBKEY_PAY_TO_PUBKEY,
    SCRIPT_PUBKEY_PAY_TO_PUBKEY_HASH,
    SCRIPT_PUBKEY_PAY_TO_SCRIPT_HASH
};

enum SigHashType {
    SIGHASH_ALL             = 0x01,
    SIGHASH_NONE            = 0x02,
    SIGHASH_SINGLE          = 0x03,
    SIGHASH_ANYONECANPAY    = 0x80
};

// TODO: make payee_t a class
typedef std::pair<ScriptType, uchar_vector> payee_t;

/*
 * getScriptPubKeyPayee - create a pair containing the type of transaction and the data necessary to fetch
 *      the redeem script.
*/
payee_t getScriptPubKeyPayee(const uchar_vector& scriptPubKey);

/*
 * isValidAddress - check whether address is valid
*/
bool isValidAddress(const std::string& address, const unsigned char addressVersions[]);

/*
 * getTxOutForAddress - create a txout script from a base58check address
*/
uchar_vector getTxOutScriptForAddress(const std::string& address, const unsigned char addressVersions[]);

/*
 * getAddressForTxOutScript - create a base58check address from a txoutscript
*/
std::string getAddressForTxOutScript(const bytes_t& txoutscript, const unsigned char addressVersions[]);


class Script
{
public:
    enum type_t { UNKNOWN, PAY_TO_PUBKEY, PAY_TO_PUBKEY_HASH, PAY_TO_MULTISIG_SCRIPT_HASH };
    Script(type_t type, unsigned int minsigs, const std::vector<bytes_t>& pubkeys, const std::vector<bytes_t>& sigs = std::vector<bytes_t>());

    // If signinghash is empty, 0-length placeholders are expected in txinscript for missing signatures and no signatures are checked.
    // If signinghash is not empty, all signatures are checked and 0-length placeholders are inserted as necessary.
    // If clearinvalidsigs is true, invalid signatures are cleared. Otherwise, an exception is thrown if signatures are invalid.
    explicit Script(const bytes_t& txinscript, const bytes_t& signinghash = bytes_t(), bool clearinvalidsigs = false);

    type_t type() const { return type_; }
    unsigned int minsigs() const { return minsigs_; }
    const std::vector<bytes_t>& pubkeys() const { return pubkeys_; }
    const std::vector<bytes_t>& sigs() const { return sigs_; }

    static const bool USE_PLACEHOLDERS = true;

    enum sigtype_t { EDIT, SIGN, BROADCAST }; 
    /*
     * EDIT         - includes 0-length placeholders for missing signatures
     * SIGN         - format the script for signing
     * BROADCAST    - format the script for broadcast (remove 0-length placeholders)
    */

    bytes_t txinscript(sigtype_t sigtype) const;
    bytes_t txoutscript() const;

    const bytes_t& redeemscript() const { return redeemscript_; }
    const bytes_t& hash() const { return hash_; }

    unsigned int sigsneeded() const; // returns how many signatures are still needed
    std::vector<bytes_t> missingsigs() const; // returns pubkeys for which we are still missing signatures
    std::vector<bytes_t> presentsigs() const; // returns pubkeys for which we have signatures
    bool addSig(const bytes_t& pubkey, const bytes_t& sig); // returns true iff signature was absent and has been added
    void clearSigs(); // resets all signatures to 0-length placeholders
    unsigned int mergesigs(const Script& other); // merges the signatures from another script that is otherwise identical. returns number of signatures added.

private:
    type_t type_;
    unsigned int minsigs_;
    std::vector<bytes_t> pubkeys_;
    std::vector<bytes_t> sigs_; // empty vectors for missing signatures

    bytes_t redeemscript_; // empty if type is not script hash
    bytes_t hash_;
};


typedef std::vector<Script> scripts_t;

class Signer
{
public:
    Signer() : isSigned_(false) { }
    explicit Signer(const Coin::Transaction& tx, bool clearinvalidsigs = false) { setTx(tx, clearinvalidsigs); }

    void setTx(const Coin::Transaction& tx, bool clearinvalidsigs = false);
    const Coin::Transaction& getTx() const { return tx_; }

    // sign returns a vector of the pubkeys for which signatures were added.
    std::vector<bytes_t> sign(const std::vector<secure_bytes_t>& privkeys);

    bool isSigned() const { return isSigned_; }

    const scripts_t& getScripts() const { return scripts_; }

    void clearSigs() { for (auto& script: scripts_) script.clearSigs(); }

private:
    Coin::Transaction tx_;
    bool isSigned_;
    scripts_t scripts_;
};

}
}

#endif // _COINQ_SCRIPT_H_

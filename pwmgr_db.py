import struct, os, cryptography

# here be dragons
from cryptography.hazmat.primitives import padding
from cryptography.hazmat.primitives.ciphers import Cipher, algorithms, modes


if __name__ == "__main__":
  print("WARN: This module should not be run directly.")


key = b""
iv = b""

def pad(bstr, padlen): # PKCS7
    return bstr+bytes([(padlen-len(bstr)%padlen)]*(padlen-len(bstr)%padlen))

def unpad(bstr):
    if bstr[-1] != bstr[-2]:
        return bstr
    return bstr[:-bstr[-1]]

def gen_key(keylen):
    if not ((keylen == 16) or (keylen == 24) or (keylen == 32)):
        print(f"Attempted to generate key with invalid key length {keylen}.")
    global key, iv
    key = os.urandom(keylen)
    iv = os.urandom(16)
    keyfd = open("key.bin", "wb")
    keyfd.write(struct.pack("<L", keylen))
    keyfd.write(key)
    keyfd.write(iv)
    return key, iv

def get_entries(store):
    if os.path.exists("key.bin"):
        keyfd = open("key.bin", "rb")
        keylen = struct.unpack("<L", keyfd.read(4))[0]
        print(keylen)
        if not ((keylen == 16) or (keylen == 24) or (keylen == 32)):
             print("Key length out of ranges (16, 24, 32), refusing to read key")
             return None
        global key, iv
        key = struct.unpack(f"={keylen}s", keyfd.read(keylen))[0]
        iv = struct.unpack("=16s", keyfd.read(16))[0]
        cipher = Cipher(algorithms.AES(key), modes.CBC(iv))
        keyfd.close()
        dbfd = open("passdb.bin", "rb")
        cntBuf = dbfd.read(4)
        entryCount = struct.unpack("<L", cntBuf)[0]
        for i in range(entryCount):
            entry = []
            for j in range(4):
                de = cipher.decryptor()
                lenBuf = dbfd.read(4)
                elen = struct.unpack("<L", lenBuf)[0]
                p = padding.PKCS7(len(key)*8).unpadder()
                match j:
                    case 0:
                        entry.append(dbfd.read(elen).decode(encoding="utf-8"))
                    case 1:
                        entry.append(unpad(de.update(dbfd.read(elen)) + de.finalize()).decode(encoding="utf-8"))
                    case 2:
                        entry.append(unpad(de.update(dbfd.read(elen)) + de.finalize()).decode(encoding="utf-8"))
                    case 3:
                        entry.append(dbfd.read(elen).decode(encoding="utf-8"))
            store.append(entry)
    else:
        return None
    print("get_entries(" + str(store) + ")")
    return 0

def sync_entries(store):
    if key != b"" and iv != b"":
        dbfd = open("passdb.bin", "wb")
        dbfd.write(struct.pack("<L", len(store)))
        cipher = Cipher(algorithms.AES(key), modes.CBC(iv))
        for entry in store:
            for j in range(4):
                epacked = b""
                en = cipher.encryptor()
                match j:
                    case 0:
                        epacked = entry[0].encode(encoding="utf-8")
                    case 1:
                        epacked = en.update(pad(entry[1].encode(encoding="utf-8"), len(key))) + en.finalize()
                    case 2:
                        epacked = en.update(pad(entry[2].encode(encoding="utf-8"), len(key))) + en.finalize()
                    case 3:
                        epacked = entry[3].encode(encoding="utf-8")
                elen = len(epacked)
                dbfd.write(struct.pack("<L", elen))
                dbfd.write(epacked)
        dbfd.flush()
        dbfd.close()
    else:
        print("Cannot sync to file - no key provided.")
    print("sync_entries(" + str(store) + ")")
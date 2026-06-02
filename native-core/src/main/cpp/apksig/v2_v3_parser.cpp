#include "apksig/v2_v3_parser.hpp"
#include "apksig/signature_scheme.hpp"
#include "apksig/signing_block.hpp"
#include <openssl/asn1.h>
#include <openssl/pkcs7.h>
#include <openssl/x509.h>
#include <openssl/obj_mac.h>
#include <openssl/opensslv.h>
#include <openssl/bio.h>
#include <openssl/evp.h>
#include <stdexcept>
#include <cstring>
#include <android/log.h>
#include <vector>
#include <string>

#define LOG_TAG "Loader"
#define LOGI_V23(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE_V23(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

namespace apksig {

namespace {
    // Helper to read uint32 from little-endian bytes
    std::uint32_t readUint32LE(const std::uint8_t* data) {
        return static_cast<std::uint32_t>(data[0]) |
               (static_cast<std::uint32_t>(data[1]) << 8) |
               (static_cast<std::uint32_t>(data[2]) << 16) |
               (static_cast<std::uint32_t>(data[3]) << 24);
    }
    
    // Helper to read uint64 from little-endian bytes
    std::uint64_t readUint64LE(const std::uint8_t* data) {
        return static_cast<std::uint64_t>(data[0]) |
               (static_cast<std::uint64_t>(data[1]) << 8) |
               (static_cast<std::uint64_t>(data[2]) << 16) |
               (static_cast<std::uint64_t>(data[3]) << 24) |
               (static_cast<std::uint64_t>(data[4]) << 32) |
               (static_cast<std::uint64_t>(data[5]) << 40) |
               (static_cast<std::uint64_t>(data[6]) << 48) |
               (static_cast<std::uint64_t>(data[7]) << 56);
    }
}

std::unique_ptr<SignatureBlock> V2V3Parser::parseV2(const std::vector<std::uint8_t>& block_data) {
    return parse(block_data, APK_SIGNATURE_SCHEME_V2_ID);
}

std::unique_ptr<SignatureBlock> V2V3Parser::parseV3(const std::vector<std::uint8_t>& block_data) {
    return parse(block_data, APK_SIGNATURE_SCHEME_V3_ID);
}

namespace {
    // Helper to encode bytes to base64
    std::string encodeBase64(const std::vector<std::uint8_t>& data) {
        if (data.empty()) {
            return "";
        }
        
        BIO* bio = BIO_new(BIO_s_mem());
        BIO* b64 = BIO_new(BIO_f_base64());
        BIO_set_flags(b64, BIO_FLAGS_BASE64_NO_NL);
        bio = BIO_push(b64, bio);
        
        BIO_write(bio, data.data(), static_cast<int>(data.size()));
        BIO_flush(bio);
        
        char* encoded_data = nullptr;
        long encoded_len = BIO_get_mem_data(bio, &encoded_data);
        
        std::string result(encoded_data, encoded_len);
        
        BIO_free_all(bio);
        
        return result;
    }
    
    bool readLengthPrefixedSlice(const std::vector<std::uint8_t>& data, std::size_t& offset,
                                 std::vector<std::uint8_t>& result, const char* context = nullptr) {
        if (offset + 4 > data.size()) {
            return false;
        }
        std::uint32_t length = readUint32LE(data.data() + offset);
        offset += 4;
        
        if (length > data.size() - offset) {
            return false;
        }
        
        result.resize(length);
        std::copy(data.begin() + offset, data.begin() + offset + length, result.begin());
        offset += length;
        return true;
    }
}

std::unique_ptr<SignatureBlock> V2V3Parser::parse(const std::vector<std::uint8_t>& block_data,
                                                   std::uint32_t scheme_id) {
    if (block_data.empty() || block_data.size() < 8) {
        return nullptr;
    }
    
    std::size_t offset = 0;
    std::uint32_t outer_length = readUint32LE(block_data.data() + offset);
    offset += 4;
    
    if (outer_length > block_data.size() - offset) {
        return nullptr;
    }
    
    std::uint32_t inner_length = readUint32LE(block_data.data() + offset);
    offset += 4;
    
    if (inner_length > block_data.size() - offset || inner_length == 0) {
        return nullptr;
    }
    
    auto signature_block = std::make_unique<SignatureBlock>();
    signature_block->scheme_id = scheme_id;
    
    std::size_t signers_offset = offset;
    int signer_count = 0;
    
    while (signers_offset < offset + inner_length) {
        Signer signer;
        
        if (!readLengthPrefixedSlice(block_data, signers_offset, signer.signed_data, nullptr)) {
            break;
        }
        
        if (scheme_id == APK_SIGNATURE_SCHEME_V3_ID) {
            if (signers_offset + 8 > block_data.size()) {
                break;
            }
            signers_offset += 8;
        }
        
        std::vector<std::uint8_t> signatures_data;
        if (!readLengthPrefixedSlice(block_data, signers_offset, signatures_data, nullptr)) {
            break;
        }
        
        std::size_t sigs_offset = 0;
        while (sigs_offset < signatures_data.size()) {
            std::vector<std::uint8_t> signature_block_data;
            if (!readLengthPrefixedSlice(signatures_data, sigs_offset, signature_block_data, nullptr)) {
                break;
            }
            
            if (signature_block_data.size() < 4) {
                break;
            }
            
            std::uint32_t algorithm_id = readUint32LE(signature_block_data.data());
            std::size_t sig_block_offset = 4;
            std::vector<std::uint8_t> signature_bytes;
            if (!readLengthPrefixedSlice(signature_block_data, sig_block_offset, signature_bytes, nullptr)) {
                if (sig_block_offset < signature_block_data.size()) {
                    signature_bytes.assign(signature_block_data.begin() + sig_block_offset,
                                          signature_block_data.end());
                }
            }
            
            signer.signature_algorithms.push_back(algorithm_id);
            signer.signatures.push_back(std::move(signature_bytes));
        }
        
        std::vector<std::uint8_t> public_key;
        if (!readLengthPrefixedSlice(block_data, signers_offset, public_key, nullptr)) {
            break;
        }
        
        std::size_t signed_data_offset = 0;
        std::vector<std::uint8_t> digests;
        readLengthPrefixedSlice(signer.signed_data, signed_data_offset, digests, nullptr);
        
        std::vector<std::uint8_t> certificates_data;
        if (readLengthPrefixedSlice(signer.signed_data, signed_data_offset, certificates_data, nullptr)) {
            std::size_t certs_offset = 0;
            while (certs_offset < certificates_data.size()) {
                std::vector<std::uint8_t> cert_der;
                if (readLengthPrefixedSlice(certificates_data, certs_offset, cert_der, nullptr)) {
                    signer.certificates.push_back(std::move(cert_der));
                } else {
                    break;
                }
            }
        }
        
        signature_block->signers.push_back(std::move(signer));
        signer_count++;
        
        if (signer_count > 16) {
            LOGE_V23("Too many signers: %d", signer_count);
            break;
        }
    }
    
    return signature_block;
}

bool V2V3Parser::parseSignedData(const std::vector<std::uint8_t>& signed_data_bytes,
                                 std::vector<Signer>& signers) {
    // This function is not currently used - certificate extraction is done
    // directly via extractCertificates. Keeping for potential future use.
    (void)signed_data_bytes;
    (void)signers;
    return false;
}

bool V2V3Parser::parseSignerInfo(const std::vector<std::uint8_t>& signer_info_bytes,
                                 Signer& signer) {
    // This would require detailed ASN.1 parsing of SignerInfo structure
    // For now, we use OpenSSL's PKCS7 parsing which handles this
    return false;  // Not fully implemented - using parseSignedData instead
}

bool V2V3Parser::extractCertificates(const std::vector<std::uint8_t>& signer_info_bytes,
                                     std::vector<std::vector<std::uint8_t>>& certificates) {
    // Try to parse as PKCS7 to extract certificates
    const unsigned char* data = signer_info_bytes.data();
    PKCS7* pkcs7 = d2i_PKCS7(nullptr, &data, static_cast<long>(signer_info_bytes.size()));
    
    if (!pkcs7) {
        return false;
    }
    
    bool success = false;
    
    if (OBJ_obj2nid(pkcs7->type) == NID_pkcs7_signed) {
        PKCS7_SIGNED* signed_data = pkcs7->d.sign;
        if (signed_data) {
            STACK_OF(X509)* certs = signed_data->cert;
            if (certs) {
                int cert_count = sk_X509_num(certs);
                for (int i = 0; i < cert_count; ++i) {
                    X509* cert = sk_X509_value(certs, i);
                    if (cert) {
                        unsigned char* der = nullptr;
                        int der_len = i2d_X509(cert, &der);
                        if (der_len > 0 && der) {
                            certificates.emplace_back(der, der + der_len);
                            OPENSSL_free(der);
                            success = true;
                        }
                    }
                }
            }
        }
    }
    
    PKCS7_free(pkcs7);
    return success;
}

} // namespace apksig


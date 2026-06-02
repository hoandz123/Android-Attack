// Tiny AArch64 opcode decoders for the stripped IL2CPP registration scanner.
#pragma once

#include <cstdint>

namespace il2cpp_api {
namespace detail {

inline bool decode_bl(uint64_t pc, uint32_t insn, uint64_t *tgt) {
    if (!tgt) return false;
    if ((insn >> 26) != 0x25u) return false;
    uint32_t imm26 = insn & 0x03FFFFFFu;
    if ((imm26 & (1u << 25)) != 0) imm26 |= 0xFC000000u;

    uint64_t tgt_u = pc + static_cast<uint64_t>(static_cast<int32_t>(imm26) << 2);
    *tgt = tgt_u;
    return true;
}

inline bool decode_adrp(uint64_t pc, uint32_t insn, unsigned *rd, uint64_t *page) {
    if (!rd || !page) return false;
    if ((insn & 0x9F000000u) != 0x90000000u) return false;
    *rd = insn & 0x1Fu;
    uint32_t immhi = (insn >> 5) & 0x7FFFFu;
    uint32_t immlo = (insn >> 29) & 3u;
    uint32_t imm = (immhi << 2) | immlo;
    if (imm & (1u << 20)) imm |= 0xFFF00000u;

    uint64_t base = pc & ~static_cast<uint64_t>(0xFFFull);
    *page =
        base + static_cast<uint64_t>(static_cast<int64_t>(static_cast<int32_t>(imm)) << 12);
    return true;
}

inline bool decode_add_imm(uint32_t insn, unsigned *rd, unsigned *rn, uint64_t *imm) {
    if (!rd || !rn || !imm) return false;
    if ((insn & 0xFF800000u) != 0x91000000u) return false;
    *rd = insn & 0x1Fu;
    *rn = (insn >> 5) & 0x1Fu;
    uint32_t imm12 = (insn >> 10) & 0xFFFu;
    unsigned sh = (insn >> 22) & 1u;
    *imm = static_cast<uint64_t>(imm12) << (sh ? 12 : 0);
    return true;
}

inline bool decode_adr(uint64_t pc, uint32_t insn, unsigned *rd, uint64_t *addr) {
    if (!rd || !addr) return false;
    if (((insn >> 24) & 0x9Fu) != 0x10u) return false;

    *rd = insn & 0x1Fu;
    uint32_t immlo = (insn >> 29) & 3u;
    uint32_t immhi = (insn >> 5) & 0x7FFFFu;
    uint32_t imm = (immhi << 2) | immlo;
    if (imm & (1u << 20)) imm |= 0xFFF00000u;

    *addr =
        static_cast<uint64_t>(static_cast<int64_t>(pc) +
                              static_cast<int64_t>(static_cast<int32_t>(imm)));
    return true;
}

inline bool is_control_flow(uint32_t insn) {
    const uint32_t op6 = insn >> 26;
    if (op6 == 0x05u || op6 == 0x25u) return true;
    if (((insn >> 24) & 0xFFu) == 0x54u) return true;
    if ((insn & 0x7E000000u) == 0x34000000u) return true;
    if ((insn & 0x7E000000u) == 0x36000000u) return true;
    if ((insn & 0xFE1FFC1Fu) == 0xD61F0000u) return true;
    if ((insn & 0xFE1FFC1Fu) == 0xD63F0000u) return true;
    if ((insn & 0xFFFFFC1Fu) == 0xD65F0000u) return true;
    return false;
}

}  // namespace detail
}  // namespace il2cpp_api

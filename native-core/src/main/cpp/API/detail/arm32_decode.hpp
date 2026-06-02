// Tiny ARM (A32) mode opcode decoders for stripped IL2CPP registration scanner (armv7a).
#pragma once

#include <cstdint>

namespace il2cpp_api {
namespace detail {

inline uint32_t arm32_ror32(uint32_t val, unsigned rot) {
    rot &= 31u;
    if (rot == 0) return val;
    return (val >> rot) | (val << (32u - rot));
}

inline bool decode_bl_arm(uint32_t pc, uint32_t insn, uint64_t *tgt) {
    if (!tgt) return false;

    if ((insn & 0xFE000000u) == 0xFA000000u) {
        uint32_t imm24 = insn & 0xFFFFFFu;
        int32_t simm = static_cast<int32_t>(imm24 << 8) >> 8;
        int64_t disp =
            (static_cast<int64_t>(simm) * 4) + static_cast<int64_t>((insn >> 23) & 2u);
        *tgt = static_cast<uint64_t>(static_cast<int64_t>(static_cast<uint32_t>(pc) + 8) + disp);
        return true;
    }

    if ((insn & 0x0F000000u) == 0x0B000000u && ((insn >> 28) & 0xFu) != 0xFu) {
        uint32_t imm24 = insn & 0xFFFFFFu;
        int32_t simm = static_cast<int32_t>(imm24 << 8) >> 8;
        int64_t disp = static_cast<int64_t>(simm) * 4;
        *tgt = static_cast<uint64_t>(static_cast<int64_t>(static_cast<uint32_t>(pc) + 8) + disp);
        return true;
    }

    return false;
}

inline bool decode_ldr_literal(uint32_t pc, uint32_t insn, unsigned *rt, uint32_t *addr) {
    if (!rt || !addr) return false;
    if ((insn & 0x0F7F0000u) != 0x051F0000u) return false;
    const unsigned imm12 = insn & 0xFFFu;
    const unsigned U = (insn >> 23) & 1u;
    const int32_t addend = U ? static_cast<int32_t>(imm12) : -static_cast<int32_t>(imm12);
    *rt = (insn >> 12) & 0xFu;
    *addr = static_cast<uint32_t>(static_cast<int64_t>(static_cast<uint32_t>(pc) + 8) + addend);
    return true;
}

/** LDR Rt, [Rn, #imm] (offset, P=1, W=0, L=1, B=0) with Rn != PC (literal uses decode_ldr_literal). */
inline bool decode_ldr_imm(uint32_t insn, unsigned *rt, unsigned *rn, int32_t *off) {
    if (!rt || !rn || !off) return false;
    if ((insn & 0x0F700000u) != 0x05100000u) return false;
    *rn = (insn >> 16) & 0xFu;
    if (*rn == 15u) return false;
    *rt = (insn >> 12) & 0xFu;
    const unsigned imm12 = insn & 0xFFFu;
    const unsigned U = (insn >> 23) & 1u;
    *off = U ? static_cast<int32_t>(imm12) : -static_cast<int32_t>(imm12);
    return true;
}

inline bool decode_add_pc_imm(uint32_t pc, uint32_t insn, unsigned *rd, uint32_t *addr) {
    if (!rd || !addr) return false;
    const uint32_t imm8 = insn & 0xFFu;
    const unsigned rot = ((insn >> 8) & 0xFu) * 2u;
    const uint32_t value = arm32_ror32(imm8, rot);

    if ((insn & 0x0FFF0000u) == 0x028F0000u) {
        *rd = (insn >> 12) & 0xFu;
        *addr = static_cast<uint32_t>(static_cast<uint64_t>(static_cast<uint32_t>(pc) + 8) +
                                      static_cast<uint64_t>(value));
        return true;
    }
    if ((insn & 0x0FFF0000u) == 0x024F0000u) {
        *rd = (insn >> 12) & 0xFu;
        *addr = static_cast<uint32_t>(static_cast<int64_t>(static_cast<uint32_t>(pc) + 8) -
                                      static_cast<int64_t>(value));
        return true;
    }
    return false;
}

/** ADD Rd, PC, Rm — register form, no shift, S=0 (Unity IL2CPP registration prelude). */
inline bool decode_add_pc_reg(uint32_t insn, unsigned *rd, unsigned *rm) {
    if (!rd || !rm) return false;
    if ((insn & 0x0FEF0FF0u) != 0x008F0000u) return false;
    *rd = (insn >> 12) & 0xFu;
    *rm = insn & 0xFu;
    return true;
}

inline bool decode_movw(uint32_t insn, unsigned *rd, uint16_t *imm16) {
    if (!rd || !imm16) return false;
    if ((insn & 0x0FF00000u) != 0x03000000u) return false;
    *rd = (insn >> 12) & 0xFu;
    *imm16 = static_cast<uint16_t>(((insn >> 4) & 0xF000u) | (insn & 0xFFFu));
    return true;
}

inline bool decode_movt(uint32_t insn, unsigned *rd, uint16_t *imm16) {
    if (!rd || !imm16) return false;
    if ((insn & 0x0FF00000u) != 0x03400000u) return false;
    *rd = (insn >> 12) & 0xFu;
    *imm16 = static_cast<uint16_t>(((insn >> 4) & 0xF000u) | (insn & 0xFFFu));
    return true;
}

inline bool is_control_flow_arm(uint32_t insn) {
    if ((insn & 0x0E000000u) == 0x0A000000u) return true;
    if ((insn & 0xFE000000u) == 0xFA000000u) return true;
    if ((insn & 0x0FFFFFD0u) == 0x012FFF10u) return true;
    if ((insn & 0x0FE0F010u) == 0x01A0F000u) return true;
    if ((insn & 0x0E50F000u) == 0x0410F000u) return true;
    if ((insn & 0x0FFF8000u) == 0x08BD8000u) return true;
    if ((insn & 0x0E000000u) == 0x08000000u && (insn & 0x00100000u) != 0u &&
        (insn & 0x00008000u) != 0u)
        return true;
    return false;
}

}  // namespace detail
}  // namespace il2cpp_api

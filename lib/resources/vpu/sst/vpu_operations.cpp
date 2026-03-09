#include "vpu_operations.h"
#include "vpu.h"

namespace VPU_Operations {

std::function<void()> getVPUHandler(Vpu *vpu, VPU_PKG::VPU_MODE mode) {
  static auto handlers = VPU_Operations::createHandlers(vpu);
  if (handlers.find(mode) != handlers.end()) {
    return handlers[mode];
  } else {
    vpu->out.fatal(CALL_INFO, -1, "Unsupported VPU mode: %d\n", mode);
    return [] {}; // This line will never be reached due to fatal exit
  }
}

std::unordered_map<VPU_PKG::VPU_MODE, std::function<void()>>
createHandlers(Vpu *vpu) {
  return {
      {VPU_PKG::VPU_MODE::VPU_MODE_IDLE, [vpu] { Impl::handle_idle(vpu); }},
      {VPU_PKG::VPU_MODE::VPU_MODE_ADD, [vpu] { Impl::handle_add(vpu); }},
      {VPU_PKG::VPU_MODE::VPU_MODE_ADD_ACC,
       [vpu] { Impl::handle_add_acc(vpu); }},
      {VPU_PKG::VPU_MODE::VPU_MODE_SUB, [vpu] { Impl::handle_sub(vpu); }},
      {VPU_PKG::VPU_MODE::VPU_MODE_SUB_ACC,
       [vpu] { Impl::handle_sub_acc(vpu); }},
      {VPU_PKG::VPU_MODE::VPU_MODE_MUL, [vpu] { Impl::handle_mul(vpu); }},
      {VPU_PKG::VPU_MODE::VPU_MODE_MAC, [vpu] { Impl::handle_mac(vpu); }},
      {VPU_PKG::VPU_MODE::VPU_MODE_MIN, [vpu] { Impl::handle_min(vpu); }},
      {VPU_PKG::VPU_MODE::VPU_MODE_MAX, [vpu] { Impl::handle_max(vpu); }},
      {VPU_PKG::VPU_MODE::VPU_MODE_LSHIFT, [vpu] { Impl::handle_lshift(vpu); }},
      {VPU_PKG::VPU_MODE::VPU_MODE_RSHIFT, [vpu] { Impl::handle_rshift(vpu); }},
      {VPU_PKG::VPU_MODE::VPU_MODE_SEQ, [vpu] { Impl::handle_seq(vpu); }},
      {VPU_PKG::VPU_MODE::VPU_MODE_SNE, [vpu] { Impl::handle_sne(vpu); }},
      {VPU_PKG::VPU_MODE::VPU_MODE_SLT, [vpu] { Impl::handle_slt(vpu); }},
      {VPU_PKG::VPU_MODE::VPU_MODE_SLE, [vpu] { Impl::handle_sle(vpu); }},
      {VPU_PKG::VPU_MODE::VPU_MODE_SGT, [vpu] { Impl::handle_sgt(vpu); }},
      {VPU_PKG::VPU_MODE::VPU_MODE_SGE, [vpu] { Impl::handle_sge(vpu); }},
      {VPU_PKG::VPU_MODE::VPU_MODE_AND, [vpu] { Impl::handle_and(vpu); }},
      {VPU_PKG::VPU_MODE::VPU_MODE_OR, [vpu] { Impl::handle_or(vpu); }},
      {VPU_PKG::VPU_MODE::VPU_MODE_XOR, [vpu] { Impl::handle_xor(vpu); }},
      {VPU_PKG::VPU_MODE::VPU_MODE_VADD, [vpu] { Impl::handle_vadd(vpu); }},
      {VPU_PKG::VPU_MODE::VPU_MODE_VADD_ACC,
       [vpu] { Impl::handle_vadd_acc(vpu); }},
      {VPU_PKG::VPU_MODE::VPU_MODE_VSUB, [vpu] { Impl::handle_vsub(vpu); }},
      {VPU_PKG::VPU_MODE::VPU_MODE_VSUB_ACC,
       [vpu] { Impl::handle_vsub_acc(vpu); }},
      {VPU_PKG::VPU_MODE::VPU_MODE_VMUL, [vpu] { Impl::handle_vmul(vpu); }},
      {VPU_PKG::VPU_MODE::VPU_MODE_VMAC, [vpu] { Impl::handle_vmac(vpu); }},
      {VPU_PKG::VPU_MODE::VPU_MODE_VMIN, [vpu] { Impl::handle_vmin(vpu); }},
      {VPU_PKG::VPU_MODE::VPU_MODE_VMAX, [vpu] { Impl::handle_vmax(vpu); }},
      {VPU_PKG::VPU_MODE::VPU_MODE_VLSHIFT,
       [vpu] { Impl::handle_vlshift(vpu); }},
      {VPU_PKG::VPU_MODE::VPU_MODE_VRSHIFT,
       [vpu] { Impl::handle_vrshift(vpu); }},
      {VPU_PKG::VPU_MODE::VPU_MODE_VSEQ, [vpu] { Impl::handle_vseq(vpu); }},
      {VPU_PKG::VPU_MODE::VPU_MODE_VSNE, [vpu] { Impl::handle_vsne(vpu); }},
      {VPU_PKG::VPU_MODE::VPU_MODE_VSLT, [vpu] { Impl::handle_vslt(vpu); }},
      {VPU_PKG::VPU_MODE::VPU_MODE_VSLE, [vpu] { Impl::handle_vsle(vpu); }},
      {VPU_PKG::VPU_MODE::VPU_MODE_VSGT, [vpu] { Impl::handle_vsgt(vpu); }},
      {VPU_PKG::VPU_MODE::VPU_MODE_VSGE, [vpu] { Impl::handle_vsge(vpu); }},
      {VPU_PKG::VPU_MODE::VPU_MODE_VAND, [vpu] { Impl::handle_vand(vpu); }},
      {VPU_PKG::VPU_MODE::VPU_MODE_VOR, [vpu] { Impl::handle_vor(vpu); }},
      {VPU_PKG::VPU_MODE::VPU_MODE_VXOR, [vpu] { Impl::handle_vxor(vpu); }},
      {VPU_PKG::VPU_MODE::VPU_MODE_VNOT, [vpu] { Impl::handle_vnot(vpu); }},
      {VPU_PKG::VPU_MODE::VPU_MODE_ADD_IMM,
       [vpu] { Impl::handle_add_imm(vpu); }},
      {VPU_PKG::VPU_MODE::VPU_MODE_MUL_IMM,
       [vpu] { Impl::handle_mul_imm(vpu); }},
      {VPU_PKG::VPU_MODE::VPU_MODE_MIN_IMM,
       [vpu] { Impl::handle_min_imm(vpu); }},
      {VPU_PKG::VPU_MODE::VPU_MODE_MAX_IMM,
       [vpu] { Impl::handle_max_imm(vpu); }},
      {VPU_PKG::VPU_MODE::VPU_MODE_LSHIFT_IMM,
       [vpu] { Impl::handle_lshift_imm(vpu); }},
      {VPU_PKG::VPU_MODE::VPU_MODE_RSHIFT_IMM,
       [vpu] { Impl::handle_rshift_imm(vpu); }},
      {VPU_PKG::VPU_MODE::VPU_MODE_SEQ_IMM,
       [vpu] { Impl::handle_seq_imm(vpu); }},
      {VPU_PKG::VPU_MODE::VPU_MODE_SNE_IMM,
       [vpu] { Impl::handle_sne_imm(vpu); }},
      {VPU_PKG::VPU_MODE::VPU_MODE_SLT_IMM,
       [vpu] { Impl::handle_slt_imm(vpu); }},
      {VPU_PKG::VPU_MODE::VPU_MODE_SLE_IMM,
       [vpu] { Impl::handle_sle_imm(vpu); }},
      {VPU_PKG::VPU_MODE::VPU_MODE_SGT_IMM,
       [vpu] { Impl::handle_sgt_imm(vpu); }},
      {VPU_PKG::VPU_MODE::VPU_MODE_SGE_IMM,
       [vpu] { Impl::handle_sge_imm(vpu); }},
      {VPU_PKG::VPU_MODE::VPU_MODE_AND_IMM,
       [vpu] { Impl::handle_and_imm(vpu); }},
      {VPU_PKG::VPU_MODE::VPU_MODE_OR_IMM, [vpu] { Impl::handle_or_imm(vpu); }},
      {VPU_PKG::VPU_MODE::VPU_MODE_XOR_IMM,
       [vpu] { Impl::handle_xor_imm(vpu); }}};
}

namespace Impl {

void handle_idle(Vpu *vpu) {
  // Access through public interface - assuming VPU has an 'out' member
  // accessible You might need to add a getOutput() method to VPU class
  // vpu->getOutput().output("IDLE\n");
}

void handle_add(Vpu *vpu) {
  vpu->handleOperation("add",
                       [](std::vector<int64_t> a, std::vector<int64_t> b) {
                         std::vector<int64_t> result;
                         for (auto &elem : a) {
                           result.push_back(add_sat(elem, b[0]));
                         }
                         return result;
                       });
}

void handle_add_acc(Vpu *vpu) {
  vpu->getOutput().fatal(CALL_INFO, -1, "add_acc is not implemented in VPU\n");
}

void handle_sub(Vpu *vpu) {
  vpu->handleOperation("sub",
                       [](std::vector<int64_t> a, std::vector<int64_t> b) {
                         std::vector<int64_t> result;
                         for (auto &elem : a) {
                           result.push_back(add_sat(elem, -b[0]));
                         }
                         return result;
                       });
}

void handle_sub_acc(Vpu *vpu) {
  vpu->getOutput().fatal(CALL_INFO, -1, "sub_acc is not implemented in VPU\n");
}

void handle_mul(Vpu *vpu) {
  vpu->handleOperation(
      "mul", [vpu](std::vector<int64_t> a, std::vector<int64_t> b) {
        std::vector<int64_t> result;
        for (auto &elem : a) {
          result.push_back(mul_sat(elem, b[0], vpu->getWordBitwidth(),
                                   vpu->fractional_bitwidth));
        }
        return result;
      });
}

void handle_mac(Vpu *vpu) {
  vpu->handleOperation(
      "MAC", [vpu](std::vector<int64_t> a, std::vector<int64_t> b) {
        std::vector<int64_t> vec_result;
        // Note: This might need adjustment based on how you want to handle
        // the accumulate register access. You might need to add more public
        // methods to the VPU class.
        auto &acc_reg = vpu->getAccumulateRegister();

        // This assumes vectorToInt64 and int64ToVector are accessible
        // You might need to move these to vpu_pkg.h or make them public
        for (int i = 0; i < a.size(); i++) {
          int64_t result = add_sat(mul_sat(a[i], b[0], vpu->getWordBitwidth(),
                                           vpu->fractional_bitwidth),
                                   acc_reg[i]);

          acc_reg[i] = result;
          vec_result.push_back(result);
        }
        return vec_result;
      });
}

void handle_min(Vpu *vpu) {
  vpu->handleOperation("min",
                       [](std::vector<int64_t> a, std::vector<int64_t> b) {
                         std::vector<int64_t> result;
                         for (size_t i = 0; i < a.size(); i++) {
                           result.push_back(std::min(a[i], b[0]));
                         }
                         return result;
                       });
}

void handle_max(Vpu *vpu) {
  vpu->handleOperation("max",
                       [](std::vector<int64_t> a, std::vector<int64_t> b) {
                         std::vector<int64_t> result;
                         for (size_t i = 0; i < a.size(); i++) {
                           result.push_back(std::max(a[i], b[0]));
                         }
                         return result;
                       });
}

void handle_lshift(Vpu *vpu) {
  vpu->handleOperation("lshift",
                       [](std::vector<int64_t> a, std::vector<int64_t> b) {
                         std::vector<int64_t> result;
                         for (size_t i = 0; i < a.size(); i++) {
                           result.push_back(a[i] << b[0]);
                         }
                         return result;
                       });
}

void handle_rshift(Vpu *vpu) {
  vpu->handleOperation("rshift",
                       [](std::vector<int64_t> a, std::vector<int64_t> b) {
                         std::vector<int64_t> result;
                         for (size_t i = 0; i < a.size(); i++) {
                           result.push_back(a[i] >> b[0]);
                         }
                         return result;
                       });
}

void handle_seq(Vpu *vpu) {
  vpu->handleOperation("seq",
                       [](std::vector<int64_t> a, std::vector<int64_t> b) {
                         std::vector<int64_t> result;
                         for (size_t i = 0; i < a.size(); i++) {
                           result.push_back(a[i] == b[0] ? 1 : 0);
                         }
                         return result;
                       });
}

void handle_sne(Vpu *vpu) {
  vpu->handleOperation("sne",
                       [](std::vector<int64_t> a, std::vector<int64_t> b) {
                         std::vector<int64_t> result;
                         for (size_t i = 0; i < a.size(); i++) {
                           result.push_back(a[i] != b[0] ? 1 : 0);
                         }
                         return result;
                       });
}

void handle_slt(Vpu *vpu) {
  vpu->handleOperation("slt",
                       [](std::vector<int64_t> a, std::vector<int64_t> b) {
                         std::vector<int64_t> result;
                         for (size_t i = 0; i < a.size(); i++) {
                           result.push_back(a[i] < b[0] ? 1 : 0);
                         }
                         return result;
                       });
}

void handle_sle(Vpu *vpu) {
  vpu->handleOperation("sle",
                       [](std::vector<int64_t> a, std::vector<int64_t> b) {
                         std::vector<int64_t> result;
                         for (size_t i = 0; i < a.size(); i++) {
                           result.push_back(a[i] <= b[0] ? 1 : 0);
                         }
                         return result;
                       });
}

void handle_sgt(Vpu *vpu) {
  vpu->handleOperation("sgt",
                       [](std::vector<int64_t> a, std::vector<int64_t> b) {
                         std::vector<int64_t> result;
                         for (size_t i = 0; i < a.size(); i++) {
                           result.push_back(a[i] > b[0] ? 1 : 0);
                         }
                         return result;
                       });
}

void handle_sge(Vpu *vpu) {
  vpu->handleOperation("sge",
                       [](std::vector<int64_t> a, std::vector<int64_t> b) {
                         std::vector<int64_t> result;
                         for (size_t i = 0; i < a.size(); i++) {
                           result.push_back(a[i] >= b[0] ? 1 : 0);
                         }
                         return result;
                       });
}

void handle_and(Vpu *vpu) {
  vpu->handleOperation("and",
                       [](std::vector<int64_t> a, std::vector<int64_t> b) {
                         std::vector<int64_t> result;
                         for (size_t i = 0; i < a.size(); i++) {
                           result.push_back(a[i] & b[0]);
                         }
                         return result;
                       });
}

void handle_or(Vpu *vpu) {
  vpu->handleOperation("or",
                       [](std::vector<int64_t> a, std::vector<int64_t> b) {
                         std::vector<int64_t> result;
                         for (size_t i = 0; i < a.size(); i++) {
                           result.push_back(a[i] | b[0]);
                         }
                         return result;
                       });
}

void handle_xor(Vpu *vpu) {
  vpu->handleOperation("xor",
                       [](std::vector<int64_t> a, std::vector<int64_t> b) {
                         std::vector<int64_t> result;
                         for (size_t i = 0; i < a.size(); i++) {
                           result.push_back(a[i] ^ b[0]);
                         }
                         return result;
                       });
}

void handle_vadd(Vpu *vpu) {
  vpu->handleOperation("vadd",
                       [](std::vector<int64_t> a, std::vector<int64_t> b) {
                         std::vector<int64_t> result;
                         for (size_t i = 0; i < a.size(); i++) {
                           result.push_back(add_sat(a[i], b[i]));
                         }
                         return result;
                       });
}

void handle_vsub(Vpu *vpu) {
  vpu->handleOperation("vsub",
                       [](std::vector<int64_t> a, std::vector<int64_t> b) {
                         std::vector<int64_t> result;
                         for (size_t i = 0; i < a.size(); i++) {
                           result.push_back(add_sat(a[i], -b[i]));
                         }
                         return result;
                       });
}

void handle_vmul(Vpu *vpu) {
  vpu->handleOperation(
      "vmul", [vpu](std::vector<int64_t> a, std::vector<int64_t> b) {
        std::vector<int64_t> result;
        for (size_t i = 0; i < a.size(); i++) {
          result.push_back(mul_sat(a[i], b[i], vpu->getWordBitwidth(),
                                   vpu->fractional_bitwidth));
        }
        return result;
      });
}

void handle_vmac(Vpu *vpu) {
  vpu->handleOperation(
      "VMAC", [vpu](std::vector<int64_t> a, std::vector<int64_t> b) {
        std::vector<int64_t> vec_result;
        auto &acc_reg = vpu->getAccumulateRegister();

        for (size_t i = 0; i < a.size(); i++) {
          int64_t result = add_sat(mul_sat(a[i], b[i], vpu->getWordBitwidth(),
                                           vpu->fractional_bitwidth),
                                   acc_reg[i]);

          acc_reg[i] = result;
          vec_result.push_back(result);
        }
        return vec_result;
      });
}

void handle_vmin(Vpu *vpu) {
  vpu->handleOperation("vmin",
                       [](std::vector<int64_t> a, std::vector<int64_t> b) {
                         std::vector<int64_t> result;
                         for (size_t i = 0; i < a.size(); i++) {
                           result.push_back(std::min(a[i], b[i]));
                         }
                         return result;
                       });
}

void handle_vmax(Vpu *vpu) {
  vpu->handleOperation("vmax",
                       [](std::vector<int64_t> a, std::vector<int64_t> b) {
                         std::vector<int64_t> result;
                         for (size_t i = 0; i < a.size(); i++) {
                           result.push_back(std::max(a[i], b[i]));
                         }
                         return result;
                       });
}

void handle_vlshift(Vpu *vpu) {
  vpu->handleOperation("vlshift",
                       [](std::vector<int64_t> a, std::vector<int64_t> b) {
                         std::vector<int64_t> result;
                         for (size_t i = 0; i < a.size(); i++) {
                           result.push_back(a[i] << b[i]);
                         }
                         return result;
                       });
}

void handle_vrshift(Vpu *vpu) {
  vpu->handleOperation("vrshift",
                       [](std::vector<int64_t> a, std::vector<int64_t> b) {
                         std::vector<int64_t> result;
                         for (size_t i = 0; i < a.size(); i++) {
                           result.push_back(a[i] >> b[i]);
                         }
                         return result;
                       });
}

void handle_vseq(Vpu *vpu) {
  vpu->handleOperation("vseq",
                       [](std::vector<int64_t> a, std::vector<int64_t> b) {
                         std::vector<int64_t> result;
                         for (size_t i = 0; i < a.size(); i++) {
                           result.push_back(a[i] == b[i] ? 1 : 0);
                         }
                         return result;
                       });
}

void handle_vsne(Vpu *vpu) {
  vpu->handleOperation("vsne",
                       [](std::vector<int64_t> a, std::vector<int64_t> b) {
                         std::vector<int64_t> result;
                         for (size_t i = 0; i < a.size(); i++) {
                           result.push_back(a[i] != b[i] ? 1 : 0);
                         }
                         return result;
                       });
}

void handle_vslt(Vpu *vpu) {
  vpu->handleOperation("vslt",
                       [](std::vector<int64_t> a, std::vector<int64_t> b) {
                         std::vector<int64_t> result;
                         for (size_t i = 0; i < a.size(); i++) {
                           result.push_back(a[i] < b[i] ? 1 : 0);
                         }
                         return result;
                       });
}

void handle_vsle(Vpu *vpu) {
  vpu->handleOperation("vsle",
                       [](std::vector<int64_t> a, std::vector<int64_t> b) {
                         std::vector<int64_t> result;
                         for (size_t i = 0; i < a.size(); i++) {
                           result.push_back(a[i] <= b[i] ? 1 : 0);
                         }
                         return result;
                       });
}

void handle_vsgt(Vpu *vpu) {
  vpu->handleOperation("vsgt",
                       [](std::vector<int64_t> a, std::vector<int64_t> b) {
                         std::vector<int64_t> result;
                         for (size_t i = 0; i < a.size(); i++) {
                           result.push_back(a[i] > b[i] ? 1 : 0);
                         }
                         return result;
                       });
}

void handle_vsge(Vpu *vpu) {
  vpu->handleOperation("vsge",
                       [](std::vector<int64_t> a, std::vector<int64_t> b) {
                         std::vector<int64_t> result;
                         for (size_t i = 0; i < a.size(); i++) {
                           result.push_back(a[i] >= b[i] ? 1 : 0);
                         }
                         return result;
                       });
}

void handle_vand(Vpu *vpu) {
  vpu->handleOperation("vand",
                       [](std::vector<int64_t> a, std::vector<int64_t> b) {
                         std::vector<int64_t> result;
                         for (size_t i = 0; i < a.size(); i++) {
                           result.push_back(a[i] & b[i]);
                         }
                         return result;
                       });
}

void handle_vor(Vpu *vpu) {
  vpu->handleOperation("vor",
                       [](std::vector<int64_t> a, std::vector<int64_t> b) {
                         std::vector<int64_t> result;
                         for (size_t i = 0; i < a.size(); i++) {
                           result.push_back(a[i] | b[i]);
                         }
                         return result;
                       });
}

void handle_vxor(Vpu *vpu) {
  vpu->handleOperation("vxor",
                       [](std::vector<int64_t> a, std::vector<int64_t> b) {
                         std::vector<int64_t> result;
                         for (size_t i = 0; i < a.size(); i++) {
                           result.push_back(a[i] ^ b[i]);
                         }
                         return result;
                       });
}

void handle_vnot(Vpu *vpu) {
  vpu->handleOperation("vnot",
                       [](std::vector<int64_t> a, std::vector<int64_t> b) {
                         std::vector<int64_t> result;
                         for (size_t i = 0; i < a.size(); i++) {
                           result.push_back(~a[i]);
                         }
                         return result;
                       });
}

void handle_add_imm(Vpu *vpu) {
  vpu->handleOperation("add_imm",
                       [](std::vector<int64_t> a, std::vector<int64_t> b) {
                         std::vector<int64_t> result;
                         for (auto &elem : a) {
                           result.push_back(add_sat(elem, b[0]));
                         }
                         return result;
                       });
}

void handle_sub_imm(Vpu *vpu) {
  vpu->handleOperation("sub_imm",
                       [](std::vector<int64_t> a, std::vector<int64_t> b) {
                         std::vector<int64_t> result;
                         for (auto &elem : a) {
                           result.push_back(add_sat(elem, -b[0]));
                         }
                         return result;
                       });
}

void handle_mul_imm(Vpu *vpu) {
  vpu->handleOperation(
      "mul_imm", [vpu](std::vector<int64_t> a, std::vector<int64_t> b) {
        std::vector<int64_t> result;
        for (auto &elem : a) {
          result.push_back(mul_sat(elem, b[0], vpu->getWordBitwidth(),
                                   vpu->fractional_bitwidth));
        }
        return result;
      });
}

void handle_mac_imm(Vpu *vpu) {
  vpu->handleOperation(
      "MAC_imm", [vpu](std::vector<int64_t> a, std::vector<int64_t> b) {
        std::vector<int64_t> vec_result;
        // Note: This might need adjustment based on how you want to handle
        // the accumulate register access. You might need to add more public
        // methods to the VPU class.
        auto &acc_reg = vpu->getAccumulateRegister();

        // This assumes vectorToInt64 and int64ToVector are accessible
        // You might need to move these to vpu_pkg.h or make them public
        for (int i = 0; i < a.size(); i++) {
          int64_t result = add_sat(mul_sat(a[i], b[0], vpu->getWordBitwidth(),
                                           vpu->fractional_bitwidth),
                                   acc_reg[i]);

          acc_reg[i] = result;
          vec_result.push_back(result);
        }
        return vec_result;
      });
}

void handle_min_imm(Vpu *vpu) {
  vpu->handleOperation("min_imm",
                       [](std::vector<int64_t> a, std::vector<int64_t> b) {
                         std::vector<int64_t> result;
                         for (size_t i = 0; i < a.size(); i++) {
                           result.push_back(std::min(a[i], b[0]));
                         }
                         return result;
                       });
}

void handle_max_imm(Vpu *vpu) {
  vpu->handleOperation("max_imm",
                       [](std::vector<int64_t> a, std::vector<int64_t> b) {
                         std::vector<int64_t> result;
                         for (size_t i = 0; i < a.size(); i++) {
                           result.push_back(std::max(a[i], b[0]));
                         }
                         return result;
                       });
}

void handle_lshift_imm(Vpu *vpu) {
  vpu->handleOperation("lshift_imm",
                       [](std::vector<int64_t> a, std::vector<int64_t> b) {
                         std::vector<int64_t> result;
                         for (size_t i = 0; i < a.size(); i++) {
                           result.push_back(a[i] << b[0]);
                         }
                         return result;
                       });
}

void handle_rshift_imm(Vpu *vpu) {
  vpu->handleOperation("rshift_imm",
                       [](std::vector<int64_t> a, std::vector<int64_t> b) {
                         std::vector<int64_t> result;
                         for (size_t i = 0; i < a.size(); i++) {
                           result.push_back(a[i] >> b[0]);
                         }
                         return result;
                       });
}

void handle_seq_imm(Vpu *vpu) {
  vpu->handleOperation("seq_imm",
                       [](std::vector<int64_t> a, std::vector<int64_t> b) {
                         std::vector<int64_t> result;
                         for (size_t i = 0; i < a.size(); i++) {
                           result.push_back(a[i] == b[0] ? 1 : 0);
                         }
                         return result;
                       });
}

void handle_sne_imm(Vpu *vpu) {
  vpu->handleOperation("sne_imm",
                       [](std::vector<int64_t> a, std::vector<int64_t> b) {
                         std::vector<int64_t> result;
                         for (size_t i = 0; i < a.size(); i++) {
                           result.push_back(a[i] != b[0] ? 1 : 0);
                         }
                         return result;
                       });
}

void handle_slt_imm(Vpu *vpu) {
  vpu->handleOperation("slt_imm",
                       [](std::vector<int64_t> a, std::vector<int64_t> b) {
                         std::vector<int64_t> result;
                         for (size_t i = 0; i < a.size(); i++) {
                           result.push_back(a[i] < b[0] ? 1 : 0);
                         }
                         return result;
                       });
}

void handle_sle_imm(Vpu *vpu) {
  vpu->handleOperation("sle_imm",
                       [](std::vector<int64_t> a, std::vector<int64_t> b) {
                         std::vector<int64_t> result;
                         for (size_t i = 0; i < a.size(); i++) {
                           result.push_back(a[i] <= b[0] ? 1 : 0);
                         }
                         return result;
                       });
}

void handle_sgt_imm(Vpu *vpu) {
  vpu->handleOperation("sgt_imm",
                       [](std::vector<int64_t> a, std::vector<int64_t> b) {
                         std::vector<int64_t> result;
                         for (size_t i = 0; i < a.size(); i++) {
                           result.push_back(a[i] > b[0] ? 1 : 0);
                         }
                         return result;
                       });
}

void handle_sge_imm(Vpu *vpu) {
  vpu->handleOperation("sge_imm",
                       [](std::vector<int64_t> a, std::vector<int64_t> b) {
                         std::vector<int64_t> result;
                         for (size_t i = 0; i < a.size(); i++) {
                           result.push_back(a[i] >= b[0] ? 1 : 0);
                         }
                         return result;
                       });
}

void handle_and_imm(Vpu *vpu) {
  vpu->handleOperation("and_imm",
                       [](std::vector<int64_t> a, std::vector<int64_t> b) {
                         std::vector<int64_t> result;
                         for (size_t i = 0; i < a.size(); i++) {
                           result.push_back(a[i] & b[0]);
                         }
                         return result;
                       });
}

void handle_or_imm(Vpu *vpu) {
  vpu->handleOperation("or_imm",
                       [](std::vector<int64_t> a, std::vector<int64_t> b) {
                         std::vector<int64_t> result;
                         for (size_t i = 0; i < a.size(); i++) {
                           result.push_back(a[i] | b[0]);
                         }
                         return result;
                       });
}

void handle_xor_imm(Vpu *vpu) {
  vpu->handleOperation("xor_imm",
                       [](std::vector<int64_t> a, std::vector<int64_t> b) {
                         std::vector<int64_t> result;
                         for (size_t i = 0; i < a.size(); i++) {
                           result.push_back(a[i] ^ b[0]);
                         }
                         return result;
                       });
}

} // namespace Impl
} // namespace VPU_Operations
